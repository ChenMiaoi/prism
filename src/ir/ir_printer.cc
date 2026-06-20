#include "prism/ir/ir_printer.hpp"

#include "prism/ir/basic_block.hpp"
#include "prism/ir/constant.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/module.hpp"
#include "prism/ir/type.hpp"
#include "prism/ir/value.hpp"

namespace prism::ir {

/** @brief Prints the entire module as a human-readable string. */
String IRPrinter::print_module(const Module* module) {
    String result;
    result.append("module \"");
    result.append(module->get_name());
    result.append("\" {\n");
    for (size_t i = 0; i < module->functions().size(); ++i) {
        result.append(print_function(module->functions()[i].get()));
        result.append("\n\n");
    }
    result.append("}\n");
    return result;
}

/** @brief Prints a single function as a human-readable string. */
String IRPrinter::print_function(const Function* fn) {
    String result;
    result.append(fn->get_return_type()->print());
    result.append(" @");
    result.append(fn->get_name());
    result.append("(");
    for (size_t i = 0; i < fn->arg_count(); ++i) {
        if (i > 0) result.append(", ");
        result.append(fn->args()[i]->get_type()->print());
    }
    result.append(")");
    if (fn->is_declaration()) return result;
    result.append(" {\n");
    for (size_t i = 0; i < fn->blocks().size(); ++i) {
        result.append(print_basic_block(fn->blocks()[i].get()));
    }
    result.append("}");
    return result;
}

/** @brief Prints a single basic block as a human-readable string. */
String IRPrinter::print_basic_block(const BasicBlock* bb) {
    String result;
    if (!bb->get_name().empty()) {
        result.append(bb->get_name());
        result.append(":\n");
    }
    for (size_t i = 0; i < bb->instructions().size(); ++i) {
        result.append("  ");
        result.append(print_instruction(bb->instructions()[i].get()));
        result.append("\n");
    }
    return result;
}

/** @brief Converts an opcode enum value to its string representation. */
static const char* opcodeToString(Opcode op) {
    switch (op) {
    case Opcode::Add:
        return "add";
    case Opcode::Sub:
        return "sub";
    case Opcode::Mul:
        return "mul";
    case Opcode::SDiv:
        return "sdiv";
    case Opcode::SRem:
        return "srem";
    case Opcode::ICmpEQ:
        return "icmp eq";
    case Opcode::ICmpNE:
        return "icmp ne";
    case Opcode::ICmpSLT:
        return "icmp slt";
    case Opcode::ICmpSGT:
        return "icmp sgt";
    case Opcode::ICmpSLE:
        return "icmp sle";
    case Opcode::ICmpSGE:
        return "icmp sge";
    case Opcode::Br:
        return "br";
    case Opcode::CondBr:
        return "br";
    case Opcode::Ret:
        return "ret";
    case Opcode::Call:
        return "call";
    case Opcode::Alloca:
        return "alloca";
    case Opcode::Load:
        return "load";
    case Opcode::Store:
        return "store";
    case Opcode::Phi:
        return "phi";
    default:
        return "unknown";
    }
}

/** @brief Prints a single instruction as a human-readable string. */
String IRPrinter::print_instruction(const Instruction* inst) {
    String result;
    const String& name = inst->get_name();
    bool has_result = !name.empty() && !inst->is_terminator() && !inst->is_memory_op();

    if (auto* phi = dynamic_cast<const PHINode*>(inst)) {
        result.append("%");
        result.append(name);
        result.append(" = phi ");
        result.append(inst->get_type()->print());
        for (unsigned i = 0; i < phi->incoming_count(); ++i) {
            result.append(", [ ");
            result.append(print_value(phi->incoming_value(i)));
            result.append(", %");
            result.append(phi->incoming_block(i)->get_name());
            result.append(" ]");
        }
        return result;
    }

    if (has_result) {
        result.append("%");
        result.append(name);
        result.append(" = ");
    }

    result.append(opcodeToString(inst->get_opcode()));

    if (inst->get_type()) {
        result.append(" ");
        result.append(inst->get_type()->print());
    }

    for (unsigned i = 0; i < inst->operand_count(); ++i) {
        result.append(", ");
        result.append(print_value(inst->operand(i)));
    }

    return result;
}

/** @brief Prints a value as a string, formatting constants inline and named values as "%name". */
String IRPrinter::print_value(const Value* val) {
    if (!val) return "<null>";

    if (auto* ci = dynamic_cast<const ConstantInt*>(val)) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(ci->get_value()));
        return String(buf, static_cast<size_t>(len));
    }
    if (auto* cf = dynamic_cast<const ConstantFP*>(val)) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%f", cf->get_value());
        return String(buf, static_cast<size_t>(len));
    }

    const String& name = val->get_name();
    if (name.empty()) return "<unnamed>";
    return String("%") + name;
}

}  // namespace prism::ir
