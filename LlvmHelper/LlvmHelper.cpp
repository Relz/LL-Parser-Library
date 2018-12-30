#include <llvm/IR/Constants.h>
#include "LlvmHelper.h"
#include "../LexerLibrary/TokenLibrary/TokenConstant/TokenConstant.h"

llvm::Type * LlvmHelper::CreateType(llvm::LLVMContext & context, std::string const & type, std::string const & arraySizeString)
{
	llvm::Type * elementType;
	if (type == TokenConstant::CoreType::Number::INTEGER)
	{
		elementType = llvm::Type::getInt32Ty(context);
	}
	else if (type == TokenConstant::CoreType::Number::FLOAT)
	{
		elementType = llvm::Type::getDoubleTy(context);
	}
	else if (type == TokenConstant::CoreType::CHARACTER)
	{
		elementType = llvm::Type::getInt8Ty(context);
	}
	else if (type == TokenConstant::CoreType::BOOLEAN)
	{
		elementType = llvm::Type::getInt8Ty(context);
	}
	else if (type == TokenConstant::CoreType::VOID)
	{
		elementType = llvm::Type::getVoidTy(context);
	}
	else if (type == TokenConstant::CoreType::Complex::STRING)
	{
		elementType = llvm::ArrayType::getInt8PtrTy(context);
	}
	else
	{
		throw std::runtime_error("LlvmHelper::CreateType: Unsupported type \"" + type + "\"");
	}
	if (arraySizeString.empty())
	{
		return elementType;
	}
	return llvm::ArrayType::get(elementType, stoi(arraySizeString));
}

llvm::Constant * LlvmHelper::CreateConstant(llvm::LLVMContext & context, std::string const & type, std::string const & value)
{
	if (type == TokenConstant::CoreType::Number::INTEGER)
	{
		return LlvmHelper::CreateInteger32Constant(context, std::stoi(value));
	}
	else if (type == TokenConstant::CoreType::Number::FLOAT)
	{
		return LlvmHelper::CreateFloatConstant(context, std::stof(value));
	}
	else if (type == TokenConstant::CoreType::BOOLEAN)
	{
		return LlvmHelper::CreateBooleanConstant(context, value);
	}
	else if (type == TokenConstant::CoreType::CHARACTER)
	{
		return LlvmHelper::CreateCharacterConstant(context, value[1]);
	}
	throw std::runtime_error("LlvmHelper::CreateConstant: Unsupported type \"" + type + "\"");
}

llvm::Constant * LlvmHelper::CreateBooleanConstant(llvm::LLVMContext & context, bool value)
{
	return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), value, false);
}

llvm::Constant * LlvmHelper::CreateBooleanConstant(llvm::LLVMContext & context, std::string const & value)
{
	if (value == "True")
	{
		return LlvmHelper::CreateBooleanConstant(context, true);
	}
	else if (value == "False")
	{
		return LlvmHelper::CreateBooleanConstant(context, false);
	}
	throw std::runtime_error("LlvmHelper::CreateConstant: \"" + value + "\"" + " is not boolean literal, possible values: \"True\", \"False\"");
}

llvm::Constant * LlvmHelper::CreateArrayConstant(llvm::LLVMContext & context, llvm::Type * arrayLiteralElementType, std::vector<llvm::Value *> const & arrayLiteralValues)
{
	std::vector<llvm::Constant*> values;
	for (llvm::Value * arrayLiteralValue : arrayLiteralValues)
	{
		values.emplace_back((llvm::Constant*)arrayLiteralValue);
	}

	return llvm::ConstantArray::get((llvm::ArrayType*)arrayLiteralElementType, values);
}

llvm::Constant * LlvmHelper::CreateCharacterConstant(llvm::LLVMContext & context, char value)
{
	return llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), value, false);
}

llvm::Constant * LlvmHelper::CreateInteger32Constant(llvm::LLVMContext & context, int value)
{
	return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), value, true);
}

llvm::Constant * LlvmHelper::CreateInteger64Constant(llvm::LLVMContext & context, int value)
{
	return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), value, true);
}

llvm::Constant * LlvmHelper::CreateFloatConstant(llvm::LLVMContext & context, double value)
{
	return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), value);
}

llvm::Value * LlvmHelper::CreateAdd(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::CoreType::Number::INTEGER)
	{
		return builder->CreateAdd(lhs, rhs, name);
	}
	else if (type == TokenConstant::CoreType::Number::FLOAT)
	{
		lhs = ConvertToFloat(builder, lhs);
		rhs = ConvertToFloat(builder, rhs);

		return builder->CreateFAdd(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateAdd: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateSub(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::CoreType::Number::INTEGER)
	{
		return builder->CreateSub(lhs, rhs, name);
	}
	else if (type == TokenConstant::CoreType::Number::FLOAT)
	{
		lhs = ConvertToFloat(builder, lhs);
		rhs = ConvertToFloat(builder, rhs);

		return builder->CreateFSub(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateSub: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateMul(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::CoreType::Number::INTEGER)
	{
		return builder->CreateMul(lhs, rhs, name);
	}
	else if (type == TokenConstant::CoreType::Number::FLOAT)
	{
		lhs = ConvertToFloat(builder, lhs);
		rhs = ConvertToFloat(builder, rhs);

		return builder->CreateFMul(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateMul: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateSDiv(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::CoreType::Number::FLOAT)
	{
		lhs = ConvertToFloat(builder, lhs);
		rhs = ConvertToFloat(builder, rhs);

		return builder->CreateFDiv(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateSDiv: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateExactSDiv(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::CoreType::Number::INTEGER)
	{
		lhs = ConvertToInteger(builder, lhs);
		rhs = ConvertToInteger(builder, rhs);

		return builder->CreateExactSDiv(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateExactSDiv: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::CreateSRem(llvm::IRBuilder<> * builder, std::string const & type, llvm::Value * lhs, llvm::Value * rhs, std::string const & name)
{
	if (type == TokenConstant::CoreType::Number::INTEGER)
	{
		return builder->CreateSRem(lhs, rhs, name);
	}
	throw std::runtime_error("LlvmHelper::CreateSRem: Unsupported type \"" + type + "\"");
}

llvm::Value * LlvmHelper::ConvertToFloat(llvm::IRBuilder<> * builder, llvm::Value * value)
{
	return builder->CreateSIToFP(value, llvm::Type::getDoubleTy(builder->getContext()), "conversion_to_float");
}

llvm::Value * LlvmHelper::ConvertToInteger(llvm::IRBuilder<> * builder, llvm::Value * value)
{
	return builder->CreateFPToSI(value, llvm::Type::getInt32Ty(builder->getContext()), "conversion_to_integer");
}
