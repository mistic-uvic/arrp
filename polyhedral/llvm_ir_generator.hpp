#ifndef STREAM_POLYHEDRAL_LLVM_IR_GENERATOR_INCLUDED
#define STREAM_POLYHEDRAL_LLVM_IR_GENERATOR_INCLUDED

#include "model.hpp"
#include "../frontend/context.hpp"

#include <isl/set.h>
#include <isl/union_set.h>
#include <isl/map.h>
#include <isl/union_map.h>
#include <isl/ast_build.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

#include <unordered_map>
#include <string>

namespace stream {
namespace polyhedral {

using std::unordered_map;
using std::string;

class llvm_ir_generator
{
public:
    llvm_ir_generator(const string & module_name);

    void generate( isl_ast_node *ast,
                   const unordered_map<string, statement*> & source );

private:
    using value_type = llvm::Value*;
    using type_type = llvm::Type*;
    using block_type = llvm::BasicBlock*;
    using context = stream::context<string, value_type>;

    void process_node(isl_ast_node*);
    void process_block(isl_ast_node*);
    static int process_block_element(isl_ast_node * node, void * data);
    void process_if(isl_ast_node*);
    void process_for(isl_ast_node*);
    void process_statement(isl_ast_node*);
    value_type process_expression(isl_ast_expr*);
    value_type process_op(isl_ast_expr*);
    void process_conditional(isl_ast_expr*,
                             block_type true_block,
                             block_type false_block );
    type_type bool_type()
    {
        return llvm::Type::getInt1Ty(llvm_context());
    }
    type_type int32_type()
    {
        return llvm::Type::getInt32Ty(llvm_context());
    }
    type_type double_type()
    {
        return llvm::Type::getDoubleTy(llvm_context());
    }
    value_type bool_value( bool value )
    {
        return llvm::ConstantInt::get(bool_type(), value ? 1 : 0);
    }
    value_type int32_value( int value )
    {
        return llvm::ConstantInt::getSigned(int32_type(), value);
    }
    value_type double_value( double value )
    {
        return llvm::ConstantFP::get(double_type(), value);
    }
    block_type add_block( const string & name )
    {
        auto parent = m_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock::Create(llvm_context(), name, parent);
    }


    llvm::LLVMContext & llvm_context() { return m_module.getContext(); }

    llvm::Module m_module;
    llvm::IRBuilder<> m_builder;
    context m_ctx;
};

}
}

#endif // STREAM_POLYHEDRAL_LLVM_IR_GENERATOR_INCLUDED
