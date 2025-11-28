# Ambra Bytecode Specification (v0.1)

This document defines the **Ambra bytecode format** and the instruction set for the Ambra Virtual Machine (AVM) in version 0.1.

It is intended as a reference for implementing:

- the **code generation** phase of the compiler, and  
- the **execution engine** of the AVM.

No implementation language is assumed; this is purely conceptual.

---

## 1. Design Goals

The Ambra bytecode is designed to be:

- **Simple** — easy to generate and interpret.
- **Stack-based** — most operations use an operand stack.
- **Stable** — changes in later versions can extend, not replace, this format.
- **Educational** — emphasizes clarity over density or performance.

---

## 2. High-Level Model

The AVM maintains:

- An **instruction pointer (IP)** into the bytecode stream.
- An **operand stack** for intermediate values.
- A **global variable store**, addressed by integer indices.
- A **constant pool** of literals (integers, strings, booleans).
- A **halted state** when execution finishes.

Each instruction consists of:

- An **opcode** (identifying the operation).
- Zero or more **operands** (e.g., constant index, jump offset, variable index).

The exact binary encoding of opcodes and operands is implementation-defined.  
This document defines the *logical meaning* of each instruction.

---

## 3. Runtime Value Model

The AVM must support three value kinds in v0.1:

1. **Integer**
2. **Boolean**
3. **String**

Conceptually, a `Value` is tagged with its type:

- `Int(value)`
- `Bool(value)`
- `Str(handle)` — where `handle` points into a string table or heap.

The operand stack holds these `Value` instances.

---

## 4. Constant Pool

The **constant pool** is a fixed array of values embedded in the bytecode file.

At minimum, it contains:

- String literals (from source)
- Integer literals (optional optimization)
- Possibly pre-interned booleans (`affirmative` and `negative`)

Each entry has an index:

```text
const[0], const[1], ..., const[N-1]
```

Instructions such as `LOAD_CONST` refer to constants by **index**.

The compiler decides which literals go into the constant pool, and may choose to:

- Pool common values (e.g., reusing `" "` or `"Count:"`)  
- Pool only strings, and emit integer literals directly in instructions.

For v0.1 you may choose either approach; this spec assumes both strings and integers may appear in the constant pool.

---

## 5. Global Variables

Variables in Ambra v0.1 are compiled into **global slots**:

```text
global[0], global[1], ..., global[M-1]
```

The compiler:

- Builds a symbol table mapping identifiers (e.g., `user`, `i`) to integer slot indices.
- Emits `LOAD_GLOBAL` and `STORE_GLOBAL` instructions that refer to those indices.

Block scoping is enforced at compile time; the VM itself only sees a flat global array of slots.

---

## 6. Instruction Stream

The instruction stream is a linear sequence of bytecode instructions:

```text
instr[0], instr[1], ..., instr[K-1]
```

The IP (instruction pointer) starts at `0`. Each instruction updates IP to:

- `IP + size_of_instruction`, or
- A new value specified by a jump.

For conceptual clarity, we treat the instruction index as an integer label.  
Actual byte offsets depend on the implementation’s encoding.

---

## 7. Core Instruction Set (v0.1)

This section defines the **logical behavior** of each instruction class.  
Stack notation uses:

- `[...]` for the stack, with the **top** on the right.
- Example: `[a, b]` means `a` was pushed first, then `b`.

---

### 7.1 Stack Manipulation

#### `PUSH_INT n`

Pushes an integer literal onto the stack.

- **Operand:** integer `n` (literal or index into const pool, depending on implementation)
- **Stack before:** `[ ... ]`
- **Stack after:** `[ ..., Int(n) ]`

If integers are stored in the constant pool instead, this may be expressed as `LOAD_CONST k` with a constant of type `Int`.

#### `LOAD_CONST k`

Pushes a constant pool entry onto the stack.

- **Operand:** `k` — constant pool index
- **Stack before:** `[ ... ]`
- **Stack after:** `[ ..., const[k] ]`

The value `const[k]` may be a string, integer, or boolean.

---

### 7.2 Global Variable Access

#### `LOAD_GLOBAL i`

Loads a global variable onto the stack.

- **Operand:** `i` — global slot index
- **Semantics:** Reads `global[i]` and pushes it.
- **Stack before:** `[ ... ]`
- **Stack after:** `[ ..., global[i] ]`

If `global[i]` is uninitialized, behavior is an error (ideally caught at compile time).

#### `STORE_GLOBAL i`

Stores the top of the stack into a global variable.

- **Operand:** `i` — global slot index
- **Stack before:** `[ ..., v ]`
- **Stack after:** `[ ... ]`
- **Semantics:** `global[i] = v`

Typically used to implement:

- `summon x = expr;` — translate to store into a new slot.
- `x = expr;` — translate to store into an existing slot.

---

### 7.3 Arithmetic Operations

All arithmetic instructions:

- Pop two integers from the stack.
- Apply the operation.
- Push the result as an integer.

If the operand types are not integers, this is a runtime error.

#### `ADD`

- **Stack before:** `[ ..., Int(a), Int(b) ]`
- **Stack after:** `[ ..., Int(a + b) ]`

#### `SUB`

- **Stack before:** `[ ..., Int(a), Int(b) ]`
- **Stack after:** `[ ..., Int(a - b) ]`

#### `MUL`

- **Stack before:** `[ ..., Int(a), Int(b) ]`
- **Stack after:** `[ ..., Int(a * b) ]`

#### `DIV`

- **Stack before:** `[ ..., Int(a), Int(b) ]`
- **Stack after:** `[ ..., Int(a / b) ]`
- Division by zero is a runtime error.

---

### 7.4 Unary Operations

#### `NEG`

Numeric negation.

- **Stack before:** `[ ..., Int(a) ]`
- **Stack after:** `[ ..., Int(-a) ]`

#### `NOT`

Logical negation.

- **Stack before:** `[ ..., Bool(b) ]`
- **Stack after:** `[ ..., Bool(!b) ]`

If the operand is not a boolean, this is a runtime error.

---

### 7.5 Comparison Operations

Comparison instructions consume **two** operands and push a boolean.

All compare:

- Pop `right` then `left` (top-of-stack order)
- Push `Bool(result)`

#### `CMP_EQ`

- **Result:** `Bool(left == right)`

#### `CMP_NE`

- **Result:** `Bool(left != right)`

#### `CMP_LT`

- **Result:** `Bool(left < right)` (integers only in v0.1)

#### `CMP_LE`

- **Result:** `Bool(left <= right)`

#### `CMP_GT`

- **Result:** `Bool(left > right)`

#### `CMP_GE`

- **Result:** `Bool(left >= right)`

Type rules:

- In v0.1, comparisons are most naturally defined for integers.  
- An implementation may allow string equality (`CMP_EQ` / `CMP_NE`) but is not required to implement ordering (`<`, `>`) for strings.

---

### 7.6 Control Flow

Control flow uses **relative jumps** (from the current IP) or **absolute instruction indices**. Either is acceptable as long as compiler and VM agree.

#### `JUMP target`

Unconditional jump.

- **Operand:** `target` — instruction index or relative offset.
- **Stack:** unchanged.
- **Effect:** `IP = target`.

Used to:

- Skip over blocks (e.g., from the end of a `should` block to after all branches).

#### `JUMP_IF_FALSE target`

Conditional jump based on a boolean on the stack.

- **Operand:** `target` — instruction index or relative offset.
- **Stack before:** `[ ..., Bool(cond) ]`
- **Stack after:** `[ ... ]`
- **If** `cond` is `false`, then `IP = target`.
- **Else** continue to next instruction.

Used for:

- `should (condition) { ... }`
- `otherwise should (condition) { ... }`
- `aslongas (condition) { ... }` loops

All conditions in v0.1 must evaluate to boolean values; no truthiness.

---

### 7.7 String Operations and `say`

Ambra v0.1 defines only one **user-visible** string operation: printing via `say`.

String concatenation for interpolation is implemented at compile time using:

- `LOAD_CONST` for literal segments, and
- A combination of concatenation operations (if defined) or a “build string” pattern.

Two common strategies:

1. **Dedicated `CONCAT` instruction**
2. **`SAY` handles multiple values and concatenates at runtime**

For v0.1, we assume the second: the `SAY` instruction takes N arguments that are already on the stack.

#### `SAY n`

Prints one or more values with an implicit newline.

- **Operand:** `n` — number of arguments to print
- **Stack before:** `[ ..., v1, v2, ..., vN ]`
- **Stack after:** `[ ... ]`
- **Semantics:**
  - Convert each `v_i` to a string.
  - Concatenate them with a space (implementation choice).
  - Output the result followed by `\n`.

The compiler translates:

```text
say(expr1, expr2, ..., exprN);
```

into:

1. Evaluate each expression, pushing results on the stack in order.
2. Emit `SAY N`.

---

### 7.8 Program Termination

#### `HALT`

Indicates the end of the program.

- **Stack:** unchanged
- **Effect:** VM stops executing and returns control to the host.

The compiler should ensure there is exactly one reachable `HALT` at the end of the main instruction stream.

---

## 8. Example Translations (Conceptual)

This section describes **how high-level Ambra constructs map to bytecode**, conceptually.  
The exact instruction indices and constant pool layout are implementation choices.

> These are not exact encodings, but logical sequences to guide your compiler design.

---

### 8.1 Variable declaration

Source:

```ambra
summon x = 10;
```

Conceptual steps:

1. Load integer literal `10` (via `PUSH_INT` or `LOAD_CONST`).
2. Store into global slot assigned to `x` (e.g., index 0).

Bytecode sketch:

```text
PUSH_INT 10
STORE_GLOBAL 0    ; slot for x
```

---

### 8.2 Assignment

Source:

```ambra
x = x + 1;
```

Conceptual steps:

1. Load `x`.
2. Load integer literal `1`.
3. Add.
4. Store back to `x`.

Bytecode sketch:

```text
LOAD_GLOBAL 0     ; x
PUSH_INT 1
ADD
STORE_GLOBAL 0
```

---

### 8.3 `say` with multiple arguments

Source:

```ambra
say("Count:", i);
```

Conceptual steps:

1. Load constant string `"Count:"`.
2. Load value of `i`.
3. Emit `SAY 2`.

Bytecode sketch:

```text
LOAD_CONST k      ; "Count:"
LOAD_GLOBAL 1     ; i
SAY 2
```

---

### 8.4 `should` / `otherwise` chain

Source:

```ambra
should (x > 10) {
    say("Big");
}
otherwise should (x == 10) {
    say("Exact");
}
otherwise {
    say("Small");
}
```

Conceptual layout:

1. Evaluate `x > 10`.
2. If false, jump to second condition.
3. Emit `say("Big")`.
4. Jump to end.
5. Evaluate `x == 10`.
6. If false, jump to `otherwise` block.
7. Emit `say("Exact")`.
8. Jump to end.
9. Emit `say("Small")`.
10. End label.

Actual instruction indices are up to the compiler. The key idea is:

- each `should` and `otherwise should` uses a **conditional jump**  
- each taken branch uses an **unconditional jump** to skip remaining branches.

---

### 8.5 `aslongas` loop

Source:

```ambra
aslongas (i < 5) {
    say("i is", i);
    i = i + 1;
}
```

Conceptual layout:

1. Start label (loop entry).
2. Evaluate `i < 5`.
3. If false, jump to loop end.
4. Emit loop body.
5. Jump back to start label.

Bytecode sketch:

```text
LOOP_START:
    LOAD_GLOBAL 0         ; i
    PUSH_INT 5
    CMP_LT
    JUMP_IF_FALSE LOOP_END

    LOAD_CONST k          ; "i is"
    LOAD_GLOBAL 0         ; i
    SAY 2

    LOAD_GLOBAL 0
    PUSH_INT 1
    ADD
    STORE_GLOBAL 0

    JUMP LOOP_START
LOOP_END:
    ; continue
```

---

## 9. Bytecode File Layout

An Ambra v0.1 bytecode file conceptually consists of:

1. **Header**
   - Magic number (e.g., `"AMBRA"` in bytes)
   - Version (e.g., `0x0001`)
   - Sizes of constant pool and instruction stream

2. **Constant Pool Section**
   - Count of constants `N`
   - `N` serialized constant entries

3. **Instruction Section**
   - Count of instructions `K`
   - Encoded instructions

Example (abstract structure):

```text
[Magic]
[Version]
[ConstCount]
[Const0] [Const1] ... [ConstN-1]
[InstrCount]
[Instr0] [Instr1] ... [InstrK-1]
```

The **exact binary encoding** is implementation-specific:

- Opcodes might be single bytes.
- Operands might be 1, 2, or 4 bytes.
- Constants may be length-prefixed strings or fixed-size integers.

For experimentation, a text-based or JSON-based intermediate representation is also acceptable while prototyping.

---

## 10. Error Handling

The AVM should detect and report:

- Invalid opcodes
- Stack underflow / overflow
- Type errors (e.g., adding strings and integers)
- Division by zero
- Jump to invalid instruction index

How errors are surfaced (exit code, message, exceptions) is implementation-defined.

---

## 11. Future Extensions (Non-normative)

Later versions may extend the instruction set with:

- Function support (`CALL`, `RET`, `LOAD_LOCAL`, `STORE_LOCAL`)
- Heap-allocated arrays, maps, and their ops
- Logical `AND` / `OR` with short-circuit behavior
- Dedicated `CONCAT` for string concatenation
- Debugging / tracing instructions
- Optimizations (e.g., specialized constant instructions like `PUSH_INT_0`)

These are out of scope for v0.1 but have been anticipated in the current design.

---

End of **Ambra Bytecode Specification (v0.1)**.