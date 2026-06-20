#pragma once

#include "prism/basic/source_location.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"

namespace prism::ast {

class Expr;
class VarDecl;

/** @brief Enumerates the kinds of declaration AST nodes. */
enum class DeclKind {
    TranslationUnit,  ///< Top-level translation unit
    Function,         ///< Function declaration
    Variable,         ///< Variable declaration
    Parameter,        ///< Function parameter declaration
    Class,            ///< Class declaration
    Struct,           ///< Struct declaration
    Template,         ///< Template declaration
    TemplateParam,    ///< Template parameter declaration
};

/** @brief Base class for all declaration AST nodes. */
class Decl {
public:
    virtual ~Decl() = default;

    /** @brief Returns the declaration kind discriminator.
     *  @return The DeclKind for this node. */
    DeclKind kind() const { return kind_; }

    /** @brief Returns the source location of this declaration.
     *  @return The SourceLocation where this declaration appears. */
    SourceLocation loc() const { return loc_; }

    /** @brief Returns the name of this declaration.
     *  @return A reference to the name string. */
    const prism::String& name() const { return name_; }

protected:
    /** @brief Constructs a declaration with a kind, name, and source location.
     *  @param kind The declaration kind.
     *  @param name The declaration name.
     *  @param loc  The source location. */
    Decl(DeclKind kind, const char* name, SourceLocation loc = SourceLocation::invalid())
        : kind_(kind), name_(name), loc_(loc) {}

private:
    DeclKind kind_;
    prism::String name_;
    SourceLocation loc_;
};

/** @brief AST node for a top-level translation unit. */
class TranslationUnitDecl final : public Decl {
public:
    /** @brief Constructs a translation unit declaration. */
    TranslationUnitDecl() : Decl(DeclKind::TranslationUnit, "") {}

    /** @brief Returns the immutable list of top-level declarations.
     *  @return Const reference to the vector of declarations. */
    const prism::Vector<Decl*>& decls() const { return decls_; }

    /** @brief Returns the mutable list of top-level declarations.
     *  @return Reference to the vector of declarations. */
    prism::Vector<Decl*>& decls() { return decls_; }

    /** @brief Appends a declaration to this translation unit.
     *  @param decl The declaration to add. */
    void add_decl(Decl* decl) { decls_.push_back(decl); }

private:
    prism::Vector<Decl*> decls_;
};

/** @brief AST node for function parameter declarations. */
class ParamDecl final : public Decl {
public:
    /** @brief Constructs a parameter declaration.
     *  @param type_name The parameter type name.
     *  @param param_name The parameter name.
     *  @param loc        The source location. */
    ParamDecl(const char* type_name, const char* param_name, SourceLocation loc = SourceLocation::invalid())
        : Decl(DeclKind::Parameter, param_name, loc), type_name_(type_name) {}

    /** @brief Returns the parameter type name.
     *  @return A reference to the type name string. */
    const prism::String& type_name() const { return type_name_; }

private:
    prism::String type_name_;
};

/** @brief AST node for function declarations. */
class FunctionDecl final : public Decl {
public:
    /** @brief Constructs a function declaration.
     *  @param return_type The return type name.
     *  @param func_name   The function name.
     *  @param params      The parameter declarations.
     *  @param loc         The source location. */
    FunctionDecl(const char* return_type, const char* func_name, prism::Vector<ParamDecl*> params,
                 SourceLocation loc = SourceLocation::invalid())
        : Decl(DeclKind::Function, func_name, loc)
        , return_type_(return_type)
        , params_(static_cast<prism::Vector<ParamDecl*>&&>(params)) {}

    /** @brief Returns the return type name.
     *  @return A reference to the return type string. */
    const prism::String& return_type() const { return return_type_; }

    /** @brief Returns the parameter declarations.
     *  @return A reference to the vector of parameter declarations. */
    const prism::Vector<ParamDecl*>& params() const { return params_; }

    /** @brief Returns the function body statement.
     *  @return Pointer to the body statement, or nullptr if not yet set. */
    class Stmt* body() const { return body_; }

    /** @brief Sets the function body statement.
     *  @param stmt The statement to use as the function body. */
    void set_body(Stmt* stmt) { body_ = stmt; }

    /** @brief Returns whether this function has a body.
     *  @return True if a body has been set. */
    bool has_body() const { return body_ != nullptr; }

private:
    prism::String return_type_;
    prism::Vector<ParamDecl*> params_;
    Stmt* body_ = nullptr;
};

/** @brief AST node for class and struct declarations. */
class RecordDecl final : public Decl {
public:
    /** @brief Constructs a record (class/struct) declaration.
     *  @param is_class    True for class, false for struct.
     *  @param record_name The record name.
     *  @param loc         The source location. */
    RecordDecl(bool is_class, const char* record_name, SourceLocation loc = SourceLocation::invalid())
        : Decl(is_class ? DeclKind::Class : DeclKind::Struct, record_name, loc), is_class_(is_class) {}

    /** @brief Returns whether this record is a class (not a struct).
     *  @return True if this is a class declaration. */
    bool is_class() const { return is_class_; }

    /** @brief Returns the field declarations.
     *  @return A reference to the vector of field declarations. */
    const prism::Vector<VarDecl*>& fields() const { return fields_; }

    /** @brief Adds a field declaration to this record.
     *  @param field The field declaration to add. */
    void add_field(VarDecl* field) { fields_.push_back(field); }

    /** @brief Marks that this record has a default constructor.
     *  @param loc The source location of the constructor. */
    void set_default_constructor(SourceLocation loc) {
        has_default_constructor_ = true;
        constructor_loc_ = loc;
    }

    /** @brief Marks that this record has a destructor.
     *  @param loc The source location of the destructor. */
    void set_destructor(SourceLocation loc) {
        has_destructor_ = true;
        destructor_loc_ = loc;
    }

    /** @brief Returns whether this record has a default constructor.
     *  @return True if a default constructor is present. */
    bool has_default_constructor() const { return has_default_constructor_; }

    /** @brief Returns whether this record has a destructor.
     *  @return True if a destructor is present. */
    bool has_destructor() const { return has_destructor_; }

    /** @brief Returns the source location of the default constructor.
     *  @return The constructor's SourceLocation. */
    SourceLocation constructor_loc() const { return constructor_loc_; }

    /** @brief Returns the source location of the destructor.
     *  @return The destructor's SourceLocation. */
    SourceLocation destructor_loc() const { return destructor_loc_; }

private:
    bool is_class_;
    prism::Vector<VarDecl*> fields_;
    bool has_default_constructor_ = false;
    bool has_destructor_ = false;
    SourceLocation constructor_loc_ = SourceLocation::invalid();
    SourceLocation destructor_loc_ = SourceLocation::invalid();
};

/** @brief AST node for variable declarations. */
class VarDecl final : public Decl {
public:
    /** @brief Constructs a variable declaration.
     *  @param type_name The type name string.
     *  @param var_name  The variable name string.
     *  @param init      The initializer expression, or nullptr.
     *  @param loc       The source location. */
    VarDecl(const char* type_name, const char* var_name, Expr* init = nullptr,
            SourceLocation loc = SourceLocation::invalid())
        : Decl(DeclKind::Variable, var_name, loc), type_name_(type_name), init_(init) {}

    /** @brief Returns the variable type name.
     *  @return A reference to the type name string. */
    const prism::String& type_name() const { return type_name_; }

    /** @brief Returns the initializer expression.
     *  @return Pointer to the init expression, or nullptr. */
    Expr* init_expr() const { return init_; }

private:
    prism::String type_name_;
    Expr* init_ = nullptr;
};

}  // namespace prism::ast
