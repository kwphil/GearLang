# GearLang compiler: Type System

The type system is the semantic layer between parsing and code generation.

Types are represented by a single class:

```cpp
Sem::Type
```

Every expression, variable, function argument, and return value carries a `Sem::Type`.

---

## Design overview

The type system is:

* **value-based** (types are copied, not uniquely owned)
* **self-parsing** (types parse directly from a `Lexer::Stream`)
* **LLVM-aware** (types convert directly to `llvm::Type*`)
* **globally registered** (structs and unions are stored in static maps)

---

## Type categories

A `Sem::Type` represents exactly one of:

```text
Primitive     void, bool, char, iN, uN, fN
Record        struct or union (named or anonymous)
Array         array(T) or array(T, N)
Pointer       any of the above, wrapped in one or more ^
```

These are not subclasses — they are modes of a single object, determined by which internal fields are set.

---

## Primitive types

```cpp
enum class PrimType {
    Void, Bool, Char,
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    Invalid
}
```

### Notes

* `Char` and `I8` are semantically distinct but map to the same LLVM type (`i8`)
* `U*` types also map to the same LLVM integer types as their `I*` counterparts — signedness is tracked by GearLang, not LLVM
* `Invalid` is the zero state — an uninitialized or unparsable type

---

## Record types

Records are structs or unions.

```cpp
using Struct = vector<pair<string, shared_ptr<Type>>>;
```

Each field is a name-type pair.

### Storage

Records are stored in static maps, keyed by name:

```cpp
static unordered_map<string, Struct*> struct_list;
static unordered_map<string, Struct*> union_list;
```

A `Type` object holds a raw pointer into one of these maps:

```cpp
Struct* record_type;
string record_name;
bool record_is_struct;
```

`record_is_struct` distinguishes struct from union — both use the same `Struct` data type internally.

---

### Anonymous records

If no name is given, one is generated automatically:

```text
__GEAR_struct_anonymous_4
__GEAR_union_anonymous_6
```

Using a global counter:

```cpp
int anon_struct = 0;
```

---

## Array types

```gr
array(T)
array(T, N)
```

Stored as:

```cpp
shared_ptr<Type> array_type;
unsigned int array_size;  // -1 if unsized
```

Arrays are recursive — the element type is itself a full `Sem::Type`.

---

## Pointer types

Pointers are expressed with `^`:

```gr
i32^     // pointer to i32
i32^^    // pointer to pointer to i32
```

Stored as a depth counter:

```cpp
unsigned int pointer;
```

### Operations

```cpp
Type ref()    // wraps one more pointer level
Type deref()  // removes one pointer level
```

`ref()` works by re-serializing the type via `dump()` and re-parsing it. `deref()` directly decrements `pointer`.

---

## Type aliases

Aliases map a name to an existing type:

```cpp
static unordered_map<string, Type*> alias_list;

static void add_alias(string name, Type& ty);
```

During parsing, aliases are resolved before falling through to primitives.

---

## Parsing

Types are parsed directly from a token stream:

```cpp
Type::Type(Lexer::Stream& s)
```

This is called anywhere a type annotation appears in the grammar.

---

### Parse order

The constructor resolves types in this priority:

```text
1. array(...)
2. union { ... } or union Name
3. struct { ... } or struct Name
4. Known struct name      (struct_list lookup)
5. Known union name       (union_list lookup)
6. Known alias            (alias_list lookup)
7. Primitive keyword
8. Unknown → deferred     (unparsed cache)
```

After resolving the base type, pointer depth is parsed:

```cpp
while(s.peek()->type == Lexer::Type::Caret) {
    s.pop();
    pointer++;
}
```

---

### Inline record definitions

Structs and unions can be defined inline as type annotations:

```gr
struct {
    x i32;
    y i32;
}
```

The parser reads fields in a loop until `}`:

```text
field_name Type ;
```

Named inline structs are registered immediately into `struct_list` / `union_list`.

If a name is given but no `{` follows, the parser jumps to `find_parse` to look up the existing definition.

---

### Deferred types

If a type name is not recognized during parsing, it is cached for later resolution:

```cpp
unparsed_types[name] = { { this }, span };
```

Multiple uses of the same unknown type name are grouped:

```cpp
unparsed_types[name].first.push_back(this);
```

These are resolved after the FFI stage via:

```cpp
Type::parse_unparsed()
```

At that point, if the type is still unknown, a fatal error is emitted:

```cpp
Error::throw_error(span, "Unknown type", ErrorCodes::UNKNOWN_TYPE);
```

---

## Queries

The type system exposes a rich set of predicates:

```cpp
bool is_primitive()             // no pointer, no record, no array
bool is_underlying_primitive()  // no record, no array (pointer allowed)
bool is_struct()
bool is_union()
bool is_array()
bool is_pointer_ty()
bool is_float()
bool is_int()
```

### Pointer level

```cpp
int pointer_level()   // -1 if not a pointer, otherwise depth
```

### Bit classification

```cpp
PrimType bits_low_type()   // lowest type in the family (i8, f32)
int bit_level()            // 0 = 8-bit, 1 = 16-bit, 2 = 32-bit, ...
```

These are used during analysis and codegen to select LLVM operations by width.

---

### Struct field access

```cpp
int struct_parameter_index(string name)
Type struct_param_ty(int index)
```

`struct_parameter_index` returns `-1` if the field doesn't exist. It throws `std::logic_error` if called on a non-struct type.

---

### Compatibility

```cpp
bool is_compatible(Type& other)
bool is_compatible(Type&& other)
```

Two types are compatible if they are in the same family:

```text
pointer ↔ pointer
float   ↔ float
int     ↔ int
record  ↔ same record (pointer equality)
```

This is used during analysis to validate assignments and operations.

---

## LLVM conversion

Types convert to LLVM types for codegen:

```cpp
llvm::Type* to_llvm(Context& ctx) const
```

### Dispatch

```text
Primitive   → primitive_to_llvm
Pointer     → llvm::PointerType (opaque, address space 0)
Struct      → get_llvm_struct (cached lookup)
```

Note: all pointer types map to a single opaque LLVM pointer. GearLang tracks pointee type internally.

---

### Primitive mapping

```text
bool        → i1
char, i8, u8 → i8
i16, u16    → i16
i32, u32    → i32
i64, u64    → i64
f32         → float
f64         → double
void        → void
```

---

### Struct conversion

```cpp
static llvm::Type* struct_to_llvm(Type& obj, Context& ctx, string name)
```

Gathers all field types, calls `llvm::StructType::create`, and registers the result in:

```cpp
static unordered_map<string, llvm::StructType*> record_type_list;
```

Subsequent lookups use `get_llvm_struct(name)` rather than recreating the type.

---

### Underlying type

```cpp
llvm::Type* get_underlying_type(Context& ctx) const
```

Used when dereferencing a pointer or indexing an array — returns the type of the pointed-to or contained value.

---

## `dump()`

Serializes a type back to a GearLang type string:

```text
i32         → "i32"
i32^        → "i32^"
array(i32)  → "array(i32)"
array(i32,4)→ "array(i32,4)"
MyStruct    → "MyStruct"
```

Used for debug output and by `ref()` to construct pointer types.

---

## Cleanup

```cpp
static void clear_records()
```

Frees all heap-allocated `Struct` objects in `struct_list`, `union_list`, and `alias_list`. Must be called after compilation is complete.

---

## Invariants

The type system guarantees:

* A default-constructed `Type` has `prim_type == PrimType::Invalid`
* Pointer depth is always explicit — `pointer == 0` means non-pointer
* All named structs and unions are globally unique by name
* After `parse_unparsed()`, no unresolved type references remain
* Struct LLVM types are registered before any node references them

---

## Design notes

* Types are **value types** — copied freely, not reference-counted at the usage site. Record identity is preserved via raw pointer equality into the global maps.
* The `goto` in the constructor (`find_parse`, `pointer_parse`) is used to share the record lookup and pointer parsing paths across struct, union, and named-type cases.
* `ref()` round-trips through `dump()` and the lexer rather than manipulating `pointer` directly. This is simpler but means `ref()` always re-parses.
* Signedness (`u*` vs `i*`) is not represented at the LLVM level — the distinction only matters to GearLang's analyzer.

---

## Limitations

* No function pointer types
* No tuple types
* Unsigned/signed distinction is not enforced at codegen
* `clear_records()` uses raw `delete` — incompatible with arena allocation

---

## Future work

* Function pointer type support
* Separate signedness into a type flag rather than separate enum values
* Arena allocation for record storage
* Formal subtyping or coercion rules