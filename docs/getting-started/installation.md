# Installation

## Building from Source

Volta is currently available by building from source.

### Prerequisites

- C++ compiler with C++17 support (GCC 7+ or Clang 5+)
- Make
- Git

### Steps

1. Clone the repository:
```bash
git clone https://github.com/yourusername/volta.git
cd volta
```

2. Build the project:
```bash
make
```

3. The Volta interpreter will be available at `./bin/volta`

4. (Optional) Add to your PATH:
```bash
export PATH=$PATH:/path/to/volta/bin
```

## Verifying Installation

Run the Volta interpreter:
```bash
./bin/volta --version
```

You should see the Volta version information.
