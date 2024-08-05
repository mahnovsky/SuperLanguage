#pragma once

#include "lexer.hpp"
#include "nodes.hpp"

class Parser
{
public:
	Parser(std::vector<Token>&& tokens);

	Node* parse();

private:
	void eat(TokType tokType);

	std::vector<Node*> statementList();

	Node* statement();

	Node* expression();

	Node* factor();

	Node* term();
private:

	Scope* _current_scope = nullptr;
	std::vector<Token> _tokens;
	std::vector<Token>::const_iterator _current;
	bool _skip_semicolon = false;
};
