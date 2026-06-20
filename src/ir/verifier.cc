#include "prism/ir/verifier.hpp"

#include "prism/ir/basic_block.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/module.hpp"

namespace prism::ir {

/** @brief Verifies the integrity of all functions in the module. */
Expected<void, Error> Verifier::verify_module(const Module& module) const {
    for (size_t i = 0; i < module.functions().size(); ++i) {
        auto result = verify_function(*module.functions()[i]);
        if (!result) return Unexpected<Error>(result.error());
    }
    return {};
}

/** @brief Verifies the integrity of a single function, including all its basic blocks. */
Expected<void, Error> Verifier::verify_function(const Function& function) const {
    if (function.is_declaration()) return {};
    if (function.empty()) return Unexpected<Error>(make_error("function definition has no basic blocks"));
    for (size_t i = 0; i < function.blocks().size(); ++i) {
        auto result = verify_basic_block(*function.blocks()[i]);
        if (!result) return Unexpected<Error>(result.error());
    }
    return {};
}

/** @brief Verifies the integrity of a single basic block (terminator presence, phi ordering, etc.). */
Expected<void, Error> Verifier::verify_basic_block(const BasicBlock& block) const {
    if (block.empty()) return Unexpected<Error>(make_error("basic block has no terminator"));
    bool saw_non_phi = false;
    bool saw_terminator = false;
    for (size_t i = 0; i < block.instructions().size(); ++i) {
        auto* inst = block.instructions()[i].get();
        if (saw_terminator) return Unexpected<Error>(make_error("instruction appears after terminator"));
        if (inst->get_parent() != &block)
            return Unexpected<Error>(make_error("instruction parent does not match owning block"));
        if (inst->get_opcode() == Opcode::Phi) {
            auto* phi = static_cast<const PHINode*>(inst);
            if (saw_non_phi) return Unexpected<Error>(make_error("phi node must appear before non-phi instructions"));
            if (phi->incoming_count() == 0) return Unexpected<Error>(make_error("phi node has no incoming values"));
            if (phi->incoming_count() != phi->operand_count())
                return Unexpected<Error>(make_error("phi incoming count mismatch"));
            for (unsigned incoming = 0; incoming < phi->incoming_count(); ++incoming) {
                if (!phi->incoming_value(incoming) || !phi->incoming_block(incoming)) {
                    return Unexpected<Error>(make_error("phi node has null incoming edge"));
                }
                if (phi->incoming_block(incoming)->get_parent() != block.get_parent()) {
                    return Unexpected<Error>(make_error("phi incoming block is not in the same function"));
                }
            }
        } else {
            saw_non_phi = true;
        }
        if (inst->is_terminator()) saw_terminator = true;
    }
    if (!saw_terminator) return Unexpected<Error>(make_error("basic block has no terminator"));
    return {};
}

}  // namespace prism::ir
