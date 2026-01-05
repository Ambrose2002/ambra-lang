# Ambra

A whimsical, educational programming language with a magical syntax.

## Quick Start

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler (clang, gcc, etc.)
- Git

### Installation

1. **Clone the repository:**
```bash
git clone https://github.com/Ambrose2002/ambra-lang.git
cd ambra_lang
```

2. **Build the project:**
```bash
mkdir -p build
cd build
cmake ..
make
```

3. **Install to your PATH:**
```bash
# Install to ~/.local/bin (recommended)
mkdir -p ~/.local/bin
cp bin/ambra ~/.local/bin/

# Add to PATH if not already there (add this to your ~/.zshrc or ~/.bashrc)
export PATH="$HOME/.local/bin:$PATH"

# Reload your shell configuration
source ~/.zshrc  # or source ~/.bashrc
```

Alternatively, you can use CMake's install target:
```bash
# From the build directory
cmake --install . --prefix ~/.local
```

### Running Ambra Programs

Once installed, you can run any `.ara` file from anywhere:

```bash
ambra myprogram.ara
```

### Example Program

Create a file called `hello.ara`:

```ambra
</This is a comment
summon message = "Hello, Ambra!\n";
say message;

summon x = 10;
summon y = 20;
say "The sum is: " + (x + y);

should (x < y) {
    say "\nx is less than y";
}
```

Run it:
```bash
ambra hello.ara
```

## Language Features

- **Variables:** Declare with `summon`
- **Output:** Print with `say`
- **Types:** Int, Bool, String with automatic type conversion
- **String Concatenation:** Use `+` to combine strings
- **Control Flow:** `should`/`otherwise` conditionals, `aslongas` loops
- **Booleans:** `affirmative` (true) and `negative` (false)
- **Comments:** Single-line `</` or multi-line `</ ... />`

## Documentation

- **[MANUAL.md](MANUAL.md)** - Complete language reference
- **[docs/architecture.md](docs/architecture.md)** - Compiler and VM architecture
- **[tests/sample_programs/](tests/sample_programs/)** - Example programs

## Running Tests

```bash
# From the build directory
ctest
# or
make test
```

## Project Structure

```
ambra_lang/
├── src/           # Compiler and VM source code
├── tests/         # Unit tests and sample programs
├── docs/          # Technical documentation
├── MANUAL.md      # User manual
└── README.md      # This file
```

## Contributing

Contributions welcome! Please read the documentation to understand the language design before submitting PRs.