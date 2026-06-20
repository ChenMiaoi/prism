# Prism C++23 Compiler Roadmap

> Prism is a teaching-oriented mini full C++ compiler, written in modern C++23 and intended to support the C++23 standard, using LLVM/Clang architecture patterns, GCC/G++ lowering and ABI lessons, Folly-style low-level utility discipline, and intentionally applying “keep the strengths, discard the baggage” by excluding obsolete C++ syntax and library choices instead of preserving them for compatibility.

## 1. Current Baseline

The current tree already has useful compiler-shaped modules, but the frontend path is not wired from source text to a real AST.

| Module | Paths | Owns | Current state |
|---|---|---|---|
| `basic` | `prism/include/prism/basic`, `prism/src/basic` | `DiagnosticsEngine`, `Diagnostic`, `DiagLevel`, `Error`, `SourceLocation` | Current diagnostics already use Prism string/vector-style storage. |
| `core` | `prism/include/prism/core` | `prism::String`, `StringView`, `Vector`, `HashMap`, `Optional`, `Expected`, `UniquePtr`, `SharedPtr`, `Function`, `BumpAllocator`, `format/print`, `raw_ostream` | Folly/LLVM-inspired utility layer. |
| `ast` | `prism/include/prism/ast`, `prism/src/ast` | `ASTContext`, `DeclKind`, `ExprKind`, `StmtKind`, `TranslationUnitDecl`, `FunctionDecl`, `BinaryExpr`, `CallExpr`, `BlockStmt` | AST vocabulary exists for teaching source structure. |
| `sema` | `prism/include/prism/sema`, `prism/src/sema` | `sema::TypeInfo`, `SemanticAnalyzer::analyze`, statement/expression/type checks | Currently small and stringly typed. |
| `ir` | `prism/include/prism/ir`, `prism/src/ir` | LLVM-like `Context`, `Type`, `Value`, `User`, `Use`, `Instruction`, `Module`, `Function`, `BasicBlock`, `IRBuilder`, `IRPrinter` | IR has LLVM-style teaching vocabulary. |
| `codegen` | `prism/include/prism/codegen`, `prism/src/codegen` | `ASTLowering`, `x86_64::X86_64CodeGenerator` | Current backend is textual and narrow. |
| `driver` | `prism/include/prism/driver/compiler.hpp`, `prism/src/driver/compiler.cc` | `Compiler::compile`, `Compiler::compile_source`, `emit_ir` | `Compiler::compile_source` is a placeholder and does not invoke parser/sema/lowering/backend. |
| `tests` | `prism/tests/unit/test_all.cc`, `prism/tests/examples` | IR, sema, codegen, and example smoke checks | Current tests manually exercise IR, sema, codegen, and a narrow full-pipeline smoke path. |

Confirmed gaps:

- No real lexer/token layer.
- No parser, preprocessor, or source manager.
- No real source-to-AST path.
- No optimizer/pass manager.
- No LLVM backend source was found.
- RISC-V is option-referenced but not present in inspected sources.
- README now states the `prism::Expected`/`prism::` utility layer and the no-`std::` implementation rule; existing source still contains `std::` technical debt to migrate into Prism-owned core facilities.

## 2. Target Architecture

Prism should grow as a transparent compiler pipeline whose intermediate forms can be inspected by readers.

```text
Source files
  → SourceManager / FileManager
  → Lexer / modern diagnostics for rejected legacy tokens
  → Parser / recursive-descent C++ subset
  → AST / ASTContext-owned nodes
  → Sema / scopes, types, value categories, overload sets
  → IRGen / ASTLowering
  → Prism IR / SSA, CFG, verifier
  → PassManager / analyses and optimizations
  → Backend / x86-64 first, LLVM IR optional, RISC-V later
  → Assembly or object output
```

Ownership rule: long-lived compiler objects should be arena/context-owned (`ASTContext`, `ir::Context`, later `BumpAllocator` where appropriate); transient results should return `prism::Expected<T, prism::Error>` or `prism::Expected<void, prism::Error>` instead of exceptions.

Dependency rule: LLVM, GCC, and Folly are references, not dependencies by default; only the optional LLVM backend may require LLVM when `PRISM_ENABLE_LLVM` is ON.

## 3. C++23 and Library Policy

Prism is not a compatibility museum and not a standard-library wrapper. It is a full Prism implementation: every facility used by compiler code must either be a Prism facility or a deliberately isolated third-party/reference boundary.

Prism is a C++23 compiler implemented in C++23, but Prism source code must not use `std::` standard-library facilities as an implementation shortcut. New code should be aggressively modern C++ in syntax and discipline: explicit ownership, value semantics, concepts where they clarify contracts, `constexpr`/`consteval` where they remove runtime work, structured bindings, designated initialization where appropriate, and `prism::Expected`-style error flow. Do not write old C++03/C++11 style, and do not hide behind `std::vector`, `std::string`, `std::unique_ptr`, `std::allocator`, `std::map`, `std::unordered_map`, `std::optional`, `std::function`, `std::format`, `std::print`, `std::iostream`, or related standard-library types in Prism implementation code.

If Prism needs a facility that resembles a standard-library component, implement the Prism version in the core layer after studying the corresponding design: e.g. use/extend `prism::UniquePtr` instead of `std::unique_ptr`, `prism::Vector` instead of `std::vector`, `prism::String`/`StringView` instead of `std::string`/`std::string_view`, `prism::Expected` instead of `std::expected`, and Prism allocators/arenas instead of `std::allocator` or allocator-aware STL containers. External projects such as LLVM, GCC, Folly, fmt, libc++, and libstdc++ are references for tradeoffs, not dependency surfaces and not code to copy.

| Status | Facility or rule | Policy |
|---|---|---|
| Approved | `prism::String` / `prism::StringView` | Use for owned/non-owned strings. The final design target is a better teaching string inspired by Folly `fbstring`, LLVM `SmallString`/`StringRef`, libc++/libstdc++ SSO lessons, and Prism’s existing `prism::String`; do not clone or use `std::string` in Prism implementation code. |
| Approved | `prism::Vector<T>` | Use for compiler-owned contiguous sequences; do not use `std::vector` in Prism implementation code. |
| Approved | `prism::HashMap<K,V>` and future `prism::ScopedHashTable<T>` | Use for maps and scopes; any scoped table must be implemented on Prism containers, not `std::unordered_map`, `std::map`, or other STL storage. |
| Approved | `prism::Expected<T, Error>` / `prism::Unexpected<Error>` | Use for recoverable failures; do not use `std::expected`, exceptions, or exception-like control flow for normal compiler errors. |
| Approved | `prism::format`, `prism::print`, `prism::println`, `prism::raw_ostream` | Use for output. fmt and C++23 print semantics are design references only; Prism must not depend on `std::format`, `std::print`, or `iostream` as implementation surfaces. |
| Approved | `prism::UniquePtr`, `prism::SharedPtr`, `BumpAllocator`, future Prism arena/allocator APIs | Use for ownership and allocation. If a new allocation pattern is needed, implement it as a Prism allocator/arena instead of using `std::allocator` or allocator-aware STL containers. |
| Forbidden | `std::` in Prism implementation code | No `std::unique_ptr`, `std::shared_ptr`, `std::vector`, `std::string`, `std::optional`, `std::function`, `std::map`, `std::unordered_map`, `std::allocator`, `std::format`, `std::print`, `std::iostream`, or equivalent STL implementation shortcut in Prism-owned compiler/core code. |
| Forbidden | Primary `iostream`/`istream`/`ostream` Prism library surface | Do not implement stream APIs as the main surface; if host interop is temporarily unavoidable, keep it isolated behind Prism APIs and schedule removal. |
| Forbidden | Legacy C++ compatibility baggage | Exclude `std::auto_ptr`, `std::bind1st`/`bind2nd`, `std::mem_fun`, dynamic exception specifications, trigraph/digraph teaching support beyond lexer rejection diagnostics, old `export` template semantics, C++98/03 compatibility modes, broad vendor compatibility quirks, and any broad standard-library clone before the compiler pipeline works. |

Existing `std::` occurrences are technical debt to remove, not accepted precedent. Each migration should replace the use with a Prism-owned facility and keep the implementation teachable.

## 4. Milestone Roadmap

Milestones are ordered by compiler dependency, not by feature glamour.

| Milestone | Scope | Acceptance check |
|---|---|---|
| P0 Standalone English and Chinese docs and baseline | Produce standalone English and Chinese roadmap documents; keep README aligned with `prism::Expected`/`prism::` naming and the no-`std::` implementation rule whenever docs change. | `prism/docs/compiler-roadmap.en.md` and `prism/docs/compiler-roadmap.zh.md` exist, contain no web-page frontmatter/sidebar assumptions, and state the “keep the strengths, discard the baggage” / `去其糟粕，取其精华` library policy plus the full-Prism no-`std::` rule. |
| P1 Source + Lexer | Add `SourceManager`, `FileManager`, `TokenKind`, `Token`, `Lexer`; reject legacy tokens with diagnostics. | `int main(){return 0;}` tokenizes with source ranges, and trigraph input produces a clear diagnostic. |
| P2 Parser + AST construction | Add recursive-descent parser for functions, variables, blocks, `if`, `while`, `for`, `return`, calls, unary/binary expressions. | Parser builds `TranslationUnitDecl` through `ASTContext` for the existing example programs. |
| P3 Sema | Replace string-only type checks with typed symbols/scopes, value category checks, conversions, function signatures, and diagnostic recovery. | Duplicate declarations, undeclared names, invalid returns, and call arity/type errors are reported without crashing. |
| P4 IR + verifier + passes | Complete SSA/CFG invariants, add PHI nodes, verifier, pass manager, constant folding, dead-code elimination, and minimal mem2reg path. | Verifier rejects malformed blocks and optimized IR preserves existing example behavior. |
| P5 Lowering integration | Connect `Compiler::compile_source` to parser → sema → lowering → IR printer/backend. | `--emit-ir` prints generated IR from real source rather than placeholder text. |
| P6 x86-64 backend | Implement SysV AMD64 calls, stack layout, comparisons, branches, labels, constants, register allocation baseline, and assembly emission. | hello/fibonacci/factorial examples assemble and run on x86-64 where supported. |
| P7 Modern C++ surface | Add classes/structs, constructors/destructors, RAII, references, overload resolution, templates subset, concepts later, and modules later. | Each feature has one compile-pass and one compile-fail example tied to the teaching chapter. |
| P8 Optional backends/runtime | Implement optional LLVM IR backend behind `PRISM_ENABLE_LLVM`, then RISC-V only when source exists; add minimal runtime/library pieces after compiler milestones need them. | Backend choice changes output format without changing frontend diagnostics. |

## 5. Core Module Design Constraints

### Basic/Diagnostics

Diagnostics are part of the teaching interface, not compiler afterthoughts. Prism should explicitly avoid the traditional C++ compiler failure mode where one root error explodes into pages of template/internal noise. Use Rust-style diagnostics as the reference quality bar: concise primary message, exact source span, labeled notes, actionable help, and recovery that suppresses low-value cascades.

Required diagnostic shape:

- One primary error per real root cause when possible; follow-on errors should be downgraded, grouped, or suppressed when they are consequences of the first failure.
- Every user-facing diagnostic must carry a stable error code, `SourceLocation`, and source range once range plumbing exists.
- Messages should name the violated rule in learner language, then show the relevant source line with a caret/range label.
- `note:` entries explain why Prism inferred a type, overload candidate, lifetime, or lookup result.
- `help:` entries suggest the smallest valid rewrite when Prism can do so confidently.
- Template, overload, and future concepts diagnostics must summarize candidate sets before showing details; never dump raw instantiation stacks as the default output.
- Fatal diagnostics are reserved for unrecoverable infrastructure failures; normal lexer/parser/sema failures must stay recoverable enough to continue finding independent errors.

Example style target:

```text
error[E0201]: call to `add` expects 2 arguments, got 1
 --> example.cc:6:18
  |
6 |     return add(3);
  |            --- ^ missing argument of type `int`
  |
note: `add` declared here with signature `int add(int a, int b)`
 --> example.cc:1:5
help: pass a second argument: `add(3, value)`
```

The implementation may borrow presentation lessons from Rust, Clang, and Elm, but must be Prism-owned: no dependency on Rust tooling or standard-library formatting surfaces.

### Lexer/Parser

The lexer owns tokenization and legacy-token rejection; the parser owns grammar and recovery; neither performs semantic lookup.

### AST/Sema

AST preserves source structure for teaching; sema attaches meaning and rejects invalid programs; do not lower away concepts before they are visible to diagnostics.

### IR/Passes

IR follows the LLVM-style `Value`/`User`/`Use` teaching model; every pass must state its invariant and verifier expectation.

### Backend

x86-64 is the first concrete backend; LLVM IR is optional interop, not a replacement for Prism IR; RISC-V remains later until a real source file is added.

### Core Library

Utility code should teach implementation techniques only when it supports the compiler or replaces a known weak legacy C++ facility with a better modern design; do not expand into a general `std` clone before the compiler pipeline works.

## 6. Reference Implementation Map

References guide design choices; they do not license blind copying.

Corresponding source code for reference comparisons is available under `/Users/nya/workspace/opensource/modern_cpp/references/impl`.

| Inspiration | Prism decision | Local links |
|---|---|---|
| LLVM/Clang | Use `ASTContext`, diagnostics, IR `Value/User/Use`, `IRBuilder`, and pass/verifier discipline as teaching models. | `docs/internals/compiler/clang-internals.md`, `docs/libraries/llvm/index.md` |
| GCC/G++ | Learn from lowering stages, ABI awareness, tree/GIMPLE/RTL lessons, and diagnostics compatibility choices. | `docs/internals/compiler/gcc-internals.md` |
| Folly | Prefer small fast utilities, explicit ownership, cache-conscious containers, `fbstring` as one string-design reference, and pragmatic “better than legacy standard interface” replacements. | `docs/libraries/folly/index.md` |
| fmt / C++23 print / libc++ / libstdc++ | Treat formatting and standard-library internals as teaching references, not code-copy targets or dependencies; implement Prism-owned equivalents such as formatting, strings, ownership, and allocators instead of using `std::` facilities. | `docs/libraries/libstdcxx/index.md`, `docs/internals/cpp23/expected.md` |

Every borrowed design must be rewritten in Prism style and explained for readers; no blind source copying and no `std::` shortcuts in Prism implementation code.

## 7. Learning Path

1. Build and run tests to learn the harness and failure vocabulary; concept taught: source locations in diagnostics and reproducible fixtures.
2. Inspect tokens to see how raw characters become language units; concept taught: lexical source locations and rejected legacy spelling.
3. Inspect an AST dump to connect grammar to object lifetime; concept taught: grammar ownership through `ASTContext`.
4. Inspect sema diagnostics for valid and invalid examples; concept taught: type rules, value categories, lookup, and recovery.
5. Inspect Prism IR after lowering; concept taught: SSA, CFG, `Value`/`User`/`Use`, and explicit control flow.
6. Run one optimization and compare before/after IR; concept taught: data-flow, invariants, and verifier expectations.
7. Inspect x86-64 assembly for a tiny function; concept taught: ABI, stack layout, calls, branches, and registers.
8. Compare one feature against LLVM/GCC/Folly references; concept taught: library design tradeoffs and “keep the strengths, discard the baggage”.

## 8. Verification Matrix

Build/typecheck alone is insufficient for compiler features; each compiler phase needs at least one compile-pass and one compile-fail fixture when implemented.

| Area | Verification | Expected evidence |
|---|---|---|
| Documentation | From repository root, `test -f prism/docs/compiler-roadmap.en.md` and `test -f prism/docs/compiler-roadmap.zh.md` succeed, and the content-anchor Python check for required headings and phrases passes. | Both roadmaps exist, have no web/frontmatter metadata, and contain required anchors. |
| Diagnostics UX | Compile-fail fixtures compare rendered diagnostics against Rust-style expectations: error code, primary span, labeled source, `note:`, `help:`, and bounded cascade count. | Invalid programs produce concise root-cause diagnostics instead of verbose C++-style cascades or raw instantiation dumps. |
| Lexer/parser | Tokenize and parse `int main(){return 0;}` with source ranges; reject trigraph input with a clear diagnostic. | One compile-pass fixture and one compile-fail fixture. |
| Sema | Check duplicate declarations, undeclared names, invalid returns, and call arity/type errors. | Diagnostics are reported without crashing and with useful source locations. |
| IR | Verify SSA/CFG invariants, PHI placement, and pass outputs. | Verifier rejects malformed blocks; optimized IR preserves behavior. |
| Backend | For future concrete input `int main() { return 42; }`, emitted IR contains a `ret` of integer constant `42`; the x86-64 backend eventually emits a function returning `42`. | Assembly or object output matches the frontend result. |
| Examples | hello/fibonacci/factorial examples compile, run where supported, and keep frontend diagnostics stable across backend choices. | Example fixtures cover both success and expected failure. |
| Library policy | The roadmaps name the no-`std::` implementation rule, Prism-owned replacements (`Vector`, `String`, `Expected`, `UniquePtr`, allocators), `iostream`, `fmt`, `std::print`, and `fbstring`/Folly string inspiration so the “keep the strengths, discard the baggage” / `去其糟粕，取其精华` direction is testable from the documents. | Forbidden and approved library choices are explicit, and standard-library internals are reference material only. |
| Language split | The English document is the canonical English version and the Chinese document is the canonical Chinese version. | The roadmap is split into one `en` document and one `zh` document, not one mixed bilingual file. |
