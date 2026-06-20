# Prism C++23 编译器实现路线图

> Prism 是面向教学的迷你完整 C++ 编译器，使用现代 C++23 编写，并以支持 C++23 标准为目标，借鉴 LLVM/Clang 架构模式、GCC/G++ 降低与 ABI 经验、Folly 风格的底层工具纪律，并有意识地实践“去其糟粕，取其精华”：排除过时的 C++ 语法和库选择，而不是为了兼容性保留它们。

## 1. 当前基线

当前代码树已经具备有用的编译器模块形态，但前端尚未把源码文本连到真实 AST。

| 模块 | 路径 | 职责对象 | 当前状态 |
|---|---|---|---|
| `basic` | `prism/include/prism/basic`, `prism/src/basic` | `DiagnosticsEngine`, `Diagnostic`, `DiagLevel`, `Error`, `SourceLocation` | 当前诊断已经使用 Prism 字符串/vector 风格存储。 |
| `core` | `prism/include/prism/core` | `prism::String`, `StringView`, `Vector`, `HashMap`, `Optional`, `Expected`, `UniquePtr`, `SharedPtr`, `Function`, `BumpAllocator`, `format/print`, `raw_ostream` | 受 Folly/LLVM 启发的工具层。 |
| `ast` | `prism/include/prism/ast`, `prism/src/ast` | `ASTContext`, `DeclKind`, `ExprKind`, `StmtKind`, `TranslationUnitDecl`, `FunctionDecl`, `BinaryExpr`, `CallExpr`, `BlockStmt` | AST 词汇已经存在，用于教学源码结构。 |
| `sema` | `prism/include/prism/sema`, `prism/src/sema` | `sema::TypeInfo`, `SemanticAnalyzer::analyze`, statement/expression/type checks | 当前规模较小，并且类型信息偏字符串化。 |
| `ir` | `prism/include/prism/ir`, `prism/src/ir` | LLVM-like `Context`, `Type`, `Value`, `User`, `Use`, `Instruction`, `Module`, `Function`, `BasicBlock`, `IRBuilder`, `IRPrinter` | IR 具备 LLVM 风格的教学词汇。 |
| `codegen` | `prism/include/prism/codegen`, `prism/src/codegen` | `ASTLowering`, `x86_64::X86_64CodeGenerator` | 当前后端是文本化且覆盖面较窄。 |
| `driver` | `prism/include/prism/driver/compiler.hpp`, `prism/src/driver/compiler.cc` | `Compiler::compile`, `Compiler::compile_source`, `emit_ir` | `Compiler::compile_source` 是占位实现，未调用 parser/sema/lowering/backend。 |
| `tests` | `prism/tests/unit/test_all.cc`, `prism/tests/examples` | IR、sema、codegen 和示例烟测 | 当前测试手动覆盖 IR、sema、codegen，以及较窄的全管线烟测路径。 |

已确认缺口：

- 没有真实 lexer/token 层。
- 没有 parser、preprocessor 或 source manager。
- 没有真实 source-to-AST 路径。
- 没有 optimizer/pass manager。
- 未发现 LLVM 后端源码。
- RISC-V 只在选项中被提到，已检查源码中尚不存在。
- README 已说明 `prism::Expected`/`prism::` 工具层和禁止 `std::` 实现捷径的规则；现有源码中的 `std::` 仍是需要迁移到 Prism 自有核心设施的技术债。

## 2. 目标架构

Prism 应当成长为一条透明的编译器管线，让读者能够检查各个中间形态。

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

所有权规则：长生命周期编译器对象应由 arena/context 拥有（`ASTContext`、`ir::Context`，之后在合适位置使用 `BumpAllocator`）；瞬时结果应返回 `prism::Expected<T, prism::Error>` 或 `prism::Expected<void, prism::Error>`，而不是使用异常。

依赖规则：LLVM、GCC 和 Folly 默认是参考对象，而不是依赖；只有可选 LLVM 后端在 `PRISM_ENABLE_LLVM` 开启时才可以要求 LLVM。

## 3. C++23 与库使用准则

Prism 不是兼容性博物馆，也不是标准库外壳。Prism 要做 full Prism：编译器代码使用的每个设施都必须是 Prism 自有设施，或被明确隔离在第三方/参考边界。

Prism 是使用 C++23 编写的 C++23 编译器，但 Prism 源码不得把 `std::` 标准库设施当作实现捷径。新代码应当在语法和工程纪律上保持现代 C++：显式所有权、值语义、能澄清约束的 concepts、能移除运行期开销的 `constexpr`/`consteval`、结构化绑定、适当的指定初始化，以及 `prism::Expected` 风格错误流。不要因为熟悉就写 C++03/C++11 风格，也不要在 Prism 实现代码中借用 `std::vector`、`std::string`、`std::unique_ptr`、`std::allocator`、`std::map`、`std::unordered_map`、`std::optional`、`std::function`、`std::format`、`std::print`、`std::iostream` 或类似标准库类型偷懒。

如果 Prism 需要类似标准库组件的设施，必须参考对应核心设计后实现 Prism 版本：例如使用/扩展 `prism::UniquePtr` 而不是 `std::unique_ptr`，使用 `prism::Vector` 而不是 `std::vector`，使用 `prism::String`/`StringView` 而不是 `std::string`/`std::string_view`，使用 `prism::Expected` 而不是 `std::expected`，使用 Prism allocator/arena 而不是 `std::allocator` 或依赖 allocator 的 STL 容器。LLVM、GCC、Folly、fmt、libc++、libstdc++ 等外部项目只能作为权衡参考，不是依赖表面，也不能复制源码。

| 状态 | 设施或规则 | 准则 |
|---|---|---|
| 批准 | `prism::String` / `prism::StringView` | 用于拥有/非拥有字符串。最终设计目标是一个更适合教学的字符串，借鉴 Folly `fbstring`、LLVM `SmallString`/`StringRef`、libc++/libstdc++ SSO 经验，以及现有 `prism::String`；不要在 Prism 实现代码中克隆或使用 `std::string`。 |
| 批准 | `prism::Vector<T>` | 用于编译器拥有的连续序列；不要在 Prism 实现代码中使用 `std::vector`。 |
| 批准 | `prism::HashMap<K,V>` 和未来的 `prism::ScopedHashTable<T>` | 用于映射和作用域；任何作用域表都必须建立在 Prism 容器上，而不是 `std::unordered_map`、`std::map` 或其他 STL 存储。 |
| 批准 | `prism::Expected<T, Error>` / `prism::Unexpected<Error>` | 用于可恢复失败；不要为普通编译器错误使用 `std::expected`、异常或类似异常的控制流。 |
| 批准 | `prism::format`, `prism::print`, `prism::println`, `prism::raw_ostream` | 用于输出。fmt 和 C++23 print 语义只是设计参考；Prism 不得依赖 `std::format`、`std::print` 或 `iostream` 作为实现表面。 |
| 批准 | `prism::UniquePtr`, `prism::SharedPtr`, `BumpAllocator`, 未来的 Prism arena/allocator API | 用于所有权和分配。如果需要新的分配模式，应实现为 Prism allocator/arena，而不是使用 `std::allocator` 或依赖 allocator 的 STL 容器。 |
| 禁止 | Prism 实现代码中的 `std::` | 禁止在 Prism 自有 compiler/core 代码中使用 `std::unique_ptr`、`std::shared_ptr`、`std::vector`、`std::string`、`std::optional`、`std::function`、`std::map`、`std::unordered_map`、`std::allocator`、`std::format`、`std::print`、`std::iostream` 或等价 STL 实现捷径。 |
| 禁止 | Primary `iostream`/`istream`/`ostream` Prism library surface | 不要把流 API 实现为主要库表面；如果临时无法避免宿主互操作，必须隔离在 Prism API 后面并安排移除。 |
| 禁止 | Legacy C++ compatibility baggage | 排除 `std::auto_ptr`、`std::bind1st`/`bind2nd`、`std::mem_fun`、动态异常规格、超出 lexer 拒绝诊断范围的 trigraph/digraph 教学支持、旧 `export` 模板语义、C++98/03 兼容模式、宽泛厂商兼容怪癖，以及在编译器管线工作前实现宽泛标准库克隆。 |

现有 `std::` 出现是需要移除的技术债，不是可接受先例。每次迁移都应替换为 Prism 自有设施，并保持实现可教学。

## 4. 阶段路线图

阶段按编译器依赖顺序排列，而不是按功能吸引力排列。

| 里程碑 | 范围 | 验收检查 |
|---|---|---|
| P0 独立英文与中文文档及基线 | 产出独立英文和中文路线图；之后更新文档时，保持 README 与 `prism::Expected`/`prism::` 命名和禁止 `std::` 实现捷径的规则一致。 | `prism/docs/compiler-roadmap.en.md` 和 `prism/docs/compiler-roadmap.zh.md` 存在，不包含网页 frontmatter/sidebar 假设，并说明 “keep the strengths, discard the baggage” / `去其糟粕，取其精华` 库准则以及 full-Prism no-`std::` 规则。 |
| P1 Source + Lexer | 添加 `SourceManager`、`FileManager`、`TokenKind`、`Token`、`Lexer`；用诊断拒绝遗留 token。 | `int main(){return 0;}` 可带源码范围地被 token 化，且 trigraph 输入产生清晰诊断。 |
| P2 Parser + AST construction | 添加递归下降 parser，覆盖函数、变量、块、`if`、`while`、`for`、`return`、调用、一元/二元表达式。 | parser 能通过 `ASTContext` 为现有示例程序构建 `TranslationUnitDecl`。 |
| P3 Sema | 用带类型的符号/作用域、值类别检查、转换、函数签名和诊断恢复，替代仅字符串式类型检查。 | 重复声明、未声明名称、非法返回、调用参数数量/类型错误都能被报告且不崩溃。 |
| P4 IR + verifier + passes | 补全 SSA/CFG 不变量，添加 PHI 节点、verifier、pass manager、常量折叠、死代码消除和最小 mem2reg 路径。 | verifier 拒绝畸形 basic block，优化后的 IR 保持现有示例行为。 |
| P5 Lowering integration | 把 `Compiler::compile_source` 接到 parser → sema → lowering → IR printer/backend。 | `--emit-ir` 打印来自真实源码的生成 IR，而不是占位文本。 |
| P6 x86-64 backend | 实现 SysV AMD64 调用、栈布局、比较、分支、标签、常量、寄存器分配基线和汇编输出。 | hello/fibonacci/factorial 示例在支持 x86-64 的环境中可汇编并运行。 |
| P7 Modern C++ surface | 添加 class/struct、构造/析构、RAII、引用、重载决议、模板子集，之后加入 concepts 和 modules。 | 每个特性都有一个 compile-pass 和一个 compile-fail 示例，并绑定到教学章节。 |
| P8 Optional backends/runtime | 在 `PRISM_ENABLE_LLVM` 后实现可选 LLVM IR 后端；仅当源码存在后再实现 RISC-V；在编译器里程碑需要后再添加最小 runtime/library 部件。 | 后端选择改变输出格式，但不改变前端诊断。 |

## 5. 核心模块设计约束

### Basic/Diagnostics

诊断是教学界面的一部分，不是编译器的附属输出。Prism 应明确避免传统 C++ 编译器的失败模式：一个根因错误扩散成数页模板/内部细节噪音。诊断质量以 Rust 风格为参考：主消息简洁、源码范围精确、标签清楚、help 可执行，并通过恢复机制压制低价值级联错误。

诊断形态要求：

- 尽可能做到一个真实根因只产生一个 primary error；后续由它引发的错误应降级、合并或抑制。
- 每个面向用户的诊断都必须带稳定 error code、`SourceLocation`，并在 range 管线存在后带源码范围。
- 消息先用学习者能理解的语言说明违反的规则，再展示相关源码行和 caret/range 标签。
- `note:` 用来解释 Prism 为什么推断出某个类型、重载候选、生命周期或查找结果。
- `help:` 只在 Prism 有把握时给出最小合法改写建议。
- 模板、重载和未来 concepts 诊断必须先摘要候选集合，再展开细节；默认输出绝不倾倒原始实例化栈。
- fatal 诊断只用于不可恢复的基础设施失败；普通 lexer/parser/sema 失败必须尽量可恢复，以继续发现独立错误。

目标示例风格：

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

实现可以借鉴 Rust、Clang、Elm 的展示经验，但必须是 Prism 自有实现：不依赖 Rust 工具链，也不依赖标准库格式化表面。

### Lexer/Parser

lexer 负责 token 化和遗留 token 拒绝；parser 负责语法和恢复；两者都不执行语义查找。

### AST/Sema

AST 为教学保留源码结构；sema 附加语义并拒绝非法程序；不要在概念对诊断可见前就把它们 lower 掉。

### IR/Passes

IR 遵循 LLVM 风格的 `Value`/`User`/`Use` 教学模型；每个 pass 必须说明自己的不变量和 verifier 预期。

### Backend

x86-64 是第一个具体后端；LLVM IR 是可选互操作，不是 Prism IR 的替代品；RISC-V 在真实源码文件加入前仍属于后续目标。

### Core Library

工具代码只有在支持编译器，或用更好的现代设计替代已知薄弱的遗留 C++ 设施时，才应承担实现技术教学；在编译器管线工作前，不要扩展成通用 `std` 克隆。

## 6. 参考实现映射

参考实现用于指导设计选择，而不是允许盲目复制。

用于对照参考的对应源码位于 `/Users/nya/workspace/opensource/modern_cpp/references/impl`。

| 参考来源 | Prism 决策 | 本地链接 |
|---|---|---|
| LLVM/Clang | 以 `ASTContext`、诊断、IR `Value/User/Use`、`IRBuilder`、pass/verifier 纪律作为教学模型。 | `docs/internals/compiler/clang-internals.md`, `docs/libraries/llvm/index.md` |
| GCC/G++ | 借鉴 lowering 阶段、ABI 意识、tree/GIMPLE/RTL 经验和诊断兼容选择。 | `docs/internals/compiler/gcc-internals.md` |
| Folly | 偏好小而快的工具、显式所有权、缓存友好容器，把 `fbstring` 作为字符串设计参考之一，并采用务实的“优于遗留标准接口”的替代品。 | `docs/libraries/folly/index.md` |
| fmt / C++23 print / libc++ / libstdc++ | 把格式化和标准库内部实现作为教学参考，而不是复制对象或依赖；实现 Prism 自有的格式化、字符串、所有权和分配器等设施，而不是使用 `std::` 设施。 | `docs/libraries/libstdcxx/index.md`, `docs/internals/cpp23/expected.md` |

每个借鉴设计都必须以 Prism 风格重写并为读者解释；禁止盲目复制源码，也禁止在 Prism 实现代码中使用 `std::` 捷径。

## 7. 教学路线

1. 构建并运行测试，以学习测试框架和失败词汇；教学概念：诊断中的源码位置和可复现 fixture。
2. 检查 token，观察原始字符如何变成语言单元；教学概念：词法源码位置和被拒绝的遗留拼写。
3. 检查 AST dump，把语法和对象生命周期联系起来；教学概念：通过 `ASTContext` 表达的语法所有权。
4. 检查有效和无效示例的 sema 诊断；教学概念：类型规则、值类别、查找和恢复。
5. 检查 lowering 后的 Prism IR；教学概念：SSA、CFG、`Value`/`User`/`Use` 和显式控制流。
6. 运行一个优化并比较前后 IR；教学概念：数据流、不变量和 verifier 预期。
7. 检查一个小函数的 x86-64 汇编；教学概念：ABI、栈布局、调用、分支和寄存器。
8. 把一个特性与 LLVM/GCC/Folly 参考资料对比；教学概念：库设计权衡，以及 “keep the strengths, discard the baggage” / `去其糟粕，取其精华`。

## 8. 验证矩阵

仅 build/typecheck 不足以验证编译器特性；每个编译阶段实现后都至少需要一个 compile-pass 和一个 compile-fail fixture。

| 领域 | 验证 | 预期证据 |
|---|---|---|
| 文档 | 从仓库根目录执行 `test -f prism/docs/compiler-roadmap.en.md` 和 `test -f prism/docs/compiler-roadmap.zh.md` 成功，并且检查所需标题与短语的 Python 内容锚点检查通过。 | 两份路线图存在、没有 web/frontmatter 元数据，并包含所需锚点。 |
| 诊断 UX | compile-fail fixture 对比 Rust 风格诊断输出：error code、primary span、带标签源码、`note:`、`help:`、以及受控的级联错误数量。 | 非法程序输出简洁的根因诊断，而不是冗余的 C++ 风格级联错误或原始实例化栈。 |
| 词法与语法 | 带源码范围 token 化并解析 `int main(){return 0;}`；用清晰诊断拒绝 trigraph 输入。 | 一个 compile-pass fixture 和一个 compile-fail fixture。 |
| 语义 | 检查重复声明、未声明名称、非法返回、调用参数数量/类型错误。 | 诊断不崩溃地报告，并包含有用源码位置。 |
| 中间表示 | 验证 SSA/CFG 不变量、PHI 放置和 pass 输出。 | verifier 拒绝畸形 block；优化 IR 保持行为。 |
| 后端 | 对未来具体输入 `int main() { return 42; }`，生成 IR 包含整数常量 `42` 的 `ret`；x86-64 后端最终生成返回 `42` 的函数。 | 汇编或目标文件输出与前端结果一致。 |
| 示例 | hello/fibonacci/factorial 示例可编译，并在支持环境中运行；不同后端选择下前端诊断保持稳定。 | 示例 fixture 同时覆盖成功和预期失败。 |
| 库准则 | 路线图明确写出禁止 `std::` 实现捷径、Prism 自有替代设施（`Vector`、`String`、`Expected`、`UniquePtr`、allocators）、`iostream`、`fmt`、`std::print` 和 `fbstring`/Folly 字符串启发，使 “keep the strengths, discard the baggage” / `去其糟粕，取其精华` 方向可从文档中测试。 | 禁止和批准的库选择都被明确写出，标准库内部只能作为参考材料。 |
| 语言拆分 | 英文文档是规范英文版本，中文文档是规范中文版本。 | 路线图拆成一个 `en` 文档和一个 `zh` 文档，而不是一个混合双语文件。 |
