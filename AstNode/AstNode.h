#ifndef LLPARSERLIBRARYEXAMPLE_ASTNODE_H
#define LLPARSERLIBRARYEXAMPLE_ASTNODE_H

#include <string>
#include <vector>

class AstNode
{
public:
	std::string name;
	std::string type;
	std::string computedType;
	AstNode * father;
	std::vector<AstNode*> children;

	std::string stringValue;
};

#endif
