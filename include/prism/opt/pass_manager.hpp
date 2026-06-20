#pragma once

#include "prism/basic/error.hpp"
#include "prism/core/container/expected.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/memory/unique_ptr.hpp"

namespace prism::ir {
class Module;
}

namespace prism::opt {

/** @brief Abstract base class for IR module transformation passes. */
class ModulePass {
public:
    /** @brief Virtual destructor. */
    virtual ~ModulePass() = default;

    /** @brief Runs the pass on the given module. @return True if the module was modified, or an error. */
    virtual Expected<bool, Error> run(ir::Module& module) = 0;
};

/** @brief Manages and runs a sequence of optimization passes on an IR module. */
class PassManager {
public:
    /** @brief Adds a pass to the pass pipeline. @param pass Unique pointer to the pass to add. */
    void add_pass(UniquePtr<ModulePass> pass);

    /** @brief Runs all registered passes on the module. @return True if any pass modified the module, or an error. */
    Expected<bool, Error> run(ir::Module& module);

private:
    Vector<UniquePtr<ModulePass>> passes_;
};

/** @brief Optimization pass that folds constant expressions at compile time. */
class ConstantFoldingPass final : public ModulePass {
public:
    /** @brief Runs constant folding on the module. @return True if modified, or an error. */
    Expected<bool, Error> run(ir::Module& module) override;
};

/** @brief Optimization pass that eliminates unreachable (dead) code. */
class DeadCodeEliminationPass final : public ModulePass {
public:
    /** @brief Runs dead code elimination on the module. @return True if modified, or an error. */
    Expected<bool, Error> run(ir::Module& module) override;
};

/** @brief Optimization pass that promotes allocas to SSA registers where possible. */
class Mem2RegPass final : public ModulePass {
public:
    /** @brief Runs mem2reg promotion on the module. @return True if modified, or an error. */
    Expected<bool, Error> run(ir::Module& module) override;
};

}  // namespace prism::opt
