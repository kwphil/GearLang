# Syntax

GearLang adds a lot to the syntax, which while intuitive, needs to be clarified.

## Declarations

### Variables

A simple `let` indicates a declarations

```gear
// Initializing
let a = 0;
// No initializing
let a;
// Same thing with explicit types
let a: i32 = 0;
let a: i32;
```

### Functions

Functions are changed quite a bit

```
[extern] fn [ret type] <name> [: (<arg name> <arg type>, )]
```

The last comma is not required, but allowed

```gear
// Declaring a function
extern fn int foo: a i32;
// Defining a function
fn main {
    return a;
}
// OR
fn foo: a i32 => a;
```

## Types

Types are meant to be relatively simple, 
following a similar pattern to Rust

```
GearLang | C
char|i8  | char
i16      | short
i32      | int
i64      | long
f32      | float
f64      | double
```

## Pointers

Pointers can be defined with `^`. These functionally act as C pointers and can be directly passed into C functions

```gear
// C-str
let s: char^ = "Hello, World!";
```

You can grab an address of a variable with #

```gear
// Grabbing the address of another variable
let i = 5;
let j = #i;
```

And they can be dereferenced with `@`

```gear
let i: i32& = #0;
let j: i32&& = @i;
```
