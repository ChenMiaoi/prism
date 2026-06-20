#include "prism/opt/pass_manager.hpp"

#include "prism/ir/basic_block.hpp"
#include "prism/ir/constant.hpp"
#include "prism/ir/context.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/module.hpp"

namespace prism::opt {
namespace {

/** @brief Replaces all uses of @p old_value with @p new_value within the given function. */
void replace_uses(ir::Function& function, ir::Value* old_value, ir::Value* new_value) {
    for (size_t block_index = 0; block_index < function.blocks().size(); ++block_index) {
        auto& instructions = function.blocks()[block_index]->instructions();
        for (size_t inst_index = 0; inst_index < instructions.size(); ++inst_index) {
            auto* user = instructions[inst_index].get();
            for (unsigned operand = 0; operand < user->operand_count(); ++operand) {
                if (user->operand(operand) == old_value) user->set_operand(operand, new_value);
            }
        }
    }
}

/** @brief Returns @c true if the instruction is dead and safe to remove (no side effects). */
bool is_removable_dead_instruction(ir::Instruction* inst) {
    if (inst->is_terminator()) return false;
    if (inst->get_opcode() == ir::Opcode::Store) return false;
    if (inst->get_opcode() == ir::Opcode::Call) return false;
    return true;
}

/** @brief Folds a binary operator with constant integer operands into a single constant. */
ir::ConstantInt* fold_binary(ir::Context& context, ir::BinaryOperator* bin) {
    auto* lhs = dynamic_cast<ir::ConstantInt*>(bin->get_lhs());
    auto* rhs = dynamic_cast<ir::ConstantInt*>(bin->get_rhs());
    if (!lhs || !rhs) return nullptr;

    const int64_t left = lhs->get_value();
    const int64_t right = rhs->get_value();
    switch (bin->get_opcode()) {
    case ir::Opcode::Add:
        return context.get_int_constant(left + right, lhs->get_type());
    case ir::Opcode::Sub:
        return context.get_int_constant(left - right, lhs->get_type());
    case ir::Opcode::Mul:
        return context.get_int_constant(left * right, lhs->get_type());
    case ir::Opcode::SDiv:
        if (right == 0) return nullptr;
        return context.get_int_constant(left / right, lhs->get_type());
    case ir::Opcode::SRem:
        if (right == 0) return nullptr;
        return context.get_int_constant(left % right, lhs->get_type());
    default:
        return nullptr;
    }
}

/** @brief Folds an integer comparison with constant operands into a boolean constant. */
ir::ConstantInt* fold_icmp(ir::Context& context, ir::ICmpInst* cmp) {
    auto* lhs = dynamic_cast<ir::ConstantInt*>(cmp->get_lhs());
    auto* rhs = dynamic_cast<ir::ConstantInt*>(cmp->get_rhs());
    if (!lhs || !rhs) return nullptr;

    const int64_t left = lhs->get_value();
    const int64_t right = rhs->get_value();
    bool result = false;
    switch (cmp->get_opcode()) {
    case ir::Opcode::ICmpEQ:
        result = left == right;
        break;
    case ir::Opcode::ICmpNE:
        result = left != right;
        break;
    case ir::Opcode::ICmpSLT:
        result = left < right;
        break;
    case ir::Opcode::ICmpSGT:
        result = left > right;
        break;
    case ir::Opcode::ICmpSLE:
        result = left <= right;
        break;
    case ir::Opcode::ICmpSGE:
        result = left >= right;
        break;
    default:
        return nullptr;
    }
    return context.get_int_constant(result ? 1 : 0, context.i1_type());
}

/** @brief Returns the index of @p block within the function's block list. */
size_t block_index(ir::Function& function, ir::BasicBlock* block) {
    for (size_t i = 0; i < function.blocks().size(); ++i) {
        if (function.blocks()[i].get() == block) return i;
    }
    return function.blocks().size();
}

/** @brief Returns all predecessor blocks of @p block in the control flow graph. */
Vector<ir::BasicBlock*> predecessors(ir::Function& function, ir::BasicBlock* block) {
    Vector<ir::BasicBlock*> result;
    for (size_t i = 0; i < function.blocks().size(); ++i) {
        auto* candidate = function.blocks()[i].get();
        auto* term = dynamic_cast<ir::BranchInst*>(candidate->get_terminator());
        if (!term) continue;
        for (unsigned succ = 0; succ < term->get_num_successors(); ++succ) {
            if (term->get_successor(succ) == block) result.push_back(candidate);
        }
    }
    return result;
}

/** @brief Checks whether an alloca is promotable to registers (only simple loads/stores). */
bool is_promotable_alloca(ir::Function& function, ir::AllocaInst* alloca) {
    for (size_t block = 0; block < function.blocks().size(); ++block) {
        auto& instructions = function.blocks()[block]->instructions();
        for (size_t inst = 0; inst < instructions.size(); ++inst) {
            auto* user = instructions[inst].get();
            for (unsigned operand = 0; operand < user->operand_count(); ++operand) {
                if (user->operand(operand) != alloca) continue;
                if (auto* load = dynamic_cast<ir::LoadInst*>(user)) {
                    if (load->get_pointer_operand() == alloca) continue;
                }
                if (auto* store = dynamic_cast<ir::StoreInst*>(user)) {
                    if (store->get_pointer_operand() == alloca && store->get_value_operand() != alloca) continue;
                }
                return false;
            }
        }
    }
    return true;
}

/** @brief Tracks pending phi nodes to be inserted after promotion. */
struct PendingPhi {
    ir::BasicBlock* block = nullptr;
    ir::PHINode* phi = nullptr;
};

/** @brief Determines whether an alloca can be promoted and computes incoming values and pending phis.
 *  @param function The enclosing function.
 *  @param alloca The alloca instruction to analyze.
 *  @param incoming_values [out] Per-block incoming value for the promoted variable.
 *  @param pending_phis [out] Phi nodes to insert.
 *  @return @c true if the alloca can be promoted.
 */
bool can_promote_alloca(ir::Function& function, ir::AllocaInst* alloca, Vector<ir::Value*>& incoming_values,
                        Vector<PendingPhi>& pending_phis) {
    Vector<ir::Value*> outgoing_values;
    incoming_values.resize(function.blocks().size());
    outgoing_values.resize(function.blocks().size());

    for (size_t block = 0; block < function.blocks().size(); ++block) {
        auto* bb = function.blocks()[block].get();
        auto preds = predecessors(function, bb);
        ir::Value* current = nullptr;
        if (block == 0) {
            current = nullptr;
        } else if (preds.size() == 1) {
            size_t pred_index = block_index(function, preds[0]);
            if (pred_index >= block) return false;
            current = outgoing_values[pred_index];
        } else if (!preds.empty()) {
            Vector<ir::Value*> values;
            Vector<ir::BasicBlock*> blocks;
            ir::Value* first = nullptr;
            bool all_same = true;
            for (size_t pred = 0; pred < preds.size(); ++pred) {
                size_t pred_index = block_index(function, preds[pred]);
                if (pred_index >= block) return false;
                auto* value = outgoing_values[pred_index];
                if (!value) return false;
                if (pred == 0)
                    first = value;
                else if (value != first)
                    all_same = false;
                values.push_back(value);
                blocks.push_back(preds[pred]);
            }
            if (all_same) {
                current = first;
            } else {
                auto* phi = new ir::PHINode(alloca->get_allocated_type(), static_cast<Vector<ir::Value*>&&>(values),
                                            static_cast<Vector<ir::BasicBlock*>&&>(blocks), alloca->get_name().c_str());
                phi->set_parent(bb);
                PendingPhi pending;
                pending.block = bb;
                pending.phi = phi;
                pending_phis.push_back(static_cast<PendingPhi&&>(pending));
                current = phi;
            }
        }

        incoming_values[block] = current;
        auto& instructions = bb->instructions();
        for (size_t inst = 0; inst < instructions.size(); ++inst) {
            if (auto* store = dynamic_cast<ir::StoreInst*>(instructions[inst].get())) {
                if (store->get_pointer_operand() == alloca) current = store->get_value_operand();
            } else if (auto* load = dynamic_cast<ir::LoadInst*>(instructions[inst].get())) {
                if (load->get_pointer_operand() == alloca && !current) return false;
            }
        }
        outgoing_values[block] = current;
    }
    return true;
}

/** @brief Inserts pending phi nodes into their respective blocks after promotion. */
void insert_pending_phis(Vector<PendingPhi>& pending_phis) {
    for (size_t i = 0; i < pending_phis.size(); ++i) {
        auto& instructions = pending_phis[i].block->instructions();
        size_t insert_at = 0;
        while (insert_at < instructions.size() && instructions[insert_at]->get_opcode() == ir::Opcode::Phi) {
            ++insert_at;
        }
        instructions.insert(instructions.begin() + insert_at, UniquePtr<ir::Instruction>(pending_phis[i].phi));
    }
}

/** @brief Rewrites all loads/stores of the promoted alloca to use SSA values directly.
 *  @return @c true if any instructions were modified.
 */
bool rewrite_alloca_uses(ir::Function& function, ir::AllocaInst* alloca, Vector<ir::Value*>& incoming_values) {
    bool changed = false;
    for (size_t block = 0; block < function.blocks().size(); ++block) {
        auto* current = incoming_values[block];
        auto& instructions = function.blocks()[block]->instructions();
        for (size_t inst = 0; inst < instructions.size(); ++inst) {
            if (auto* store = dynamic_cast<ir::StoreInst*>(instructions[inst].get())) {
                if (store->get_pointer_operand() != alloca) continue;
                current = store->get_value_operand();
                instructions.erase_at(inst);
                --inst;
                changed = true;
            } else if (auto* load = dynamic_cast<ir::LoadInst*>(instructions[inst].get())) {
                if (load->get_pointer_operand() != alloca) continue;
                replace_uses(function, load, current);
                instructions.erase_at(inst);
                --inst;
                changed = true;
            } else if (instructions[inst].get() == alloca) {
                instructions.erase_at(inst);
                --inst;
                changed = true;
            }
        }
    }
    return changed;
}

}  // namespace

/** @brief Adds a module pass to the pass pipeline.
 *  @param pass The pass to add (ownership is transferred).
 */
void PassManager::add_pass(UniquePtr<ModulePass> pass) {
    passes_.push_back(static_cast<UniquePtr<ModulePass>&&>(pass));
}

/** @brief Runs all registered passes on the module.
 *  @param module The IR module to optimize.
 *  @return An Expected<bool> indicating whether any transformation was applied, or an error.
 */
Expected<bool, Error> PassManager::run(ir::Module& module) {
    bool changed = false;
    for (size_t i = 0; i < passes_.size(); ++i) {
        auto result = passes_[i]->run(module);
        if (!result) return Unexpected<Error>(result.error());
        changed = changed || result.value();
    }
    return changed;
}

/** @brief Folds constant binary operations and comparisons throughout the module.
 *  @param module The IR module to optimize.
 *  @return An Expected<bool> indicating whether any constants were folded.
 */
Expected<bool, Error> ConstantFoldingPass::run(ir::Module& module) {
    bool changed = false;
    auto& context = module.get_context();
    for (size_t function_index = 0; function_index < module.functions().size(); ++function_index) {
        auto* function = module.functions()[function_index].get();
        for (size_t block_index = 0; block_index < function->blocks().size(); ++block_index) {
            auto& instructions = function->blocks()[block_index]->instructions();
            for (size_t inst_index = 0; inst_index < instructions.size(); ++inst_index) {
                ir::Value* folded = nullptr;
                if (auto* bin = dynamic_cast<ir::BinaryOperator*>(instructions[inst_index].get())) {
                    folded = fold_binary(context, bin);
                } else if (auto* cmp = dynamic_cast<ir::ICmpInst*>(instructions[inst_index].get())) {
                    folded = fold_icmp(context, cmp);
                }
                if (!folded) continue;
                replace_uses(*function, instructions[inst_index].get(), folded);
                if (instructions[inst_index]->use_empty()) {
                    instructions.erase_at(inst_index);
                    --inst_index;
                }
                changed = true;
            }
        }
    }
    return changed;
}

/** @brief Removes dead instructions (unused values with no side effects) from the module.
 *  @param module The IR module to optimize.
 *  @return An Expected<bool> indicating whether any instructions were removed.
 */
Expected<bool, Error> DeadCodeEliminationPass::run(ir::Module& module) {
    bool changed = false;
    for (size_t function_index = 0; function_index < module.functions().size(); ++function_index) {
        auto* function = module.functions()[function_index].get();
        for (size_t block_index = 0; block_index < function->blocks().size(); ++block_index) {
            auto& instructions = function->blocks()[block_index]->instructions();
            for (size_t inst_index = 0; inst_index < instructions.size(); ++inst_index) {
                auto* inst = instructions[inst_index].get();
                if (!is_removable_dead_instruction(inst) || !inst->use_empty()) continue;
                instructions.erase_at(inst_index);
                --inst_index;
                changed = true;
            }
        }
    }
    return changed;
}

/** @brief Promotes allocas to SSA registers (mem2reg optimization).
 *  @param module The IR module to optimize.
 *  @return An Expected<bool> indicating whether any allocas were promoted.
 */
Expected<bool, Error> Mem2RegPass::run(ir::Module& module) {
    bool changed = false;
    for (size_t function_index = 0; function_index < module.functions().size(); ++function_index) {
        auto* function = module.functions()[function_index].get();
        if (function->empty()) continue;

        Vector<ir::AllocaInst*> allocas;
        auto& entry_instructions = function->blocks()[0]->instructions();
        for (size_t inst = 0; inst < entry_instructions.size(); ++inst) {
            if (auto* alloca = dynamic_cast<ir::AllocaInst*>(entry_instructions[inst].get())) {
                allocas.push_back(alloca);
            }
        }

        for (size_t alloca_index = 0; alloca_index < allocas.size(); ++alloca_index) {
            auto* alloca = allocas[alloca_index];
            if (!is_promotable_alloca(*function, alloca)) continue;

            Vector<ir::Value*> incoming_values;
            Vector<PendingPhi> pending_phis;
            if (!can_promote_alloca(*function, alloca, incoming_values, pending_phis)) continue;

            insert_pending_phis(pending_phis);
            changed = rewrite_alloca_uses(*function, alloca, incoming_values) || changed;
        }
    }
    return changed;
}
}  // namespace prism::opt
