cmake_minimum_required(VERSION 3.22)
project(prism LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PRISM_ENABLE_LLVM "Enable LLVM IR backend" OFF)
option(PRISM_ENABLE_RISCV "Enable RISC-V backend" OFF)

function(prism_enable_strict_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Werror
        )
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    endif()
endfunction()

set(PRISM_BASIC_SOURCES
    src/basic/diagnostics.cc
    src/basic/error.cc
)

set(PRISM_AST_SOURCES
    src/ast/ast_context.cc
    src/ast/decl.cc
    src/ast/stmt.cc
    src/ast/expr.cc
    src/ast/ast_dumper.cc
)

set(PRISM_FRONTEND_SOURCES
    src/frontend/lexer.cc
    src/frontend/parser.cc
)

set(PRISM_SEMA_SOURCES
    src/sema/semantic_analyzer.cc
    src/sema/type_checker.cc
)

set(PRISM_IR_SOURCES
    src/ir/context.cc
    src/ir/type.cc
    src/ir/value.cc
    src/ir/use.cc
    src/ir/user.cc
    src/ir/constant.cc
    src/ir/instruction.cc
    src/ir/basic_block.cc
    src/ir/function.cc
    src/ir/module.cc
    src/ir/ir_builder.cc
    src/ir/ir_printer.cc
    src/ir/verifier.cc
)

set(PRISM_CODEGEN_SOURCES
    src/codegen/ast_lowering.cc
    src/codegen/x86_64/x86_64_codegen.cc
    src/codegen/llvm/llvm_codegen.cc
)

set(PRISM_OPT_SOURCES
    src/opt/pass_manager.cc
)

set(PRISM_RUNTIME_SOURCES
    src/runtime/runtime.cc
)

add_executable(prism
    ${PRISM_BASIC_SOURCES}
    ${PRISM_AST_SOURCES}
    ${PRISM_FRONTEND_SOURCES}
    ${PRISM_SEMA_SOURCES}
    ${PRISM_IR_SOURCES}
    ${PRISM_CODEGEN_SOURCES}
    ${PRISM_OPT_SOURCES}
    ${PRISM_RUNTIME_SOURCES}
    src/driver/compiler.cc
)

target_include_directories(prism PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
prism_enable_strict_warnings(prism)

if(PRISM_ENABLE_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    target_compile_definitions(prism PRIVATE PRISM_ENABLE_LLVM=1)
    target_include_directories(prism PRIVATE ${LLVM_INCLUDE_DIRS})
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
    target_compile_options(prism PRIVATE ${LLVM_DEFINITIONS_LIST})
    llvm_map_components_to_libnames(llvm_libs core support irreader)
    target_link_libraries(prism PRIVATE ${llvm_libs})
endif()

if(PRISM_ENABLE_RISCV)
    target_sources(prism PRIVATE src/codegen/riscv/riscv_codegen.cc)
endif()

enable_testing()

add_executable(prism_tests
    ${PRISM_BASIC_SOURCES}
    ${PRISM_AST_SOURCES}
    ${PRISM_FRONTEND_SOURCES}
    ${PRISM_SEMA_SOURCES}
    ${PRISM_IR_SOURCES}
    ${PRISM_CODEGEN_SOURCES}
    ${PRISM_OPT_SOURCES}
    tests/unit/test_all.cc
    ${PRISM_RUNTIME_SOURCES}
)

target_include_directories(prism_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
prism_enable_strict_warnings(prism_tests)

if(PRISM_ENABLE_LLVM)
    target_compile_definitions(prism_tests PRIVATE PRISM_ENABLE_LLVM=1)
    target_include_directories(prism_tests PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_options(prism_tests PRIVATE ${LLVM_DEFINITIONS_LIST})
    target_link_libraries(prism_tests PRIVATE ${llvm_libs})
endif()

add_test(NAME unit_tests COMMAND prism_tests)
