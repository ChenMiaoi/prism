#include "prism/codegen/llvm/llvm_codegen.hpp"

#include "prism/ir/basic_block.hpp"
#include "prism/ir/constant.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/module.hpp"
#include "prism/ir/type.hpp"

#if PRISM_ENABLE_LLVM
#include <llvm/IR/LLVMContext.h>
#endif

namespace prism::codegen {
namespace {

/** @brief Converts an IR type to its LLVM IR textual representation. */
String llvm_type(prism::ir::Type* type) {
    if (!type) return "void";
    if (type->is_void_type()) return "void";
    if (auto* integer = dynamic_cast<prism::ir::IntegerType*>(type)) {
        char buf[16];
        int len = ::snprintf(buf, sizeof(buf), "i%u", integer->bit_width());
        return String(buf, static_cast<size_t>(len));
    }
    if (auto* float_type = dynamic_cast<prism::ir::FloatType*>(type)) {
        return float_type->float_kind() == prism::ir::FloatType::FloatKind::Float ? "float" : "double";
    }
    return type->print();
}

/** @brief Converts an IR value to its LLVM IR textual representation. */
String llvm_value(prism::ir::Value* value) {
    if (!value) return "void";
    if (auto* constant = dynamic_cast<prism::ir::ConstantInt*>(value)) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(constant->get_value()));
        return String(buf, static_cast<size_t>(len));
    }
    if (auto* constant = dynamic_cast<prism::ir::ConstantFP*>(value)) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%f", constant->get_value());
        return String(buf, static_cast<size_t>(len));
    }
    return String("%") + value->get_name();
}

/** @brief Returns the LLVM IR mnemonic for the given opcode. */
const char* llvm_opcode(prism::ir::Opcode opcode) {
    switch (opcode) {
    case prism::ir::Opcode::Add:
        return "add";
    case prism::ir::Opcode::Sub:
        return "sub";
    case prism::ir::Opcode::Mul:
        return "mul";
    case prism::ir::Opcode::SDiv:
        return "sdiv";
    case prism::ir::Opcode::SRem:
        return "srem";
    case prism::ir::Opcode::ICmpEQ:
        return "eq";
    case prism::ir::Opcode::ICmpNE:
        return "ne";
    case prism::ir::Opcode::ICmpSLT:
        return "slt";
    case prism::ir::Opcode::ICmpSGT:
        return "sgt";
    case prism::ir::Opcode::ICmpSLE:
        return "sle";
    case prism::ir::Opcode::ICmpSGE:
        return "sge";
    default:
        return "unknown";
    }
}

/** @brief Appends a typed value (type and value separated by a space) to the output string. */
void append_typed_value(String& out, prism::ir::Value* value) {
    out.append(llvm_type(value->get_type()));
    out.push_back(' ');
    out.append(llvm_value(value));
}

}  // namespace

/** @brief Generates LLVM IR text for the given IR module.
 *  @param module The IR module to generate LLVM IR for.
 *  @return A String containing the complete LLVM IR output.
 */
String LLVMCodeGenerator::generate(ir::Module* module) {
#if PRISM_ENABLE_LLVM
    llvm::LLVMContext llvm_context;
    (void)llvm_context;
#endif
    String out;
    out.append("; Prism LLVM IR\n");
    out.append("source_filename = \"");
    out.append(module->get_name());
    out.append("\"\n\n");

    for (size_t function_index = 0; function_index < module->functions().size(); ++function_index) {
        auto* function = module->functions()[function_index].get();
        out.append("define ");
        out.append(llvm_type(function->get_return_type()));
        out.append(" @");
        out.append(function->get_name());
        out.append("(");
        for (size_t arg = 0; arg < function->arg_count(); ++arg) {
            if (arg > 0) out.append(", ");
            out.append(llvm_type(function->args()[arg]->get_type()));
            out.append(" %");
            out.append(function->args()[arg]->get_name());
        }
        out.append(") {\n");

        for (size_t block_index = 0; block_index < function->blocks().size(); ++block_index) {
            auto* block = function->blocks()[block_index].get();
            out.append(block->get_name());
            out.append(":\n");
            for (size_t inst_index = 0; inst_index < block->instructions().size(); ++inst_index) {
                auto* inst = block->instructions()[inst_index].get();
                out.append("  ");
                if (auto* ret = dynamic_cast<ir::ReturnInst*>(inst)) {
                    out.append("ret");
                    if (auto* value = ret->get_return_value()) {
                        out.push_back(' ');
                        append_typed_value(out, value);
                    } else {
                        out.append(" void");
                    }
                } else if (auto* br = dynamic_cast<ir::BranchInst*>(inst)) {
                    if (inst->get_opcode() == ir::Opcode::CondBr) {
                        out.append("br ");
                        append_typed_value(out, const_cast<ir::Value*>(inst->operand(0)));
                        out.append(", label %");
                        out.append(br->get_successor(0)->get_name());
                        out.append(", label %");
                        out.append(br->get_successor(1)->get_name());
                    } else {
                        out.append("br label %");
                        out.append(br->get_successor(0)->get_name());
                    }
                } else if (auto* bin = dynamic_cast<ir::BinaryOperator*>(inst)) {
                    out.append("%");
                    out.append(inst->get_name());
                    out.append(" = ");
                    out.append(llvm_opcode(inst->get_opcode()));
                    out.push_back(' ');
                    out.append(llvm_type(inst->get_type()));
                    out.push_back(' ');
                    out.append(llvm_value(bin->get_lhs()));
                    out.append(", ");
                    out.append(llvm_value(bin->get_rhs()));
                } else if (auto* cmp = dynamic_cast<ir::ICmpInst*>(inst)) {
                    out.append("%");
                    out.append(inst->get_name());
                    out.append(" = icmp ");
                    out.append(llvm_opcode(inst->get_opcode()));
                    out.push_back(' ');
                    out.append(llvm_type(cmp->get_lhs()->get_type()));
                    out.push_back(' ');
                    out.append(llvm_value(cmp->get_lhs()));
                    out.append(", ");
                    out.append(llvm_value(cmp->get_rhs()));
                } else if (auto* phi = dynamic_cast<ir::PHINode*>(inst)) {
                    out.append("%");
                    out.append(inst->get_name());
                    out.append(" = phi ");
                    out.append(llvm_type(inst->get_type()));
                    for (unsigned incoming = 0; incoming < phi->incoming_count(); ++incoming) {
                        if (incoming > 0) out.append(",");
                        out.append(" [ ");
                        out.append(llvm_value(phi->incoming_value(incoming)));
                        out.append(", %");
                        out.append(phi->incoming_block(incoming)->get_name());
                        out.append(" ]");
                    }
                } else if (auto* alloca = dynamic_cast<ir::AllocaInst*>(inst)) {
                    out.append("%");
                    out.append(inst->get_name());
                    out.append(" = alloca ");
                    out.append(llvm_type(alloca->get_allocated_type()));
                } else if (auto* load = dynamic_cast<ir::LoadInst*>(inst)) {
                    out.append("%");
                    out.append(inst->get_name());
                    out.append(" = load ");
                    out.append(llvm_type(inst->get_type()));
                    out.append(", ptr ");
                    out.append(llvm_value(load->get_pointer_operand()));
                } else if (auto* store = dynamic_cast<ir::StoreInst*>(inst)) {
                    out.append("store ");
                    append_typed_value(out, store->get_value_operand());
                    out.append(", ptr ");
                    out.append(llvm_value(store->get_pointer_operand()));
                } else if (auto* call = dynamic_cast<ir::CallInst*>(inst)) {
                    if (!inst->get_name().empty()) {
                        out.append("%");
                        out.append(inst->get_name());
                        out.append(" = ");
                    }
                    out.append("call ");
                    out.append(llvm_type(call->get_called_function()->get_return_type()));
                    out.append(" @");
                    out.append(call->get_called_function()->get_name());
                    out.append("(");
                    for (unsigned arg = 0; arg < call->arg_count(); ++arg) {
                        if (arg > 0) out.append(", ");
                        append_typed_value(out, const_cast<ir::Value*>(call->operand(arg + 1)));
                    }
                    out.append(")");
                } else {
                    out.append("; unsupported instruction");
                }
                out.push_back('\n');
            }
        }
        out.append("}\n\n");
    }
    return out;
}

}  // namespace prism::codegen
