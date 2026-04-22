# Modules

GearLang offers a Rust-like Module system for internally sharing files.

## Usage

math.gr

```gr
export fn add(lhs: i32, rhs: i32) returns i32 { /* ... */ }
```

main.gr

```gr
mod math;

fn main { return math.add(3, 4); }
```

# Submodules

GearLang also offers support for submodules. These can be defined in two ways:

## Directories

math/mod.gr

```
submod add;
```

math/add.gr

```
export fn foo() { puts("add"); }
```

main.gr

```
fn main() {
    math.add.foo();
}
```

## Inline submodules

math.gr

```gr
submod add {
    fn foo() { ... }
}

// Or just the declarations

submod add {
    fn foo();
}

fn add.foo() {

}

```