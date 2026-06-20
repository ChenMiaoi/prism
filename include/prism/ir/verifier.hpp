#pragma once

#include "prism/basic/error.hpp"
#include "prism/core/container/expected.hpp"

namespace prism::ir {

class Module;
class Function;
class BasicBlock;

/**
 * @brief Validates the structural integrity of IR entities.
 *
 * Checks that the IR is well-formed: types are consistent, basic blocks are
 * properly terminated, and instruction operands are valid.
 */
class Verifier {
public:
    /**
     * @brief Verifies an entire module.
     * @param module The module to verify.
     * @return Expected<void> on success, Error on failure.
     */
    Expected<void, Error> verify_module(const Module& module) const;

    /**
     * @brief Verifies a single function.
     * @param function The function to verify.
     * @return Expected<void> on success, Error on failure.
     */
    Expected<void, Error> verify_function(const Function& function) const;

    /**
     * @brief Verifies a single basic block.
     * @param block The basic block to verify.
     * @return Expected<void> on success, Error on failure.
     */
    Expected<void, Error> verify_basic_block(const BasicBlock& block) const;
};

}  // namespace prism::ir
