

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

## 2.3.1 AST Implementation Strategy (C++ Representation)

To ensure a consistent, maintainable, and extensible AST in the Ambra compiler, the following implementation rules apply:

### Representation Model
- The AST uses **abstract base classes** (`Expr`, `Stmt`) with **derived concrete node types** (e.g., `BinaryExpr`, `SummonStmt`).
- Each variant of an expression or statement is a distinct C++ class.

### Ownership Model
- The AST is a **strict tree**:
  - Every node exclusively owns its children.
  - There is no parent pointer and no sharing of subtrees.
  - Destroying the root `Program` node destroys the entire tree.
- Ownership is implemented conceptually as unique ownership; in code this will translate into exclusive (unique) pointers.

### Source Location Convention
- Every AST node stores a `SourceLoc` representing the **line and column of the first token** that introduces that node.
  - `SummonStmt` → location of the `summon` keyword.
  - `SayStmt` → location of the `say` keyword.
  - `IfChain` → location of the first `should` keyword.
  - `WhileStmt` → location of the `aslongas` keyword.
  - `BinaryExpr` → location of its left operand’s first token.
  - `UnaryExpr` → location of its operator token.
  - `VariableExpr` → identifier location.
  - Literal expressions → location of their literal token.
  - `InterpolatedStringExpr` → location of the opening quote of the string.

### Operator Storage
- Unary and binary operators are represented using the enums:
  - `UnaryOpKind`
  - `BinaryOpKind`
- The parser maps token kinds to these operator enums when constructing AST nodes.  
  Tokens themselves are **not** stored in the AST.

### Node Field Design
- Each AST class contains exactly the fields required to represent the semantic structure:
  - e.g., `BinaryExpr` contains `left`, `op`, `right`.
  - `InterpolatedStringExpr` contains a list of `StringPart` objects.
  - `BlockStmt` contains a list of owned `Stmt` nodes.
- No node contains token text except where necessary (e.g., identifier names, literal values).  
  No node contains raw lexer tokens.

### Stability Requirement
- After construction, the AST is immutable in structure:
  - Later phases (semantic analysis, codegen) may read the AST but must not modify topology.
  - This ensures predictable compilation, easier debugging, and safe visitor patterns.

This strategy defines a consistent and unambiguous contract for implementing all AST node classes in `expr.h` and `stmt.h`.

The Abstract Syntax Tree (AST) represents the *semantic* structure of an Ambra program.  
It is not token-based; it is a clean hierarchical model of statements and expressions.

---

# Core Node Categories

The AST has three main categories of nodes:

- **Program**
- **Statements**
- **Expressions**

Each AST node includes a `SourceLoc` used for error reporting.

---

# Program Node

```
Program {
  statements: [Stmt]
}
```

---

# Statements

Statements are represented as variants of the unified `Stmt` type.

```
Stmt
  ::= Summon {
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
        # Represents:
        #   should (cond1) { ... }
        #   otherwise should (cond2) { ... }
        #   otherwise { ... }

        branches: [
          (condition: Expr, body: Block)
        ]

        elseBranch: Block?   # null if absent

        loc: SourceLoc
      }

  ::= While {
        condition: Expr
        body: Block
        loc: SourceLoc
      }
```

### Notes

- The older “Should”, “ShouldOtherwise”, and “Otherwise” nodes are now unified into **IfChain**.
- `AslongAs` is normalized to **While**.
- `Not` is **not** a statement—it's part of the unary expression system.

---

# Expressions

Expressions compute values and appear inside statements.

```
Expr
  ::= IntLiteral {
        value: int
        loc: SourceLoc
      }

  ::= BoolLiteral {
        value: bool
        loc: SourceLoc
      }

  ::= StringLiteral {
        value: string
        loc: SourceLoc
      }
```

## Interpolated Strings

Ambra supports embedded expressions within string literals:

```
"hello {x + 1} world"
```

This is modeled as:

```
StringPart
  ::= TextChunk { text: string }
  ::= ExprPart { expression: Expr }

Expr
  ::= InterpolatedString {
        parts: [StringPart]
        loc: SourceLoc
      }
```

## Variable & Composite Expressions

```
Expr
  ::= Variable {
        name: IdentifierName
        loc: SourceLoc
      }

  ::= Unary {
        op: UnaryOpKind
        operand: Expr
        loc: SourceLoc
      }

  ::= Binary {
        left: Expr
        op: BinaryOpKind
        right: Expr
        loc: SourceLoc
      }

  ::= Grouping {
        expression: Expr
        loc: SourceLoc
      }
```

---

# Operator Variants

These correspond directly to supported language operators.

```
UnaryOpKind ::= 
      LogicalNot         # "not"
    | ArithmeticNegate   # "-" (optional extension)
```

```
BinaryOpKind ::=
      Equal              # "=="
    | NotEqual           # "!="
    | Less               # "<"
    | LessEqual          # "<="
    | Greater            # ">"
    | GreaterEqual       # ">="
    | Add                # "+"
    | Subtract           # "-"
    | Multiply           # "*"
    | Divide             # "/"
```

---

# Source Locations

Every AST node stores its originating position.

```
SourceLoc {
  line: int
  column: int
}
```

The AST must remain structurally stable after creation; later phases must not modify its shape.

---

## 2.4 Semantic Analysis — Name Resolution

**Input:** AST  
**Output:** Resolution side table + symbol table + diagnostics

This pass performs **name resolution and scope checking**. It determines which declaration each identifier refers to, reports invalid uses, and records metadata for later phases. The AST shape stays unchanged.

### Goals
- Enforce declaration‑before‑use
- Detect invalid identifier usage
- Build lexical scope information
- Allow shadowing, block redeclaration errors in the same scope
- Keep analysis non‑fatal (collect multiple errors)

### Scope Model
- Lexical (static) scoping
- New scope for: program, block `{...}`, `should` body, `otherwise should` body, `otherwise` body, `aslongas` body
- No new scope for condition expressions
- Each branch body gets its own scope

### Declarations ( `summon` )
- Inserts a variable symbol into the current scope
- Name must be unique within that scope
- Same‑scope redeclaration → error, keep original, ignore redeclared symbol
- Inner scopes may shadow outer names (no warning)

### Shadowing
- Allowed without warnings
- Lookups bind to nearest enclosing declaration

### Identifier Resolution
Lookup order for each identifier use:
1. Current scope
2. Enclosing scopes outward
3. Program/global scope

If not found: record an error, mark as unresolved, continue analysis to surface more issues.

### Symbol Representation
- Fields: name, kind (Variable), source location, owning scope
- Kind is explicit to support future kinds (functions, params, etc.)

### Error Policy
- Non‑fatal: continue after errors
- Error cases: undeclared identifier, same‑scope redeclaration
- Shadowing is allowed
- Expose `hadError` flag and diagnostics list

### Resolution Side Table
- Store results outside the AST
- Map: AST identifier node → resolved symbol (or unresolved marker)
- Benefits: immutable AST, clear metadata for later phases

### Outputs
- Hierarchy of symbol tables (scopes)
- Resolution side table
- Semantic error list
- Global `hadError` indicator

### Design Rationale
- Matches modern compiler architecture
- Prevents cascading failures while keeping AST stable
- Clean separation for later phases (type checking, control flow, codegen)

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
