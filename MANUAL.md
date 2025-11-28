# 🧙‍♀️ Ambra Language Manual (v0.1)

Ambra is a small, whimsical programming language designed for learning compilers, exploring language design, and having fun writing expressive programs. It is playful, predictable, and intentionally limited in scope for the MVP so the compiler backend remains achievable and educational.

---

## 1. Introduction

Ambra embraces a magical, spell‑like personality. Programs read like small incantations, using expressive keywords such as `summon`, `should`, `otherwise`, and `aslongas`.

Ambra compiles to a custom bytecode format and runs on the Ambra Virtual Machine (AVM).

This manual describes **Ambra v0.1**, the minimum viable version of the language.

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
    say("Counting:", x);
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
Begins with `</` and ends with `/>`:

```
</
   Multi‑line comments can span
   any number of lines.
   Ambra ignores everything here.
/
>
```

Comments cannot nest in v0.1.

---

## 4. Variables

Variables are introduced using the `summon` keyword.

### Syntax

```
summon identifier = expression;
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

### Booleans  
Ambra uses expressive boolean literals:

- `affirmative` — true  
- `negative` — false  

### Strings  
Double‑quoted:

```
"Hello world"
```

#### Multi‑line strings

```
"""
This is a multi‑line
string inside Ambra.
"""
```

#### Interpolation

```
summon who = "Ambrose";
say("Hello, {who}");
```

Only simple identifier interpolation is allowed in v0.1.

---

## 6. Expressions

Ambra supports:

### Arithmetic  
```
+   -   *   /
```

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
    say("Count:", i);
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
- Automatically prints a newline  
- Converts all values to strings  

Example:

```
summon user = "Ambrose";
say("Welcome,", user, "!");
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
</ Ambra v0.1 Example Program />

summon user = "Ambrose";
summon excited = affirmative;

say("Hello,", user);

should (excited) {
    say("You seem excited today!");
}
otherwise should (user == "Flora") {
    say("Ah, Flora. A familiar presence.");
}
otherwise {
    say("Calm and collected.");
}

summon i = 0;

aslongas (i < 3) {
    say("Counting:", i);
    i = i + 1;
}

summon poem = """
Ambra is small,
Ambra is new,
Ambra is quirky,
And teaches you too.
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
- Arrays  
- Floats  
- Dictionaries  
- Modules  
- Pattern matching  
- Expression‑level interpolation  
- Improved runtime library  

These will be introduced incrementally.

---

## 14. Versioning

This document describes **Ambra v0.1 — MVP Syntax**.  
Future versions will build on this foundation.