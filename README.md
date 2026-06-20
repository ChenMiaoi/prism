# Prism

A mini C++ compiler designed for learning purposes, following LLVM's architecture patterns.

## Features

- **C++23 syntax and discipline** throughout Prism-owned code
- **LLVM-inspired architecture**: Type hierarchy, Value/User/Instruction, IRBuilder, Module
- **Prism-owned core library only**: no `std::` implementation shortcuts in compiler/core code
- **`prism::Expected`** for recoverable error handling
- **`prism::print`/`prism::format`/`raw_ostream`** instead of iostream or standard formatting APIs

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Usage

```bash
./prism input.cc -o output.s
./prism input.cc --emit-ir
```

## Testing

```bash
cd build && ctest
```

## Architecture

```
Source → Lexer → Parser → AST → Sema → IR → Backend → Assembly
```

### IR Layer (LLVM-inspired)
- `Context` - owns all IR objects, type uniquing
- `Type` hierarchy - IntegerType, FloatType, PointerType, FunctionType
- `Value` → `User` → `Instruction` with Use-Def chains
- `Module` → `Function` → `BasicBlock` → `Instruction`
- `IRBuilder` - builder pattern for instruction creation

## Prism Core Library Policy

Prism implementation code must not use `std::` standard-library facilities as shortcuts. If the compiler needs a standard-library-like facility, implement or extend the Prism-owned version:

- `prism::Vector<T>` instead of `std::vector`
- `prism::String` / `prism::StringView` instead of `std::string`
- `prism::Expected<T, Error>` instead of `std::expected`
- `prism::UniquePtr` / `prism::SharedPtr` instead of standard smart pointers
- Prism allocators/arenas instead of `std::allocator` and allocator-backed STL containers

## License

MIT License
