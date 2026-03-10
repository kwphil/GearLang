# Gear Language Syntax

## Declarations

### Variables

Variables are declared using `let`. You must either provide an initial value or an explicit type.

```gear
// Inferred type with initialization
let a = 0;

// Explicit type without initialization
let b: i32;

// Explicit type with initialization
let c: i32 = 10;

// Invalid: must provide either a type or initial value
// let d;
```

**Notes:**

* Variables are immutable by default. (Consider `mut` qualifier if your language supports mutable variables.)
* Type inference works when an initial value is provided.

---

### Functions

Functions are declared with the `fn` keyword. They can optionally include qualifiers, arguments, and a return type.

**Function Header Syntax (EBNF-style):**

```ebnf
[<qualifiers>] "fn" <name> ['(' (<arg name> <arg type>) (',' <arg name> <arg type>)* ')'] [":" <return type>] ["{" <body> "}"]
```

**Qualifiers / Keywords**

| Keyword    | Description                                 |         |                                                                   |
| ---------- | ------------------------------------------- | ------- | ----------------------------------------------------------------- |
| extern("C | C++                                         | Gear") | Imports a function from another language using the specified ABI. |
| mangle("C | C++                                         | Gear") | Applies a name-mangling scheme for the target language.           |
| export   | Exports the function with external linkage. |         |                                                                   |

**Function Examples**

```gear
// Zero-arg functions
fn main {
    return 42;
}

fn main() {
    return 42;
}

// Function with arguments and explicit return type
fn add(a i32, b i32) : i32 {
    return a + b;
}

// Short arrow-style function (expression body)
fn square(x i32) => x * x;

// External function declaration
extern fn printf(s: char^) : i32;
```

**Notes:**

* Both `fn main` and `fn main()` are valid zero-arg function forms.
* Arrow-style (`=>`) can only be used for single-expression bodies.
* External functions (`extern`) can be imported from C, C++, or Gear using a compatible ABI.

---

## Types

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

---

## Structs

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
