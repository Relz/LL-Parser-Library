#ifndef LLPARSERLIBRARYEXAMPLE_LLVMHELPER_H
#define LLPARSERLIBRARYEXAMPLE_LLVMHELPER_H

#include <llvm/IR/Constant.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>

class LlvmHelper
{
public:
	static llvm::Type * CreateType(llvm::LLVMContext & context, std::string const & type);
	static llvm::Constant * CreateConstant(llvm::LLVMContext & context, std::string const & type, std::string const & value);
	static llvm::Constant * CreateBooleanConstant(llvm::LLVMContext & context, bool value);
	static llvm::Constant * CreateBooleanConstant(llvm::LLVMContext & context, std::string const & value);
	static llvm::Constant * CreateCharacterConstant(llvm::LLVMContext & context, char value);
	static llvm::Constant * CreateIntegerConstant(llvm::LLVMContext & context, int value);
	static llvm::Constant * CreateFloatConstant(llvm::LLVMContext & context, double value);
	static llvm::Value * CreateAdd(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name);
	static llvm::Value * CreateSub(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name);
	static llvm::Value * CreateMul(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name);
	static llvm::Value * CreateSDiv(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name);
	static llvm::Value * CreateExactSDiv(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name);
	static llvm::Value * CreateSRem(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name);
	static llvm::Value * ConvertToFloat(llvm::IRBuilder<> * builder, llvm::Value * value);
	static llvm::Value * ConvertToInteger(llvm::IRBuilder<> * builder, llvm::Value * value);
private:
	LlvmHelper() = default;
};

#endif
