#include <llvm/IR/Constants.h>
#include "LlvmHelper.h"
#include "../LexerLibrary/TokenLibrary/TokenConstant/TokenConstant.h"

llvm::Type * LlvmHelper::CreateType(llvm::LLVMContext & context, std::string const & type)
{
	if (type == TokenConstant::Name::INTEGER)
	{
		return llvm::Type::getInt32Ty(context);
	}
	else if (type == TokenConstant::Name::FLOAT)
	{
		return llvm::Type::getFloatTy(context);
	}
	throw std::runtime_error("LlvmHelper::CreateType: Unsupported type \"" + type + "\"");
}

llvm::Constant * LlvmHelper::CreateConstant(llvm::LLVMContext & context, std::string const & type, std::string const & value)
{
	if (type == TokenConstant::Name::INTEGER)
	{
		return LlvmHelper::CreateIntegerConstant(context, std::stoi(value));
	}
	else if (type == TokenConstant::Name::FLOAT)
	{
		return LlvmHelper::CreateFloatConstant(context, std::stof(value));
	}
	throw std::runtime_error("LlvmHelper::CreateConstant: Unsupported type \"" + type + "\"");
}

llvm::Constant * LlvmHelper::CreateIntegerConstant(llvm::LLVMContext & context, int value)
{
	return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), value, true);
}

llvm::Constant * LlvmHelper::CreateFloatConstant(llvm::LLVMContext & context, float value)
{
	return llvm::ConstantFP::get(llvm::Type::getFloatTy(context), value);
}

llvm::Value * LlvmHelper::CreateAdd(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::Name::INTEGER)
	{
		return builder->CreateAdd(lhs, rhs, name);
	}
	else if (type == TokenConstant::Name::FLOAT)
	{
		return builder->CreateFAdd(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateAdd: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateSub(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::Name::INTEGER)
	{
		return builder->CreateSub(lhs, rhs, name);
	}
	else if (type == TokenConstant::Name::FLOAT)
	{
		return builder->CreateFSub(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateSub: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateMul(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::Name::INTEGER)
	{
		return builder->CreateMul(lhs, rhs, name);
	}
	else if (type == TokenConstant::Name::FLOAT)
	{
		return builder->CreateFMul(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateMul: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateSDiv(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::Name::FLOAT)
	{
		return builder->CreateSDiv(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateSDiv: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateExactSDiv(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::Name::INTEGER)
	{
		return builder->CreateExactSDiv(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateExactSDiv: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateSRem(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::Name::INTEGER)
	{
		return builder->CreateSRem(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateSRem: Unsupported type \"" + type + "\"");
}
