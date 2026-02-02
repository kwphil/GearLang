This file is just dedicated to random thoughts I had about implementation

## C++ (will compile with the C++ code, will need to be linked somehow)

I'm thinking about having a config file (like Rust's Cargo.toml), it could probably link to a CMakeFiles or something

Just an example with toml:

```toml
[Cpp]
Build = "CMake"
Path = "cpp/src"
```

Or maybe if it uses Makefile it might require more info

```toml
[Cpp] # Require to compile within a path, maybe like cpp/.gearlang or something
Build = "Unix Makefiles"
Path = "cpp"
Args = "build"
```

As for implementation, it will probably create wrappers between the two

building a `.gearlang/cpp/wrappers.gear` and a `${cpp_path}/.gearlang/wrappers.cpp` to create C functions


