# Gearlang

**The imperative programming language designed like a scripting language**

Build for clean, clear, and fast code.

## Usage

```
Usage: gear [--help] [--version] [--output VAR] [--object] [--emit-llvm] [--verbose] [--version] [--dump-tokens] input

Gearlang compiler

Positional arguments:
  input            Input source file 

Optional arguments:
  -h, --help       shows help message and exits 
  -v, --version    prints version information and exits 
  -o, --output     Output executable file [nargs=0..1] [default: "a.out"]
  -c, --object     Emit object file 
  -S, --emit-llvm  Emit LLVM IR file 
  -v, --verbose    Enable verbose output 
  --version        Print version information 
  --dump-tokens    Print token output
  --dump-ast       Print AST output
```

## Examples

Examples are provided in prg/

Expect these examples to be a little messy for now, 
they're here to test the compiler.

These are being phased out for tests/, expect examples/ to continue

Here's one of what to expect:

```gearlang
let hello = "Hello"
let msg = f"{hello}, World!"

fn main {
    printf("%s", msg); // Can access C functions
}
```

## Building

```sh
cmake src
make -j$(nproc)
```

## Dependencies

* [llvm](github.com/llvm/llvm-project)
* [argparse](github.com/p-ranav/argparse)

## License

This program follows rules under the MIT license
