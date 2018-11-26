#ifndef LLPARSERLIBRARYEXAMPLE_ASTNODE_H
#define LLPARSERLIBRARYEXAMPLE_ASTNODE_H

#include <string>
#include <vector>
#include <llvm/IR/Value.h>

class AstNode
{
public:
	std::string name;
	std::string type;
	std::string computedType;
	llvm::Value * llvmValue;
	std::vector<AstNode*> children;

	std::string stringValue;
};

#endif
