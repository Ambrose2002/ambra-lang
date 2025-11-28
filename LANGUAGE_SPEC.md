

# Ambra Language Specification (v0.1)

This document defines the **formal syntax and lexical structure** of the Ambra programming language, version 0.1. It is intended as a reference for implementing the Ambra compiler and tools.

This specification covers:

- Lexical structure (tokens)
- Grammar (EBNF-style)
- Operator precedence and associativity
- Static constraints for the v0.1 MVP

Semantics (runtime behavior) are described informally where helpful but are not fully formalized here.

---

## 1. Lexical Structure

### 1.1 Character Set

Ambra source files are sequences of Unicode characters, but v0.1 treats the source as if it were ASCII for purposes of identifiers, operators, and literals.

Only the following are significant for tokens:

- Letters: `A–Z`, `a–z`
- Digits: `0–9`
- Underscore: `_`
- Punctuation and symbols used by the language:  
  `+ - * / = ! < > ( ) { } " ; , { } / < >`
- Whitespace characters (space, tab, newline, carriage return)

Other characters should be rejected or reserved for future use.

---

### 1.2 Whitespace and Newlines

Whitespace (spaces, tabs, newlines, carriage returns) is ignored **except** when it serves to separate tokens or appears inside string literals.

There is no semantic significance to indentation in Ambra v0.1.

---

### 1.3 Comments

Ambra defines two kinds of comments.

#### 1.3.1 Single-line comments

A single-line comment begins with the two-character sequence:

```
</
```

and continues to the end of the current line. The `</` sequence itself must not appear inside a string literal if it is intended to start a comment.

Example:

```
</ This is a single-line comment
summon x = 10;
```

Everything from `</` to the end of the line is ignored by the lexer.

#### 1.3.2 Multi-line comments

A multi-line comment begins with `</` and ends with `/>`. Everything between those delimiters is ignored.

Example:

```
</
   This is a multi-line comment.
   The compiler ignores all of this.
/>
summon x = 10;
```

Multi-line comments:

- May span multiple lines
- **Do not nest** in v0.1 (a `</` inside a multi-line comment is treated as ordinary text)

---

### 1.4 Identifiers

An **identifier** is a sequence of characters that:

- Begins with a letter (`A–Z`, `a–z`) or underscore (`_`)
- Is followed by zero or more letters, digits, or underscores

Formally:

```text
IdentifierStart  ::= Letter | "_"
IdentifierPart   ::= Letter | Digit | "_"
Identifier       ::= IdentifierStart IdentifierPart*
```

Identifiers are case-sensitive. For example, `name`, `Name`, and `NAME` are distinct.

Certain identifiers are reserved as **keywords** and cannot be used as user-defined identifiers (see §1.5).

---

### 1.5 Keywords and Reserved Words

The following identifiers are reserved as **keywords** in Ambra v0.1 and cannot be used as variable names or other identifiers:

```text
summon
affirmative
negative
should
otherwise
aslongas
not
say
```

Future versions of Ambra may introduce additional keywords; implementations should treat unknown keywords as ordinary identifiers unless otherwise specified.

---

### 1.6 Literals

Ambra v0.1 defines three literal categories: integer literals, boolean literals, and string literals.

#### 1.6.1 Integer literals

An **integer literal** is a non-empty sequence of decimal digits, optionally preceded by a minus sign (`-`):

```text
IntegerLiteral ::= "-"? Digit+
Digit          ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
```

Examples:

- `0`
- `42`
- `-7`

Only base-10 integers are supported in v0.1.

#### 1.6.2 Boolean literals

Ambra uses the following literals for boolean values:

- `affirmative` — represents true
- `negative` — represents false

These are reserved keywords (§1.5).

#### 1.6.3 String literals

Ambra provides two forms of string literals:

1. **Simple double-quoted strings**
2. **Triple-quoted multi-line strings**

##### 1.6.3.1 Simple double-quoted strings

A simple string literal is delimited by double quotes (`"`):

```text
StringLiteralSimple ::= '"' StringCharacter* '"'
```

Where `StringCharacter` is any character except:

- an unescaped double quote `"`
- a newline
- the end of file

Ambra v0.1 does not fully specify escape sequences; implementations may start by:

- Treating `\"` as an escaped double quote
- Treating `\\` as an escaped backslash

Other backslash sequences may be undefined in v0.1.

##### 1.6.3.2 Triple-quoted multi-line strings

A multi-line string is introduced by three double quotes in a row:

```text
StringLiteralMulti ::= '"""' MultiStringCharacter* '"""'
```

`MultiStringCharacter` may include newlines and any characters except the terminating `"""` sequence.

Example:

```
summon poem = """
Ambra is small,
Ambra is new,
Ambra is quirky,
And teaches you too.
""";
```

The content between the opening and closing `"""` is taken as-is (excluding the delimiters).

##### 1.6.3.3 String interpolation

Ambra supports **simple interpolation** inside simple double-quoted strings.

Within a simple string literal, a substring of the form:

```text
" ... {identifier} ..."
```

is treated as a string with an embedded variable reference.

For v0.1:

- Only `{identifier}` is allowed (no arbitrary expressions, no nesting).
- The lexer should recognize `{` and `}` as delimiters that may require special handling by the parser or a separate interpolation pass.

Example:

```
summon name = "Ambrose";
say("Hello, {name}");
```

This is semantically equivalent to printing the concatenation of `"Hello, "` and the value of `name`, but the exact implementation strategy is left to the compiler.

---

### 1.7 Operators and Punctuation

Ambra uses the following operators and punctuation tokens:

**Arithmetic operators:**

```text
+   -   *   /
```

**Comparison operators:**

```text
==  !=  <  >  <=  >=
```

**Unary logical operator:**

```text
not
```

**Assignment operator:**

```text
=
```

**Punctuation:**

```text
(   )   {   }   ,   ;
```

These tokens form the building blocks for expressions and statements defined in the grammar.

---

## 2. Grammar (EBNF)

This section specifies the syntax of Ambra using an EBNF-like notation.

### 2.1 Notation

- `A ::= B` means *A is defined as B*.
- `A?` means *A is optional* (0 or 1 occurrences).
- `A*` means *zero or more occurrences of A*.
- `A+` means *one or more occurrences of A*.
- `A | B` means *A or B*.
- Terminals are written as quoted literals or as named token categories (e.g., `Identifier`).

The grammar is specified in terms of **parser-level constructs**, assuming a prior lexical analysis phase that has already categorized tokens.

---

### 2.2 Top-level structure

```ebnf
Program          ::= StatementList EOF ;

StatementList    ::= Statement* ;
```

---

### 2.3 Statements

Ambra v0.1 supports the following kinds of statements:

- Variable declarations
- Assignments
- `say` statements
- Conditional statements
- Loop statements
- Block statements (used within control flow)

Each statement ends with a semicolon **except** block-structured forms whose body is a brace-delimited block (the outer control-flow statement ends with `}` rather than `;`).

#### 2.3.1 General statement form

```ebnf
Statement        ::= VarDeclStmt
                   | AssignmentStmt
                   | SayStmt
                   | IfStmt
                   | WhileStmt
                   ;
```

Blocks are not standalone statements at the top level in this grammar; they appear inside control-flow constructs.

---

### 2.4 Variable Declarations and Assignments

#### 2.4.1 Variable declaration

```ebnf
VarDeclStmt      ::= "summon" Identifier "=" Expression ";" ;
```

Example:

```
summon x = 10;
summon name = "Ambrose";
```

#### 2.4.2 Assignment

Re-assignment uses the same assignment operator but **without** `summon`:

```ebnf
AssignmentStmt   ::= Identifier "=" Expression ";" ;
```

Example:

```
x = x + 1;
```

---

### 2.5 `say` Statement

`say` prints one or more expressions and appends a newline.

```ebnf
SayStmt          ::= "say" "(" ArgList? ")" ";" ;

ArgList          ::= Expression ("," Expression)* ;
```

Examples:

```
say("Hello");
say("Hello,", name, "!");
```

---

### 2.6 Blocks

A **block** is a sequence of statements delimited by braces:

```ebnf
Block            ::= "{" StatementList "}" ;
```

Blocks introduce their own lexical scope for variables (semantics beyond the grammar).

---

### 2.7 Conditionals

Ambra uses `should`, `otherwise should`, and `otherwise` for conditionals.

```ebnf
IfStmt           ::= "should" "(" Expression ")" Block ElseClause? ;

ElseClause       ::= "otherwise should" "(" Expression ")" Block ElseClause?
                   | "otherwise" Block
                   ;
```

This allows chains of the form:

```
should (cond1) {
    ...
}
otherwise should (cond2) {
    ...
}
otherwise {
    ...
}
```

There may be zero or more `otherwise should` branches and at most one final `otherwise`.

---

### 2.8 Loops

Ambra uses `aslongas` instead of `while`.

```ebnf
WhileStmt        ::= "aslongas" "(" Expression ")" Block ;
```

Example:

```
aslongas (i < 10) {
    say("i is", i);
    i = i + 1;
}
```

---

### 2.9 Expressions

Expressions in Ambra include:

- Literals
- Variable references
- Arithmetic operations
- Comparison operations
- Unary logical negation
- Parenthesized expressions
- String interpolation (handled at the lexical and/or semantic layer)

The expression grammar is defined with explicit precedence levels.

We define the following precedence (from **highest** to **lowest**):

1. Primary expressions (literals, identifiers, parenthesized)
2. Unary `not` and unary `-`
3. Multiplicative `* /`
4. Additive `+ -`
5. Comparison `< > <= >=`
6. Equality `== !=`

All binary operators are left-associative in v0.1.

#### 2.9.1 Expression entry point

```ebnf
Expression       ::= EqualityExpr ;
```

#### 2.9.2 Equality

```ebnf
EqualityExpr     ::= ComparisonExpr ( ( "==" | "!=" ) ComparisonExpr )* ;
```

#### 2.9.3 Comparison

```ebnf
ComparisonExpr   ::= AddExpr ( ( "<" | ">" | "<=" | ">=" ) AddExpr )* ;
```

#### 2.9.4 Addition and subtraction

```ebnf
AddExpr          ::= MulExpr ( ( "+" | "-" ) MulExpr )* ;
```

#### 2.9.5 Multiplication and division

```ebnf
MulExpr          ::= UnaryExpr ( ( "*" | "/" ) UnaryExpr )* ;
```

#### 2.9.6 Unary

Unary expressions support logical negation and numeric negation.

```ebnf
UnaryExpr        ::= "not" UnaryExpr
                   | "-" UnaryExpr
                   | PrimaryExpr
                   ;
```

#### 2.9.7 Primary expressions

```ebnf
PrimaryExpr      ::= IntegerLiteral
                   | StringLiteralSimple
                   | StringLiteralMulti
                   | BooleanLiteral
                   | Identifier
                   | "(" Expression ")"
                   ;
```

Where:

```ebnf
BooleanLiteral   ::= "affirmative" | "negative" ;
```

String literals include both simple and multi-line forms (§1.6.3). Interpolation is treated as a property of string literals rather than a separate grammar production in v0.1.

---

## 3. Static Constraints (v0.1)

Beyond the syntactic grammar, Ambra v0.1 imposes several basic static rules.

1. **Variable declaration before use**  
   - Every identifier used as a variable in an expression or assignment must have been introduced by a `summon` statement in an accessible scope.

2. **Condition types**  
   - The condition expressions in `should` and `aslongas` must evaluate to a boolean value.  
   - In v0.1, this means they should ultimately be derived from `affirmative`, `negative`, or comparison expressions returning booleans.

3. **Assignment compatibility**  
   - Assignments `Identifier = Expression;` should assign values of a type compatible with the variable's previous value (type system details are intentionally minimal in v0.1, but an implementation may enforce simple type consistency: integers with integers, strings with strings, booleans with booleans).

4. **No truthiness**  
   - Integers and strings are **not** implicitly coerced to booleans. Using them directly in conditional positions is a static error in v0.1.

5. **String interpolation identifiers**  
   - In `"Hello, {name}"`, the `{name}` must be a valid identifier that resolves to a variable in scope.  
   - Arbitrary expressions in `{ ... }` are not supported in v0.1.

6. **Reserved keywords**  
   - Keywords listed in §1.5 cannot be redefined or used as variable names.

---

## 4. Future Extensions (Non-normative)

Future versions of Ambra may extend this specification to include:

- Function declarations and calls
- Arrays and collection literals
- Floating-point literals
- Logical `and` / `or` operators
- More sophisticated string interpolation (full expressions inside `{ ... }`)
- User-defined types and pattern matching

These are out of scope for v0.1 but the current design aims to make such extensions natural.

---

End of **Ambra Language Specification (v0.1)**.