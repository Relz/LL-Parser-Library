#ifndef LLPARSERLIBRARY_LIBRARY_H
#define LLPARSERLIBRARY_LIBRARY_H

#include "LLTableBuilderLibrary/LLTableBuilder.h"
#include "AstNode/AstNode.h"
#include "SymbolTable/SymbolTable.h"
#include "LexerLibrary/TokenLibrary/Token.h"
#include "LLTableBuilderLibrary/TokenLibrary/TokenExtensions/TokenExtensions.h"
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>

class TokenInformation;

class LLParser
{
public:
	explicit LLParser(std::string const & ruleFileName);

	bool IsValid(
		std::string const & inputFileName,
		std::vector<TokenInformation> & tokenInformations,
		size_t & failIndex,
		std::unordered_set<Token> & expectedWords
	);

private:
	AstNode * CreateAstNode(std::string const & ruleName, unsigned int tokenCount);

	bool TryToCreateAstNode(std::string const & actionName);

	static bool ParseCreateAstNodeAction(std::string const & actionName, std::string & ruleName, unsigned int & tokenCount);
	bool ResolveActionName(std::string const & actionName) const;
	bool ResolveAstActionName(std::string const & actionName);

	unsigned int FindRowIndexInScopeByName(std::string const & name) const;

	bool CreateScopeAction();
	bool DestroyScopeAction();
	void ComputeDimensions(AstNode * extendedType, std::vector<unsigned int> & dimensions);
	bool AddVariableToScope();
	bool UpdateVariableInScope();
	bool CheckIdentifierForAlreadyExisting() const;
	bool CheckIdentifierForExisting() const;
	bool Synthesis();
	bool SynthesisPlus();
	bool SynthesisMinus();
	bool SynthesisMultiply();
	bool SynthesisIntegerDivision();
	bool SynthesisDivision();
	bool SynthesisModulus();
	bool SynthesisEquivalence();
	bool SynthesisNotEquivalence();
	bool SynthesisMoreOrEquivalence();
	bool SynthesisLessOrEquivalence();
	bool SynthesisMore();
	bool SynthesisLess();
	bool CheckVariableTypeWithAssignmentRightHandTypeForEquality() const;
	bool CheckIdentifierTypeWithAssignmentRightHandTypeForEquality() const;
	bool SynthesisType();
	bool RemoveBrackets();
	bool RemoveBracketsAndSynthesisType();
	bool RemoveIfRoundBrackets();
	bool RemoveSemicolon();
	bool RemoveScopeBrackets();
	bool ExpandLastChildren();
	bool ExpandChildrenLastChildren();
	bool RemoveBracketsAndSynthesis();
	bool RemoveComma();
	bool RemovePredefinedFunctionReadOrWriteExtra();
	bool CreateLlvmReadFunction();
	bool CreateLlvmWriteFunction();
	bool CreateLlvmArrayAssignFunction(llvm::Value * allocaInst, std::string const & variableName, llvm::Type * variableType, int arraySize);
	bool SynthesisLastChildrenChildren();
	bool SynthesisLastChildren();
	bool RemoveIfOrWhileStatementExtra();
	bool CreateIfStatement();
	bool GotoPostIfStatementLabel();
	bool StartBlockTrue();
	bool StartBlockFalse();
	bool StartBlockPrevious();
	bool CreateWhileStatement();
	bool SynthesisIfOrWhileCondition();
	bool CreateBlockWhile();
	bool StartBlockWhile();
	bool CreateBlockPreWhile();
	bool GotoBlockPreWhile();
	bool StartBlockPreWhile();
	bool SynthesisIfOrWhileConditionAndRemoveEmptyElse();
	bool SavePostIfStatementToPreviousBlocks();
	bool EndBlockPreWhile();
	bool ExpandArrayLiteral();
	bool SynthesisIdentifierPossibleArrayAccessing();
	bool abc();

	bool CreateLlvmStringLiteral();
	bool CreateLlvmCharacterLiteral();
	bool CreateLlvmBooleanLiteral();
	void ComputeArrayLiteralValues(std::vector<AstNode*> const & astNodes, std::vector<llvm::Constant*> & arrayLiteralValues);
	void ComputeArrayLiteralName(AstNode * arrayLiteralNode, std::string & arrayLiteralName);
	bool CreateLlvmArrayLiteral();
	bool CreateLlvmIntegerValue();
	bool CreateLlvmFloatValue();
	bool TryToLoadLlvmValueFromSymbolTable();
	bool TryToReferenceLlvmValueFromSymbolTable();

	void PrintTokenInformations(
		std::vector<TokenInformation> const & tokenInformations, size_t from, size_t to, std::string const & color
	);
	void PrintColoredMessage(std::string const & message, std::string const & colorCode) const;
	void PrintWarningMessage(std::string const & message) const;
	void PrintErrorMessage(std::string const & message) const;

	bool AreTypesCompatible(std::string const & lhsType, std::string const & rhsType, std::string & resultType);
	bool IsUnaryMinus(std::string const & lhs);
	AstNode * CreateLiteralAstNode(std::string const & type, std::string const & value);
	llvm::Value * CreateCondition(std::string const & name);

	static llvm::Function * PrintfPrototype(llvm::LLVMContext & context, llvm::Module * module);
	static llvm::Function * ScanfPrototype(llvm::LLVMContext & context, llvm::Module * module);
	static llvm::Function * MemcpyPrototype(llvm::LLVMContext & context, llvm::Module * module);

	std::unordered_map<std::string, std::function<bool()>> const ACTION_NAME_TO_ACTION_MAP {
		{ "Create scope", std::bind(&LLParser::CreateScopeAction, this) },
		{ "Destroy scope", std::bind(&LLParser::DestroyScopeAction, this) },
		{ "Add variable to scope", std::bind(&LLParser::AddVariableToScope, this) },
		{ "Update variable in scope", std::bind(&LLParser::UpdateVariableInScope, this) },
		{ "Check identifier for already existing", std::bind(&LLParser::CheckIdentifierForAlreadyExisting, this) },
		{ "Check identifier for existing", std::bind(&LLParser::CheckIdentifierForExisting, this) },
		{ "Synthesis", std::bind(&LLParser::Synthesis, this) },
		{ "Check variable type with AssignmentRightHand type for equality", std::bind(&LLParser::CheckVariableTypeWithAssignmentRightHandTypeForEquality, this) },
		{ "Check identifier type with AssignmentRightHand type for equality", std::bind(&LLParser::CheckIdentifierTypeWithAssignmentRightHandTypeForEquality, this) },
		{ "Create LLVM string literal", std::bind(&LLParser::CreateLlvmStringLiteral, this) },
		{ "Create LLVM character literal", std::bind(&LLParser::CreateLlvmCharacterLiteral, this) },
		{ "Create LLVM boolean literal", std::bind(&LLParser::CreateLlvmBooleanLiteral, this) },
		{ "Create LLVM array literal", std::bind(&LLParser::CreateLlvmArrayLiteral, this) },
		{ "Create llvm integer value", std::bind(&LLParser::CreateLlvmIntegerValue, this) },
		{ "Create llvm float value", std::bind(&LLParser::CreateLlvmFloatValue, this) },
		{ "Try to load LLVM value from symbol table", std::bind(&LLParser::TryToLoadLlvmValueFromSymbolTable, this) },
		{ "Try to reference LLVM value from symbol table", std::bind(&LLParser::TryToReferenceLlvmValueFromSymbolTable, this) },
		{ "Create LLVM read function", std::bind(&LLParser::CreateLlvmReadFunction, this) },
		{ "Create LLVM write function", std::bind(&LLParser::CreateLlvmWriteFunction, this) },
		{ "Create if statement", std::bind(&LLParser::CreateIfStatement, this) },
		{ "Start block true", std::bind(&LLParser::StartBlockTrue, this) },
		{ "Goto post if statement label", std::bind(&LLParser::GotoPostIfStatementLabel, this) },
		{ "Start block false", std::bind(&LLParser::StartBlockFalse, this) },
		{ "Start block previous", std::bind(&LLParser::StartBlockPrevious, this) },
		{ "Create while statement", std::bind(&LLParser::CreateWhileStatement, this) },
		{ "Create block while", std::bind(&LLParser::CreateBlockWhile, this) },
		{ "Start block while", std::bind(&LLParser::StartBlockWhile, this) },
		{ "Create block pre while", std::bind(&LLParser::CreateBlockPreWhile, this) },
		{ "Goto block pre while", std::bind(&LLParser::GotoBlockPreWhile, this) },
		{ "Start block pre while", std::bind(&LLParser::StartBlockPreWhile, this) },
		{ "Save post if statement to previous blocks", std::bind(&LLParser::SavePostIfStatementToPreviousBlocks, this) },
		{ "End block pre while", std::bind(&LLParser::EndBlockPreWhile, this) },

		{ "Synthesis Plus Integer", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Integer B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Float", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Float B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Identifier", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Identifier B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String literal", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus String literal B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character literal", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character literal B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus Character B", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticMinus", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticMultiply", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticDivision", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisPlus, this) },
		{ "Synthesis Plus ArithmeticModule", std::bind(&LLParser::SynthesisPlus, this) },

		{ "Synthesis Minus Integer", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Integer B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Float", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Float B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Identifier", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Identifier B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String literal", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus String literal B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character literal", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character literal B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus Character B", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticMinus", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticMultiply", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticDivision", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisMinus, this) },
		{ "Synthesis Minus ArithmeticModule", std::bind(&LLParser::SynthesisMinus, this) },

		{ "Synthesis Multiply Integer", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Integer B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Float", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Float B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Identifier", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Identifier B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String literal", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply String literal B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character literal", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character literal B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply Character B", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticMinus", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticMultiply", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticDivision", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisMultiply, this) },
		{ "Synthesis Multiply ArithmeticModule", std::bind(&LLParser::SynthesisMultiply, this) },

		{ "Synthesis Integer division Integer", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Integer B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Float", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Float B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Identifier", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Identifier B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String literal", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division String literal B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character literal", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character literal B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division Character B", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticMinus", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticMultiply", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticDivision", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisIntegerDivision, this) },
		{ "Synthesis Integer division ArithmeticModule", std::bind(&LLParser::SynthesisIntegerDivision, this) },

		{ "Synthesis Division Integer", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Integer B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Float", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Float B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Identifier", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Identifier B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String literal", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division String literal B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character literal", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character literal B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division Character B", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticMinus", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticMultiply", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticDivision", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisDivision, this) },
		{ "Synthesis Division ArithmeticModule", std::bind(&LLParser::SynthesisDivision, this) },

		{ "Synthesis Modulus Integer", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Integer B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Float", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Float B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Identifier", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Identifier B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String literal", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus String literal B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character literal", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character literal B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus Character B", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticMinus", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticMultiply", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticDivision", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisModulus, this) },
		{ "Synthesis Modulus ArithmeticModule", std::bind(&LLParser::SynthesisModulus, this) },

		{ "Synthesis Equivalence Integer", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Integer B", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Float", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Float B", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Identifier", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Identifier B", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence String", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence String B", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence String literal", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence String literal B", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Character literal", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Character literal B", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Character", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence Character B", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence ArithmeticMinus", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence ArithmeticMultiply", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence ArithmeticDivision", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisEquivalence, this) },
		{ "Synthesis Equivalence ArithmeticModule", std::bind(&LLParser::SynthesisEquivalence, this) },

		{ "Synthesis Not equivalence Integer", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Integer B", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Float", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Float B", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Identifier", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Identifier B", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence String", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence String B", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence String literal", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence String literal B", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Character literal", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Character literal B", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Character", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence Character B", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence ArithmeticMinus", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence ArithmeticMultiply", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence ArithmeticDivision", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisNotEquivalence, this) },
		{ "Synthesis Not equivalence ArithmeticModule", std::bind(&LLParser::SynthesisNotEquivalence, this) },

		{ "Synthesis More or equal Integer", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Integer B", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Float", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Float B", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Identifier", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Identifier B", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal String", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal String B", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal String literal", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal String literal B", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Character literal", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Character literal B", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Character", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal Character B", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal ArithmeticMinus", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal ArithmeticMultiply", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal ArithmeticDivision", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },
		{ "Synthesis More or equal ArithmeticModule", std::bind(&LLParser::SynthesisMoreOrEquivalence, this) },

		{ "Synthesis Less or equal Integer", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Integer B", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Float", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Float B", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Identifier", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Identifier B", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal String", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal String B", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal String literal", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal String literal B", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Character literal", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Character literal B", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Character", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal Character B", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal ArithmeticMinus", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal ArithmeticMultiply", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal ArithmeticDivision", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },
		{ "Synthesis Less or equal ArithmeticModule", std::bind(&LLParser::SynthesisLessOrEquivalence, this) },

		{ "Synthesis More Integer", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Integer B", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Float", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Float B", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Identifier", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Identifier B", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More String", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More String B", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More String literal", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More String literal B", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Character literal", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Character literal B", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Character", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More Character B", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More ArithmeticMinus", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More ArithmeticMultiply", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More ArithmeticDivision", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisMore, this) },
		{ "Synthesis More ArithmeticModule", std::bind(&LLParser::SynthesisMore, this) },

		{ "Synthesis Less Integer", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Integer B", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Float", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Float B", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Identifier", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Identifier B", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less String", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less String B", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less String literal", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less String literal B", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Character literal", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Character literal B", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Character", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less Character B", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less ArithmeticMinus", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less ArithmeticMultiply", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less ArithmeticDivision", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less ArithmeticIntegerDivision", std::bind(&LLParser::SynthesisLess, this) },
		{ "Synthesis Less ArithmeticModule", std::bind(&LLParser::SynthesisLess, this) },

		{ "Synthesis Integer A", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float A", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier A", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer B", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float B", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier B", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer C", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float C", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier C", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer D", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float D", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier D", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer E", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float E", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier E", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Integer F", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Float F", std::bind(&LLParser::SynthesisType, this) },
		{ "Synthesis Identifier F", std::bind(&LLParser::SynthesisType, this) },

		{ "Synthesis Left round bracket Integer Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Float Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Identifier Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket String Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket String literal Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Character Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis Left round bracket Character literal Right round bracket", std::bind(&LLParser::RemoveBracketsAndSynthesis, this) },
		{ "Synthesis If keyword Left round bracket Identifier Right round bracket Statement", std::bind(&LLParser::RemoveIfRoundBrackets, this) },
		{ "Synthesis Assignment Semicolon", std::bind(&LLParser::RemoveSemicolon, this) },
		{ "Synthesis VariableDeclaration Semicolon", std::bind(&LLParser::RemoveSemicolon, this) },
		{ "Synthesis Left curly bracket Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis VariableDeclaration StatementList Right curly bracket", std::bind(&LLParser::RemoveScopeBrackets, this) },
		{ "Synthesis VariableDeclaration StatementList", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Assignment StatementList", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Left curly bracket StatementList Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Left curly bracket VariableDeclaration Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Identifier IdentifierListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Identifier ReferencedIdentifierListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Identifier ValuedIdentifierListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Comma Identifier", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis Comma ExtendedIdentifier", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis Read function Left round bracket String literal ReadExtra Right round bracket Semicolon", std::bind(&LLParser::RemovePredefinedFunctionReadOrWriteExtra, this) },
		{ "Synthesis Write function Left round bracket String literal WriteExtra Right round bracket Semicolon", std::bind(&LLParser::RemovePredefinedFunctionReadOrWriteExtra, this) },
		{ "Synthesis Comma IdentifierList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Comma ReferencedIdentifierList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Comma ValuedIdentifierList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Write function Left round bracket String literal Right round bracket Semicolon", std::bind(&LLParser::RemovePredefinedFunctionReadOrWriteExtra, this) },
		{ "Synthesis Write function StatementList", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Left curly bracket Write function Right curly bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Else keyword StatementListBlock", std::bind(&LLParser::SynthesisLastChildren, this) },
		{ "Synthesis If keyword Left round bracket Identifier Right round bracket", std::bind(&LLParser::RemoveIfOrWhileStatementExtra, this) },
		{ "Synthesis IfCondition StatementListBlock StatementListBlock", std::bind(&LLParser::SynthesisIfOrWhileCondition, this) },
		{ "Synthesis IfConditioSynthesis Left square bracket String literal Right square bracketn StatementListBlock", std::bind(&LLParser::SynthesisIfOrWhileConditionAndRemoveEmptyElse, this) },
		{ "Synthesis WhileCondition StatementListBlock", std::bind(&LLParser::SynthesisIfOrWhileCondition, this) },
		{ "Synthesis While keyword Left round bracket Identifier Right round bracket", std::bind(&LLParser::RemoveIfOrWhileStatementExtra, this) },
		{ "Synthesis Left square bracket Integer Right square bracket", std::bind(&LLParser::ExpandArrayLiteral, this) },
		{ "Synthesis Comma Integer", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis Comma Float", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis Comma Boolean literal", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis Comma String literal", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis Integer PossibleLiteralListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Float PossibleLiteralListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Boolean literal PossibleLiteralListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis String literal PossibleLiteralListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Comma PossibleLiteralList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Integer PossibleLiteralList", std::bind(&LLParser::SynthesisLastChildren, this) },
		{ "Synthesis Left square bracket PossibleLiteralList Right square bracket", std::bind(&LLParser::ExpandArrayLiteral, this) },
		{ "Synthesis Left square bracket String literal Right square bracket", std::bind(&LLParser::ExpandArrayLiteral, this) },
		{ "Synthesis Left square bracket Boolean literal Right square bracket", std::bind(&LLParser::ExpandArrayLiteral, this) },
		{ "Synthesis Left square bracket Float Right square bracket", std::bind(&LLParser::ExpandArrayLiteral, this) },
		{ "Synthesis Left square bracket Identifier Right square bracket", std::bind(&LLParser::ExpandArrayLiteral, this) },
		{ "Synthesis Identifier PossibleArrayAccessing", std::bind(&LLParser::SynthesisIdentifierPossibleArrayAccessing, this) },
		{ "Synthesis Integer IntegerListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Left square bracket IntegerList Right square bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Comma ArrayLiteral", std::bind(&LLParser::RemoveComma, this) },
		{ "Synthesis ArrayLiteral PossibleLiteralListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Left square bracket ArrayLiteral Right square bracket", std::bind(&LLParser::RemoveBracketsAndSynthesisType, this) },
		{ "Synthesis Integer ExpressionListExtension", std::bind(&LLParser::ExpandChildrenLastChildren, this) },
		{ "Synthesis Left square bracket ExpressionList Right square bracket", std::bind(&LLParser::RemoveBrackets, this) },
		{ "Synthesis Comma IntegerList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "Synthesis Comma ExpressionList", std::bind(&LLParser::SynthesisLastChildrenChildren, this) },
		{ "", std::bind(&LLParser::abc, this) },
	};

	std::unordered_set<std::string> IGNORED_ACTION_NAMES {
		"Synthesis Type Identifier",
		"Synthesis Type Integer",
		"Synthesis ExtendedType Identifier",
		"Synthesis VariableDeclarationA Assignment Integer",
		"Synthesis VariableDeclarationA Assignment Float",
		"Synthesis VariableDeclarationA Assignment Identifier",
		"Synthesis VariableDeclarationA Assignment String",
		"Synthesis VariableDeclarationA Assignment Character",
		"Synthesis VariableDeclarationA Assignment String literal",
		"Synthesis VariableDeclarationA Assignment Character literal",
		"Synthesis VariableDeclarationA Assignment Boolean literal",
		"Synthesis VariableDeclarationA Assignment ArrayLiteral",
		"Synthesis VariableDeclarationA Assignment ArithmeticPlus",
		"Synthesis VariableDeclarationA Assignment ArithmeticMinus",
		"Synthesis VariableDeclarationA Assignment ArithmeticMultiply",
		"Synthesis VariableDeclarationA Assignment ArithmeticDivision",
		"Synthesis VariableDeclarationA Assignment ArithmeticIntegerDivision",
		"Synthesis VariableDeclarationA Assignment ArithmeticModule",
		"Synthesis Identifier Assignment Integer",
		"Synthesis Identifier Assignment Float",
		"Synthesis Identifier Assignment Identifier",
		"Synthesis Identifier Assignment String",
		"Synthesis Identifier Assignment Character",
		"Synthesis Identifier Assignment String literal",
		"Synthesis Identifier Assignment Character literal",
		"Synthesis Identifier Assignment Boolean literal",
		"Synthesis Identifier Assignment ArrayLiteral",
		"Synthesis Identifier Assignment ArithmeticPlus",
		"Synthesis Identifier Assignment ArithmeticMinus",
		"Synthesis Identifier Assignment ArithmeticMultiply",
		"Synthesis Identifier Assignment ArithmeticDivision",
		"Synthesis Identifier Assignment ArithmeticIntegerDivision",
		"Synthesis Identifier Assignment ArithmeticModule",
		"Synthesis Assignment Identifier ArithmeticNegate",
		"Synthesis VariableDeclaration VariableDeclaration",
		"Synthesis VariableDeclaration If",
		"Synthesis VariableDeclaration While",
		"Synthesis VariableDeclaration Write function",
		"Synthesis Read function Write function",
		"Synthesis Assignment If",
		"Synthesis Write function Assignment",
		"Synthesis Type PossibleArray",
		"Synthesis ExtendedIdentifier Assignment String literal",
		"Synthesis ExtendedIdentifier Assignment Integer",
		"Synthesis Write function Write function"
	};

	std::unordered_map<std::string, std::unordered_set<std::string>> EXTRA_COMPATIBLE_TYPES = {
		{ TokenConstant::CoreType::Complex::STRING, { TokenConstant::Name::STRING_LITERAL }},
		{ TokenConstant::CoreType::CHARACTER, { TokenConstant::Name::CHARACTER_LITERAL }},
		{ TokenConstant::CoreType::BOOLEAN, { TokenConstant::Name::BOOLEAN_LITERAL }},
		{ TokenConstant::CoreType::Number::FLOAT, { TokenConstant::CoreType::Number::INTEGER }},
		{ TokenConstant::CoreType::Complex::ARRAY, { TokenConstant::Name::ARRAY_LITERAL }}
	};

	LLTableBuilder m_llTableBuilder;
	std::vector<AstNode *> m_ast;
	std::vector<std::unordered_map<std::string, unsigned int>> m_scopes {{ }};
	SymbolTable m_symbolTable;
	llvm::LLVMContext m_context;
	std::unique_ptr<llvm::Module> m_module;
	llvm::FunctionType * m_mainFunctionType;
	llvm::Function * m_mainFunction;
	llvm::BasicBlock * m_mainBlock;
	llvm::IRBuilder<> * m_builder;
	std::unique_ptr<llvm::DataLayout> m_dataLayout;
	std::stack<llvm::BasicBlock*> m_preWhileBlocks;
	std::stack<llvm::BasicBlock*> m_whileBlocks;
	std::stack<llvm::BasicBlock*> m_blocksTrue;
	std::stack<llvm::BasicBlock*> m_blocksFalse;
	std::stack<llvm::BasicBlock*> m_previousBlocks;
};

#endif
