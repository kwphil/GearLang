# Gearlang

**The imperative programming language designed like a scripting language**

Build for clean, clear, and fast code.

This language uses `llvm` for binary creation

## Usage

Options will be added later on

```sh
./gearlang <input file>
```

## Examples

Examples are provided in prg/

Expect these examples to be a little messy for now, 
they're here to test the compiler.

Here's one of what to expect:

```gearlang
let hello = "Hello"
let msg = f"{hello}, World!"

fn main {
    printf("%s", msg); // Can access C functions
    
}
```

## License

This program follows rules under the MIT license