#include "prism/ir/function.hpp"

#include "prism/ir/context.hpp"
#include "prism/ir/module.hpp"

#include <cstdio>

namespace prism::ir {

/** @brief Constructs an Argument with a type, name, and parent function. */
Argument::Argument(Type* ty, const char* name, Function* parent) : Value(ty, name), parent_(parent) {
}

/** @brief Constructs a Function with the given type, name, and parent module. */
Function::Function(FunctionType* ty, const char* name, Module* module)
    : User(ty, 0, name), func_type_(ty), module_(module), is_declaration_(true) {
    for (unsigned i = 0; i < ty->param_types().size(); ++i) {
        char arg_name[16];
        ::snprintf(arg_name, sizeof(arg_name), "arg%u", i);
        auto arg = prism::make_unique<Argument>(ty->param_types()[i], arg_name, this);
        arg->set_arg_no(i);
        args_.push_back(static_cast<UniquePtr<Argument>&&>(arg));
    }
}

/** @brief Creates a new basic block within this function and returns a pointer to it. */
BasicBlock* Function::create_block(const char* name) {
    is_declaration_ = false;
    auto block = prism::make_unique<BasicBlock>(module_->get_context(), name, this);
    BasicBlock* ptr = block.get();
    blocks_.push_back(static_cast<UniquePtr<BasicBlock>&&>(block));
    return ptr;
}

/** @brief Prints the function definition or declaration as a string. */
String Function::print() const {
    String result;
    result.append(func_type_->return_type()->print());
    result.append(" @");
    result.append(get_name());
    result.append("(");

    for (size_t i = 0; i < args_.size(); ++i) {
        if (i > 0) result.append(", ");
        result.append(args_[i]->get_type()->print());
    }
    result.append(")");

    if (is_declaration_) {
        result.append(";");
        return result;
    }

    result.append(" {\n");
    for (size_t i = 0; i < blocks_.size(); ++i) {
        result.append(blocks_[i]->print());
    }
    result.append("}");
    return result;
}

}  // namespace prism::ir
