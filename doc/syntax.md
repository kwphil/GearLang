# GearLang syntax guide

This syntax guide is built to give you an idea on how the language will feel. 

GearLang is still nowhere near a production-grade compiler, so expect some things to be incomplete or missing.

## Variables

Variables are declared using `let` 

Variable declarations either require and explicit type OR be initialized with an rvalue

This enforces explicivity for types.

```gr
let name: Type = value;
let name = value;

// Examples
let a: i32 = 3;
let b = 10; Implicit type

let x; // This is invalid here.
```

Variables are mutable by default, and can be created as immutable by using the `immut` modifier

```gr
immut let hi = 3;

hi = 4; // ERROR
```

---

## Types

<<<<<<< HEAD
The primitive type system tries to follow Rust's.

Examples:

```gr
f32   // Signed 32-bit float
uf32  // Unsigned 32-bit float
i8    // Signed 8-bit integer
u16   // Unsigned 16-bit integer 
```

### Pointers

Pointers follow the rules of raw C pointers. This includes:

* Array access
* Unchecked dereferencing

There are three symbols for using pointers

* `^` denotes a pointer TYPE
* `@` denotes a pointer dereference
* `#` denotes a value reference

Smart pointers will be established later with the stdlib, attempting to mimic Rust smart pointers.
=======
Gear types are simple and map closely to C types for interoperability.

| Gear Type | C Equivalent |
| --------- | ------------ |
| `char`    | `char`       |
| `i8`      | `char`       |
| `i16`     | `short`      |
| `i32`     | `int`        |
| `i64`     | `long`       |
| `f32`     | `float`      |
| `f64`     | `double`     |

**Notes:**

* Use explicit types for FFI for safety.
* Gear’s type system is inspired by Rust but simplified for C interop.

---

## Pointers

Pointers behave like C pointers. They can store addresses, be dereferenced, and passed to external functions.

```gear
// Pointer to a string (C-style string)
let s: char^ = "Hello, World!";

// Address-of operator
let i = 5;
let p: i32^ = #i;  // p now points to i

// Dereference operator
let j: i32 = @p;

// Pointer to pointer
let pp: i32^^ = #p;
let k: i32 = @@pp;
```

**Notes:**

* Use `^` to indicate a pointer type.
* Use `#` to get the address of a variable.
* Use `@` to dereference a pointer; multiple `@` operators can be chained for pointer-to-pointer types.
>>>>>>> master

---

## Structs

<<<<<<< HEAD
```gr
struct name {
  field_name Type;
};

struct foo {
  a i32;
  b foo^;
};
```

## Functions

Functions define reusable blocks of code.

```gr
fn func_name(field_name FieldType) -> Type {
  // ...
}

// Or no parameters:

fn func_name -> Type {
  // ...
}
```

### Entry Point Behavior

If a `main` function exists, then it will be called automatically AFTER ALL global scope instructions have been ran.

## Control flow

Control branches try to follow C-style syntax, with the exceptions of `for` and `match..case`

### Example

```gr
if (x > 0) {
  printf("Positive\n");
} else {
  printf("Non-positive\n");
}
```

---

## Global Scope

GearLang allows and is built for executable statements in the global scope.

* Useful for:
  * Initialization
  * Setup logic
  * Small glue scripts between languages

---

## FFI

GearLang is built on an extensive FFI. Currently only C is implemented, but to near full inclusion.

### C

The C ffi is enabled by default, and requires Clang to run as a hard dependency. 

GearLang supports C functions, variables, types, and typedefs in C.

You can also directly include C header functions.

```gr
// Defining a specific function
extern("C") fn puts(s char^);

// Defining a whole header
include("C", "sys|loc", "stdio.h");
```
=======
Structs are pretty generic here.

```gear
struct s {
  x i32;
  y i32;
}

let foo: s;

printf("%d", s.x);

```

**Notes:**

* Use `struct` to define the struct
* Syntax for parameters are `<name> <type>;`
* Structs in GearLang do not require a trailing comma
>>>>>>> master
