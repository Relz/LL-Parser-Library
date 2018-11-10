#ifndef LLPARSERLIBRARYEXAMPLE_ASTNODE_H
#define LLPARSERLIBRARYEXAMPLE_ASTNODE_H

#include <string>
#include <vector>

class AstNode
{
public:
	std::string name;
	AstNode * father;
	std::vector<AstNode*> children;

	std::string stringValue;
};

#endif
