# Ambra Language Manual

Ambra is a small, whimsical programming language designed for learning compilers, exploring language design, and having fun writing expressive programs.

---

## 1. Introduction

Ambra adopts a very simple and easy to read syntax. It uses expressive keywords such as `summon`, `should`, `otherwise`, `say` and `aslongas`.

Ambra compiles to a custom bytecode format and runs on the Ambra Virtual Machine (AVM).

---

## 2. Program Structure

An Ambra program consists of:

- **Statements** (terminated with semicolons `;`)
- **Blocks** using braces `{ ... }`
- **Expressions**
- **Comments** (single‑ and multi‑line)

Whitespace outside of strings has no syntactic meaning.

Example:

```
summon x = 10;
aslongas (x > 0) {
    say("Counting:" + x);
    x = x - 1;
}
```

---

## 3. Comments

Ambra uses custom comment delimiters.

### Single‑line comment  
Begins with `</` and ends at the end of the line:

```
</ This is a single‑line comment
summon x = 10;
```

### Multi‑line comment  
Begins with `</` followed immediately by a newline, and ends with `/>` on its own line:

```
</
Multi‑line comments can span
any number of lines.
Ambra ignores everything here.
/>
```

**Important:** The opening `</` must be followed by a newline, and the closing `/>` must be on its own line.

Comments cannot nest.

---

## 4. Variables

Variables are introduced using the `summon` keyword.

### Declaration Syntax

```
summon identifier = expression;
```

### Reassignment

Once a variable is declared, it can be reassigned without `summon`:

```
summon x = 10;
x = 20;          </ reassignment
x = x + 5;       </ allowed
```

### Examples

```
summon name = "Ambrose";
summon feeling = affirmative;
summon count = 42;
```

Identifiers may contain:

- letters  
- digits  
- underscores  

They must begin with a letter or underscore.

---

## 5. Types

Ambra v0.1 includes:

### Integers  
Whole numbers (e.g., `42`, `0`, `-3`).

Division (`/`) performs **integer division** (truncates toward zero).

```
summon x = 7 / 2;  </ x = 3
```

### Booleans  
Ambra uses expressive boolean literals:

- `affirmative` — true  
- `negative` — false  

### Strings  
Double‑quoted:

```
"Hello world"
```

Strings support escape sequences like `\n` (newline), `\t` (tab), `\\` (backslash), and `\"` (quote).

#### String interpolation

String interpolations are written with the format <{identifier}>.

```
summon name = Ambrose;
summon age = 15; </ works with ints
summon intro = "My name is {name}. I am {age} years old.";
say intro;
```

#### Multi‑line strings

```
"""
This is a multi‑line
string inside Ambra.
"""
```

Multi-line strings preserve newlines and indentation.

---

## 6. Expressions

Ambra supports:

### Arithmetic  
```
+   -   *   /
```

Arithmetic operators work on integers only (`-`, `*`, `/` require both operands to be integers).

### String Concatenation

The `+` operator also performs string concatenation:

```
summon greeting = "Hello " + "World";
summon msg = "Score: " + 42;  </ Automatically converts 42 to string
```

**Type coercion:** If either operand of `+` is a string, both are converted to strings and concatenated. If both operands are integers, `+` performs arithmetic addition.

Examples:
- `"Hello" + "World"` → `"HelloWorld"`
- `"Score: " + 95` → `"Score: 95"`
- `42 + " points"` → `"42 points"`
- `10 + 5` → `15` (arithmetic)

### Comparisons  
```
==  !=  <  >  <=  >=
```

### Unary operators  
```
not expr
```

Grouping with parentheses is allowed:

```
summon x = (1 + 2) * 3;
```

Ambra does **not** allow truthiness.  
Only `affirmative` or `negative` may appear as conditions in v0.1.

---

## 7. Control Flow

Ambra replaces typical control‑flow keywords with expressive, conversational ones.

---

### 7.1 Conditional Execution

#### `should` — replaces `if`

```
should (condition) {
    ...
}
```

Example:

```
should (x > 10) {
    say("Big number");
}
```

---

### 7.2 Else‑if

#### `otherwise should`

```
should (x > 10) {
    say("Big");
}
otherwise should (x == 10) {
    say("Exact");
}
```

---

### 7.3 Else

#### `otherwise`

```
otherwise {
    say("Small");
}
```

---

## 8. Loops

Ambra uses **`aslongas`** instead of `while`.

### Syntax

```
aslongas (condition) {
    ...
}
```

Example:

```
summon i = 0;

aslongas (i < 5) {
    say("Count: " + i + "\n");
    i = i + 1;
}
```

---

## 9. The `say` Statement

`say` is Ambra’s built‑in output function.

### Syntax

```
say(expr1, expr2, ...);
```

- Accepts one or more expressions  
- Prints each expression (no automatic spacing or newlines)
- Converts all values to strings  

To print a newline, use `"\n"`:

```
summon user = "Ambrose";
say("Welcome, ", user, "!\n");
```

Example:

```
say("Line 1\n");
say("Line 2\n");
```

---

## 10. Semicolons

All statements must end with `;`.

```
summon x = 10;
say(x);
```

---

## 11. Full Example Program

```
</
Ambra Example Program 
/>

summon user = "Ambrose";
summon excited = affirmative;

say("Hello, " + user + ".\n");

should (excited) {
    say("You seem excited today!\n");
}
otherwise should (user == "Dom") {
    say("Ah " + user + ". A familiar presence.\n");
}
otherwise {
    say("Calm and collected.\n");
}

summon i = 0;

aslongas (i < 3) {
    say("Counting: " + i + "\n");
    i = i + 1;
}

summon poem = """
Ambra is small,
Ambra is new,
Ambra is quirky.
""";

say(poem);
```

---

## 12. Reserved Keywords

```
summon
affirmative
negative
should
otherwise
aslongas
not
say
```

---

## 13. Future Directions

Planned upgrades for Ambra:

- Functions
