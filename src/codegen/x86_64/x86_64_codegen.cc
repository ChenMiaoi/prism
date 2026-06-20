#include "prism/codegen/x86_64/x86_64_codegen.hpp"

#include "prism/ir/basic_block.hpp"
#include "prism/ir/constant.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/module.hpp"
#include "prism/ir/type.hpp"

namespace prism::codegen {

/** @brief Generates x86-64 assembly text for the given IR module.
 *  @param module The IR module to generate assembly for.
 *  @return A String containing the complete assembly output.
 */
String X86_64CodeGenerator::generate(ir::Module* module) {
    output_ = ".text\n";
    output_ += ".globl _main\n\n";
    label_count_ = 0;

    for (size_t i = 0; i < module->functions().size(); ++i) {
        emit_function(module->functions()[i].get());
    }

    return output_;
}

/** @brief Emits assembly for a single function, including prologue, basic blocks, and stack layout.
 *  @param fn The IR function to emit.
 */
void X86_64CodeGenerator::emit_function(ir::Function* fn) {
    stack_offsets_.clear();
    stack_size_ = 0;

    current_function_label_ = function_label(fn);
    output_ += current_function_label_;
    output_ += ":\n";
    for (size_t block = 0; block < fn->blocks().size(); ++block) {
        for (size_t inst = 0; inst < fn->blocks()[block]->instructions().size(); ++inst) {
            const String& name = fn->blocks()[block]->instructions()[inst]->get_name();
            if (!name.empty()) get_stack_offset(name);
        }
    }
    emit_prologue(fn);

    for (size_t i = 0; i < fn->blocks().size(); ++i) {
        emit_basic_block(fn->blocks()[i].get());
    }

    output_ += "\n";
}

/** @brief Emits assembly for a single basic block, including its label and instructions.
 *  @param bb The basic block to emit.
 */
void X86_64CodeGenerator::emit_basic_block(ir::BasicBlock* bb) {
    if (!bb->get_name().empty()) {
        output_ += block_label(bb);
        output_ += ":\n";
    }
    for (size_t i = 0; i < bb->instructions().size(); ++i) {
        emit_instruction(bb->instructions()[i].get());
    }
}

/** @brief Emits x86-64 assembly for a single IR instruction.
 *  @param inst The IR instruction to emit.
 */
void X86_64CodeGenerator::emit_instruction(ir::Instruction* inst) {
    switch (inst->get_opcode()) {
    case ir::Opcode::Add:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    addq " + value_operand(inst->operand(1)) + ", %rax\n";
        output_ += "    movq %rax, " + stack_slot(inst->get_name()) + "\n";
        break;
    case ir::Opcode::Sub:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    subq " + value_operand(inst->operand(1)) + ", %rax\n";
        output_ += "    movq %rax, " + stack_slot(inst->get_name()) + "\n";
        break;
    case ir::Opcode::Mul:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    imulq " + value_operand(inst->operand(1)) + ", %rax\n";
        output_ += "    movq %rax, " + stack_slot(inst->get_name()) + "\n";
        break;
    case ir::Opcode::SDiv:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    cqto\n";
        output_ += "    idivq " + value_operand(inst->operand(1)) + "\n";
        output_ += "    movq %rax, " + stack_slot(inst->get_name()) + "\n";
        break;
    case ir::Opcode::SRem:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    cqto\n";
        output_ += "    idivq " + value_operand(inst->operand(1)) + "\n";
        output_ += "    movq %rdx, " + stack_slot(inst->get_name()) + "\n";
        break;
    case ir::Opcode::ICmpEQ:
    case ir::Opcode::ICmpNE:
    case ir::Opcode::ICmpSLT:
    case ir::Opcode::ICmpSGT:
    case ir::Opcode::ICmpSLE:
    case ir::Opcode::ICmpSGE: {
        const char* setcc = "sete";
        if (inst->get_opcode() == ir::Opcode::ICmpNE)
            setcc = "setne";
        else if (inst->get_opcode() == ir::Opcode::ICmpSLT)
            setcc = "setl";
        else if (inst->get_opcode() == ir::Opcode::ICmpSGT)
            setcc = "setg";
        else if (inst->get_opcode() == ir::Opcode::ICmpSLE)
            setcc = "setle";
        else if (inst->get_opcode() == ir::Opcode::ICmpSGE)
            setcc = "setge";
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    cmpq " + value_operand(inst->operand(1)) + ", %rax\n";
        output_ += "    ";
        output_ += setcc;
        output_ += " %al\n";
        output_ += "    movzbq %al, %rax\n";
        output_ += "    movq %rax, " + stack_slot(inst->get_name()) + "\n";
        break;
    }
    case ir::Opcode::Ret:
        if (inst->operand_count() > 0 && inst->operand(0)) {
            output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        }
        output_ += "    movq %rbp, %rsp\n";
        output_ += "    popq %rbp\n";
        output_ += "    ret\n";
        break;
    case ir::Opcode::Br:
        output_ += "    jmp " + block_label(static_cast<ir::BasicBlock*>(inst->operand(0))) + "\n";
        break;
    case ir::Opcode::CondBr:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    testq %rax, %rax\n";
        output_ += "    jne " + block_label(static_cast<ir::BasicBlock*>(inst->operand(1))) + "\n";
        output_ += "    jmp " + block_label(static_cast<ir::BasicBlock*>(inst->operand(2))) + "\n";
        break;
    case ir::Opcode::Store:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    movq %rax, " + stack_slot(inst->operand(1)->get_name()) + "\n";
        break;
    case ir::Opcode::Load:
        output_ += "    movq " + value_operand(inst->operand(0)) + ", %rax\n";
        output_ += "    movq %rax, " + stack_slot(inst->get_name()) + "\n";
        break;
    case ir::Opcode::Call: {
        static const char* arg_regs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
        auto* callee = static_cast<ir::Function*>(inst->operand(0));
        for (unsigned i = 1; i < inst->operand_count() && i <= 6; ++i) {
            output_ += "    movq " + value_operand(inst->operand(i)) + ", ";
            output_ += arg_regs[i - 1];
            output_ += "\n";
        }
        output_ += "    call " + function_label(callee) + "\n";
        if (!inst->get_name().empty()) output_ += "    movq %rax, " + stack_slot(inst->get_name()) + "\n";
        break;
    }
    default:
        output_ += "    # unhandled opcode\n";
        break;
    }
}

/** @brief Emits the function prologue: saves %rbp and allocates stack space.
 *  @param fn The function whose prologue is being emitted.
 */
void X86_64CodeGenerator::emit_prologue(ir::Function* fn) {
    (void)fn;
    output_ += "    pushq %rbp\n";
    output_ += "    movq %rsp, %rbp\n";
    int total = stack_size_ * 8;
    if (total > 0) {
        char buf[32];
        ::snprintf(buf, sizeof(buf), "    subq $%d, %%rsp\n", total);
        output_ += buf;
    }
}

/** @brief Emits the function epilogue: restores %rbp and %rsp.
 *  @param fn The function whose epilogue is being emitted.
 */
void X86_64CodeGenerator::emit_epilogue(ir::Function* fn) {
    (void)fn;
    output_ += "    movq %rbp, %rsp\n";
    output_ += "    popq %rbp\n";
}

/** @brief Returns the assembly label for a function (prefixed with '_').
 *  @param fn The IR function.
 *  @return The assembly label string.
 */
String X86_64CodeGenerator::function_label(ir::Function* fn) const {
    return String("_") + fn->get_name();
}

/** @brief Returns the assembly label for a basic block within the current function.
 *  @param bb The basic block.
 *  @return The assembly label string.
 */
String X86_64CodeGenerator::block_label(ir::BasicBlock* bb) const {
    return current_function_label_ + String(".") + bb->get_name();
}

/** @brief Converts an IR value to its x86-64 operand string (register, constant, or stack slot).
 *  @param value The IR value to convert.
 *  @return A string representing the operand in AT&T assembly syntax.
 */
String X86_64CodeGenerator::value_operand(ir::Value* value) {
    if (auto* argument = dynamic_cast<ir::Argument*>(value)) {
        static const char* arg_regs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
        if (argument->get_arg_no() < 6) return String(arg_regs[argument->get_arg_no()]);
    }
    if (auto* constant = dynamic_cast<ir::ConstantInt*>(value)) {
        char buf[32];
        ::snprintf(buf, sizeof(buf), "$%lld", static_cast<long long>(constant->get_value()));
        return String(buf);
    }
    return stack_slot(value->get_name());
}

/** @brief Returns the stack slot string for a named value (e.g., "-8(%rbp)").
 *  @param name The name of the value.
 *  @return The AT&T memory operand string.
 */
String X86_64CodeGenerator::stack_slot(const String& name) {
    int offset = get_stack_offset(name);
    char buf[32];
    ::snprintf(buf, sizeof(buf), "%d(%%rbp)", offset);
    return String(buf);
}

/** @brief Returns the stack offset for a named value, allocating a slot if needed.
 *  @param name The name of the value.
 *  @return The byte offset from %rbp.
 */
int X86_64CodeGenerator::get_stack_offset(const String& name) {
    auto* entry = stack_offsets_.find(name);
    if (entry) return entry->value;
    return allocate_stack_slot(name);
}

/** @brief Allocates a new 8-byte stack slot for a named value.
 *  @param name The name of the value.
 *  @return The byte offset from %rbp for the new slot.
 */
int X86_64CodeGenerator::allocate_stack_slot(const String& name) {
    ++stack_size_;
    int offset = -(stack_size_ * 8);
    stack_offsets_[name] = offset;
    return offset;
}

}  // namespace prism::codegen
