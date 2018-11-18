CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(LLParserLibrary)

SET(CMAKE_CXX_STANDARD 17)

SET(SOURCE_FILES
	LLParser.cpp
	LLParser.h
	AstNode/AstNode.cpp
	AstNode/AstNode.h SymbolTable/SymbolTable.cpp SymbolTable/SymbolTable.h SymbolTable/SymbolTableRow/SymbolTableRow.cpp SymbolTable/SymbolTableRow/SymbolTableRow.h SymbolTable/SymbolTableRow/ArrayInformation/ArrayInformation.cpp SymbolTable/SymbolTableRow/ArrayInformation/ArrayInformation.h Calculator/Calculator.cpp Calculator/Calculator.h)

ADD_LIBRARY(LLParserLibrary SHARED ${SOURCE_FILES})

IF (NOT TARGET LLTableBuilderLibrary)
	ADD_SUBDIRECTORY(LLTableBuilderLibrary)
ENDIF ()
IF (NOT TARGET LexerLibrary)
	ADD_SUBDIRECTORY(LexerLibrary)
ENDIF ()

TARGET_LINK_LIBRARIES(LLParserLibrary LLTableBuilderLibrary LexerLibrary)
