

# Ambra Compiler & VM Architecture (v0.1)

This document describes the **high‑level architecture** for the Ambra compiler and the Ambra Virtual Machine (AVM).  
It guides implementation decisions while remaining language‑agnostic.  
No C++ code is included—only conceptual and structural explanations.

---

# 1. Overview

The Ambra toolchain has two major components:

1. **The Compiler** (`ambra-cc`)  
   - Converts Ambra source code into Ambra bytecode.

2. **The Ambra Virtual Machine (AVM)** (`ambra-vm`)  
   - Executes Ambra bytecode instructions.

The compilation pipeline is:

```
Source → Lexer → Parser → AST → Semantic Analysis → Bytecode Generation → Bytecode File
```

And the VM pipeline is:

```
Bytecode File → VM Loader → Execution Engine → Output
```

The system is intentionally simple for Ambra v0.1, but leaves room for growth.

---

# 2. Compiler Architecture

The Ambra compiler is organized in six distinct stages.  
Each stage is described below with responsibilities and considerations.

---

## 2.1 Lexer (Tokenization)

**Input:** Raw source text  
**Output:** A sequence of tokens (identifier, number, keyword, operator, punctuation, string literal, etc.)

### Responsibilities

- Scan characters and group them into meaningful tokens.
- Recognize:
  - identifiers (`summon`, `should`, etc.)
  - keywords
  - numbers
  - string literals (normal and triple-quoted)
  - interpolation markers (`{identifier}`)
  - comment delimiters (`</ ... />`)
  - punctuation and operators
- Track line/column positions for error reporting.

### Notes for implementation

- Treat triple‑quoted strings as multi‑character tokens.
- For interpolation, the lexer may:
  - produce a single *interpolated string token*, or
  - break it into literal + marker tokens  
  Either approach is valid for v0.1.
- Multi‑line comments must ignore everything until the closing `/>`.

---

## 2.2 Parser (Syntax Analysis)

**Input:** Token stream  
**Output:** Abstract Syntax Tree (AST)

### Responsibilities

- Validate that tokens follow the grammar from `LANGUAGE_SPEC.md`.
- Construct AST nodes for:
  - Program
  - Statements (`summon`, assignments, `say`, `should`, `otherwise`, `aslongas`)
  - Expressions (binary, unary, literals, identifiers)
  - Blocks
- Enforce structure rules (parentheses around conditions, semicolons after statements).

### Notes for implementation

- Use recursive‑descent or Pratt‑style parsing (your choice).
- Build a clean set of AST node classes.
- For `otherwise should`, the parser must treat it as part of the conditional chain.
- Treat `otherwise` as the “final else.”

---

## 2.3 AST Representation

The AST is a **tree of semantic constructs**, not tokens.

### Example core node categories

- **Program**
- **Block**
- **Statements**
  We'll model them as variants of a Stmt type

  ***Statement variants***
  Stmt 
  ::= VarDecl {
    name: IdentifierName
    initializer: Expr
    loc: SourceLoc
  }

  ::= Say {
    expression: Expr
    loc: SourceLoc
  }

  ::= Block {
    statements: [Stmt]
    loc: SourceLoc
  }

  ::= IfChain {
    ***should (cond1) {...}***
    ***otherwise should (cond2) {...}***
    ***otherwise{...}***
    branches: [(condition: Expr, body: Block)]
    elseBranch: Block?
    loc: SourceLoc
  }

  ::= While {
    condition: Expr
    body: Block
    loc: SourceLoc
  }
  - Assignment
  - Say
  - Should
  - ShouldOtherwise
  - Otherwise
  - AslongAs
  - Not
- **Expressions**
  - Binary
  - Unary
  - Literal (string/int/bool)
  - Identifier
  - InterpolatedString (optional intermediate form)

The AST should be *stable*— later phases should not modify it structurally.

---

## 2.4 Semantic Analysis

**Input:** AST  
**Output:** Annotated AST or validation status

### Responsibilities

- Resolve identifiers (declaration‑before‑use rule).
- Track scoped variables:
  - new variables appear in each block scope
  - shadowing rules (v0.1: allow or disallow—your choice)
- Verify:
  - conditions of `should`/`aslongas` evaluate to booleans
  - assignments are type‑consistent (int, string, bool)
  - interpolation references are valid identifiers
- Prepare any metadata needed by codegen (e.g., constant pools).

### Notes

- v0.1 type system is simple:  
  Integer, String, Boolean.
- Consider storing symbol tables as a stack of scopes.
- Errors caught here improve user experience.

---

## 2.5 Bytecode Generation

**Input:** AST  
**Output:** Bytecode module (instruction array + constants)

### Responsibilities

- Convert high‑level constructs into a linear sequence of bytecode instructions.
- Emit:
  - arithmetic ops
  - load/store variable ops
  - comparison ops
  - jumps for `should`, `otherwise`, `aslongas`
  - literal/string loading
- Handle string interpolation:
  - expand to concatenation ops, or
  - emit a special “build string” sequence

### Control flow strategy

- Translate `should (cond) { ... } otherwise { ... }` into:
  1. Evaluate condition
  2. Jump if false → next block
  3. Emit block
  4. Jump to end
- Similar for `otherwise should`.

### Notes

- v0.1 does not require a register machine; a simple **stack‑based VM** is recommended.
- Maintain a constant pool of:
  - strings
  - identifiers (optional)
  - integers

---

## 2.6 Bytecode File Format

The bytecode file contains:

```
Header (magic number + version)
Constant pool
Instruction stream
```

No relocations, symbols, or debugging info in v0.1.

A simple binary or JSON‑like format is fine.

---

# 3. Ambra Virtual Machine (AVM)

The AVM executes bytecode produced by the compiler.

---

## 3.1 VM Structure

The VM includes:

- **Instruction pointer (IP)**
- **Operand stack**
- **Call stack (for future versions)**
- **Heap for strings**
- **Global variable store**
- **Constant pool**

Even without functions, the stack‑based execution model is beneficial.

---

## 3.2 Execution Loop

The VM repeatedly:

1. Fetches the next instruction.
2. Decodes it.
3. Executes the corresponding operation.
4. Advances or updates the IP.

Stops when it reaches a `HALT` opcode.

This is the standard design used in Lua, Python, Wren, and many educational VMs.

---

## 3.3 Instruction Set (v0.1)

Typical opcodes include:

- **LOAD_CONST**  
- **LOAD_VAR / STORE_VAR**  
- **ADD / SUB / MUL / DIV**  
- **NEG / NOT**  
- **CMP_EQ / CMP_NE / CMP_LT / CMP_LE / CMP_GT / CMP_GE**  
- **JUMP / JUMP_IF_FALSE**  
- **SAY**  
- **HALT**

Exact numeric assignments are implementation‑defined.

---

## 3.4 Values and Types

The VM must support runtime values:

- Integer
- Boolean
- String

Strings may be reference-counted or garbage‑collected later; MVP may allocate them dynamically and never free (acceptable for now).

---

## 3.5 Global Variables

Because Ambra v0.1 has no functions, all variables are global or block‑scoped.  
Implementation choices:

- Map identifiers to integer slots
- Use a vector/array for fast LOAD/STORE
- Allow shadowing or disallow it (compiler-enforced)

---

## 3.6 Error Handling

Runtime errors include:

- Type mismatches (`affirmative + 3`)
- Undefined variable (should not occur if semantic analysis is correct)
- Illegal operations (divide by zero, etc.)

VM should report line info if available (optional in v0.1).

---

# 4. Future Extensions

Ambra’s architecture is deliberately flexible.

Planned features that fit naturally:

- Functions (add call frames, return stack)
- Arrays and dictionaries
- Modules and imports
- Full expression interpolation
- Classes or records
- Native functions in the VM
- JIT compilation (very later)

---

# 5. Summary

Ambra’s architecture is classical and educational:

- Lexer → Parser → AST → Semantic Analysis → Bytecode → VM
- Clean separation of stages
- Stack-based VM
- Simple constant pool
- Bytecode format suited for learning and extension

This architecture will remain stable as Ambra grows from v0.1 to future versions.
