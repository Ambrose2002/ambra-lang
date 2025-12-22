# Parser Design

## 1. Parser Goals & Scope

### Input and Output
- **Input:** `std::vector<Token>` produced by the lexer
- **Output:** Program AST (or an error report)

### Key Responsibilities

The parser should:

1. **Enforce Surface Grammar** – Ensure token sequences conform to Ambra's grammar rules
2. **Produce Structured AST** – Generate AST nodes matching `architecture.md`:
   - Program
   - Statement variants: Summon, Say, Block, IfChain, While
   - Expression variants: IntLiteral, BoolLiteral, StringLiteral, InterpolatedString, Variable, Unary, Binary, Grouping
3. **Report Clear Errors** – Provide syntax errors with accurate SourceLoc information from tokens
4. **Handle Recovery** – Stop cleanly on errors and optionally recover to report multiple errors

### Parsing Strategy

**Recursive descent with precedence-based expression parsing**

---

## 2. Grammar Overview (Informal EBNF)

This section defines what the parser accepts.

### 2.1 Program Structure

```ebnf
program     → statement* EOF ;
```

### 2.2 Statements

```ebnf
statement
    → summonStmt
    | sayStmt
    | ifChainStmt
    | whileStmt
    | block ;

summonStmt
    → "summon" IDENTIFIER "=" expression ";" ;

sayStmt
    → "say" expression ";" ;

whileStmt
    → "aslongas" "(" expression ")" block ;

block
    → "{" statement* "}" ;
```

### 2.3 Conditional Chains (if/otherwise)

If/otherwise chains are parsed into a single **IfChain** AST node:

```ebnf
ifChainStmt
    → "should" "(" expression ")" block
      ("otherwise" "should" "(" expression ")" block)*
      ("otherwise" block)? ;
```

**Rules:**
- The first `should` clause is mandatory
- Zero or more `otherwise should` clauses may follow
- An optional final `otherwise` block (no condition) may appear at the end

**AST Representation:**
- `branches`: Ordered list of `(condition, Block)` pairs
- `elseBranch`: Optional fallback `Block`

---

## 3. Expressions and Operator Precedence

Standard precedence hierarchy from lowest to highest binding:

```ebnf
expression  → equality ;

equality    → comparison ( ( "==" | "!=" ) comparison )* ;
comparison  → term       ( ( "<" | "<=" | ">" | ">=" ) term )* ;
term        → factor     ( ( "+" | "-" ) factor )* ;
factor      → unary      ( ( "*" | "/" ) unary )* ;

unary       → "not" unary
            | "-" unary
            | primary ;

primary     → INT_LITERAL
            | BOOL_LITERAL
            | STRING_TOKEN_SEQUENCE
            | IDENTIFIER
            | "(" expression ")" ;
```

### Operator Precedence Table

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 (lowest) | `==`, `!=` | Left |
| 2 | `<`, `<=`, `>`, `>=` | Left |
| 3 | `+`, `-` | Left |
| 4 | `*`, `/` | Left |
| 5 (highest) | `not`, `-` (unary) | Right |

**Note:** `STRING_TOKEN_SEQUENCE` is parsed into either `StringLiteral` or `InterpolatedString` depending on the presence of interpolation markers.

---

## 4. Parser Class Structure

### Member Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `tokens` | `std::vector<Token>` | Token stream from lexer |
| `current` | `int` | Index into tokens array |
| `hasError` | `bool` | Error flag for error handling |

### Utility Methods

| Method | Returns | Purpose |
|--------|---------|---------|
| `peek()` | `Token` | Get current token without consuming |
| `previous()` | `Token` | Get last consumed token |
| `isAtEnd()` | `bool` | Check if at EOF token |
| `advance()` | `Token` | Consume and return current token |
| `check(type)` | `bool` | Check type without consuming |
| `match(types...)` | `bool` | If current matches any type, consume and return true |
| `consume(type, msg)` | `Token` | Assert next token type or report error |

### Main Entry Points

| Method | Returns | Purpose |
|--------|---------|---------|
| `parseProgram()` | `Program` | Parse complete program |
| `parseStatement()` | `Stmt` | Parse a single statement |
| `parseExpression()` | `Expr` | Parse an expression |

### Helper Methods

| Method | Returns | Purpose |
|--------|---------|---------|
| `parseSummon()` | `Stmt` | Parse variable declaration |
| `parseSay()` | `Stmt` | Parse print statement |
| `parseBlock()` | `Stmt` | Parse code block |
| `parseIfChain()` | `Stmt` | Parse if/otherwise chain |
| `parseWhile()` | `Stmt` | Parse while loop |

### Expression Parsing Methods (Precedence Chain)

| Method | Parses | Precedence |
|--------|--------|-----------|
| `parseEquality()` | `==`, `!=` | 1 (lowest) |
| `parseComparison()` | `<`, `<=`, `>`, `>=` | 2 |
| `parseAddition()` | `+`, `-` | 3 |
| `parseMultiplication()` | `*`, `/` | 4 |
| `parseUnary()` | `not`, `-` | 5 |
| `parsePrimary()` | Literals, identifiers, grouping | 6 (highest) |

---

## 5. Parser Workflow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      PARSER EXECUTION                       │
└─────────────────────────────────────────────────────────────┘

                         ┌──────────────┐
                         │  Token List  │
                         │ from Lexer   │
                         └──────┬───────┘
                                │
                                ▼
                        ┌────────────────┐
                        │ parseProgram() │
                        └────────┬───────┘
                                │
                        ┌───────┴──────────┐
                        │ Until EOF_TOKEN  │
                        └───────┬──────────┘
                                │
                                ▼
                    ┌──────────────────────┐
                    │  parseStatement()    │
                    │  (dispatches to:)    │
                    ├──────────────────────┤
                    │ ├─ parseSummon()     │
                    │ ├─ parseSay()        │
                    │ ├─ parseIfChain()    │
                    │ ├─ parseWhile()      │
                    │ └─ parseBlock()      │
                    └──────────┬───────────┘
                               │
                    ┌──────────┴──────────┐
                    │ Append to           │
                    │ Program.statements  │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  Return Program AST  │
                    └──────────────────────┘
```

---

## 6. Expression Parsing Precedence Chain

```
                    ┌───────────────────┐
                    │ parseExpression() │
                    │  (entry point)    │
                    └──────────┬────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │ parseEquality()     │  ══   !=
                    │ (precedence 1)      │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │ parseComparison()   │  <   <=   >   >=
                    │ (precedence 2)      │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌───────────────────────┐
                    │ parseAddition()       │  +   -
                    │ (precedence 3)        │
                    └──────────┬────────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │ parseMultiplication()       │  *   /
                    │ (precedence 4)      │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │ parseUnary()        │  not  -
                    │ (precedence 5)      │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │ parsePrimary()      │
                    │ (precedence 6)      │
                    │                     │
                    │ Returns:            │
                    │ ├─ INT_LITERAL      │
                    │ ├─ BOOL_LITERAL     │
                    │ ├─ STRING           │
                    │ ├─ IDENTIFIER       │
                    │ └─ ( expression )   │
                    └─────────────────────┘
```

---

## 7. Statement Parsing Flowchart

```
                     ┌─────────────────────┐
                     │ parseStatement()    │
                     └──────────┬──────────┘
                                │
                    ┌───────────┼────────────┐
                    │           │            │
              Look at Token Type           │
                    │           │           │
        ┌───────────┼───────────┼───────────┼──────────┐
        │           │           │           │          │
        ▼           ▼           ▼           ▼          ▼
     SUMMON       SAY       SHOULD    ASLONGAS     LEFT_BRACE
        │           │           │           │          │
        ▼           ▼           ▼           ▼          ▼
   parseSummon  parseSay   parseIfChain parseWhile  parseBlock
        │           │           │           │          │
        └───────────┼───────────┼───────────┼──────────┘
                    │
                    ▼
           Return Stmt variant
```

---

## 8. Detailed Method Responsibilities

### 8.1 `parseProgram()`

**Behavior:**
1. Loop until `EOF_TOKEN` is encountered
2. Call `parseStatement()` for each statement
3. Append each statement to `Program.statements`
4. Return constructed `Program`

**Error Handling:**
- If `parseStatement()` reports an error, continue parsing to collect multiple errors
- Set error flag on fatal errors

### 8.2 `parseStatement()`

**Behavior:**
1. Examine `peek().type`
2. Dispatch to appropriate handler:
   - `SUMMON` → `parseSummon()`
   - `SAY` → `parseSay()`
   - `SHOULD` → `parseIfChain()`
   - `ASLONGAS` → `parseWhile()`
   - `LEFT_BRACE` → `parseBlock()`
3. Return constructed `Stmt` node

**Error:**
- Syntax error if token doesn't match any statement type

### 8.3 `parseSummon()`

**Token Pattern:** `summon IDENTIFIER = expression ;`

**Behavior:**
1. Consume `SUMMON` keyword
2. Consume and validate `IDENTIFIER`
3. Consume `EQUAL` operator
4. Call `parseExpression()` for initializer value
5. Consume `SEMI_COLON` terminator
6. Construct and return `Summon { name, initializer, location }`

**Location:** Set to `SUMMON` keyword position

### 8.4 `parseSay()`

**Token Pattern:** `say expression ;`

**Behavior:**
1. Consume `SAY` keyword
2. Call `parseExpression()` for output value
3. Consume `SEMI_COLON` terminator
4. Construct and return `Say { expression, location }`

**Location:** Set to `SAY` keyword position

### 8.5 `parseBlock()`

**Token Pattern:** `{ statement* }`

**Behavior:**
1. Consume `LEFT_BRACE` opening bracket
2. While current token is not `RIGHT_BRACE` and not `EOF_TOKEN`:
   - Call `parseStatement()`
   - Append result to statements list
3. Consume `RIGHT_BRACE` closing bracket
4. Construct and return `Block { statements, location }`

**Location:** Set to `LEFT_BRACE` position

### 8.6 `parseIfChain()`

**Token Pattern:**
```
should (cond1) { ... }
[otherwise should (cond2) { ... }]*
[otherwise { ... }]?
```

**Behavior:**

1. **First Branch (mandatory):**
   - Consume `SHOULD`
   - Consume `LEFT_PAREN`
   - Parse condition expression
   - Consume `RIGHT_PAREN`
   - Parse block
   - Add `(condition, block)` to `branches`

2. **Additional Branches (optional):**
   - While next tokens are `OTHERWISE` and `SHOULD`:
     - Consume `OTHERWISE`
     - Consume `SHOULD`
     - Parse `(condition)` and `block`
     - Append to `branches`

3. **Final Else (optional):**
   - If next token is `OTHERWISE` (without `SHOULD`):
     - Consume `OTHERWISE`
     - Parse `block` as `elseBranch`

**Construction:**
```cpp
IfChain {
  branches: [(cond1, block1), (cond2, block2), ...],
  elseBranch: Block?  // optional
  location: position of first 'should'
}
```

### 8.7 `parseWhile()`

**Token Pattern:** `aslongas ( condition ) { body }`

**Behavior:**
1. Consume `ASLONGAS`
2. Consume `LEFT_PAREN`
3. Call `parseExpression()` for condition
4. Consume `RIGHT_PAREN`
5. Call `parseBlock()` for body
6. Construct and return `While { condition, body, location }`

**Location:** Set to `ASLONGAS` keyword position

---

## 9. Expression Parsing Details

### 9.1 General Pattern

Each precedence level function:
- Parses one precedence level
- Implements **left-associative** binary operations
- Delegates to higher precedence (tighter binding)

**Pseudocode Template:**
```cpp
Expr parseLevel() {
    Expr left = parseHigherPrecedence();
    
    while (match(OPERATOR_1, OPERATOR_2, ...)) {
        Operator op = previous().type;
        Expr right = parseHigherPrecedence();
        left = Binary { op, left, right };
    }
    
    return left;
}
```

### 9.2 `parseUnary()`

**Handles:**
- `not unary` → Logical negation
- `-unary` → Numeric negation
- Otherwise delegates to `parsePrimary()`

**Construction:** `Unary { operator, operand }`

### 9.3 `parsePrimary()`

**Handles:**
- `INT_LITERAL` → `IntLiteral`
- `BOOL_LITERAL` → `BoolLiteral` (from `affirmative`/`negative`)
- `IDENTIFIER` → `Variable`
- `STRING` sequence → `StringLiteral` or `InterpolatedString`
- `( expression )` → `Grouping`

---

## 10. String Parsing (Special Case)

### 10.1 Simple vs Interpolated Strings

The lexer produces these tokens:
- `STRING` – Plain text chunk
- `INTERP_START` – Marks `{` inside string
- `INTERP_END` – Marks `}` closing interpolation

**Possible Patterns:**

1. **Plain String:** Single `STRING` token (no interpolation)
   ```
   "hello world"  →  STRING("hello world")
   ```

2. **Interpolated String:** Mix of `STRING` and interpolation blocks
   ```
   "x={x} y={y}"  →  STRING("x="), INTERP_START, x, INTERP_END, 
                      STRING(" y="), INTERP_START, y, INTERP_END
   ```

### 10.2 Parsing Algorithm

When `parsePrimary()` encounters a `STRING`:

1. **Collect string parts:**
   - Initialize `parts = []`
   - Add first `STRING` token as `TextChunk`
   - While next token is `INTERP_START`:
     - Consume `INTERP_START`
     - Parse embedded expression
     - Consume `INTERP_END`
     - Add `ExprPart { expression }` to parts
     - If another `STRING` follows, add it as `TextChunk`

2. **Decide node type:**
   - If `parts` contains only one `TextChunk`: Return `StringLiteral`
   - Otherwise: Return `InterpolatedString { parts }`

**Construction:**
```cpp
InterpolatedString {
  parts: [TextChunk/ExprPart...],
  location: position of first STRING token
}
```

### 10.3 String Parts Structure

```
InterpolatedString
├── parts: List[StringPart]
│   ├── TextChunk
│   │   └── value: string
│   │
│   ├── ExprPart
│   │   └── expression: Expr
│   │
│   ├── TextChunk
│   └── ... (alternating chunks/expressions)
│
└── location: SourceLocation
```

---

## 11. Error Recovery Strategy

### Basic Error Handling

1. **On Unexpected Token:**
   - Report error with token location
   - Skip to next reasonable synchronization point
   - Continue parsing

2. **Synchronization Points:**
   - Statement boundaries (semicolon, right brace)
   - New statement keywords (summon, say, should, aslongas)

3. **Error Flag:**
   - Set when any error is encountered
   - Checked before returning final `Program`
   - Allows collecting multiple errors in one pass

---

## 12. Summary: Parser Data Flow

```
┌──────────────────┐
│  Token Stream    │
│  from Lexer      │
└────────┬─────────┘
         │
         ▼
┌─────────────────────────────────────┐
│    Parser::parseProgram()           │
│  ┌──────────────────────────────┐   │
│  │ Loop: parseStatement()       │   │
│  │ ├─ parseSummon()            │   │
│  │ ├─ parseSay()               │   │
│  │ ├─ parseIfChain()           │   │
│  │ ├─ parseWhile()             │   │
│  │ └─ parseBlock()             │   │
│  │     └─ (recursive)          │   │
│  │                             │   │
│  │ For each expression:        │   │
│  │ └─ Precedence chain:        │   │
│  │    parseEquality()          │   │
│  │    └─ parseComparison()     │   │
│  │       └─ parseAddition()        │   │
│  │          └─ parseMultiplication()   │   │
│  │             └─ parseUnary() │   │
│  │                └─ parsePrimary()│
│  └──────────────────────────────┘   │
└────────┬─────────────────────────────┘
         │
         ▼
┌──────────────────┐
│   Program AST    │
│   or Error(s)    │
└──────────────────┘
```

---

## References

- `architecture.md` – AST node definitions
- `language_spec.md` – Grammar and language features
- `bytecode_spec.md` – Compilation targets
