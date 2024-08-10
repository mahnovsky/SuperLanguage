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

	Node* number_expression();

	Node* string_expresson();

	Node* string_factor();

	Node* factor();

	Node* term();

	Node* create_variable();

private:
	enum class TypeContext
	{
		None,
		Number,
		String
	};

	TypeContext get_expression_context() const;

	Scope* _current_scope = nullptr;
	std::vector<Token> _tokens;
	std::vector<Token>::const_iterator _current;
	bool _skip_semicolon = false;
	TypeContext _current_context = TypeContext::None;
	std::map<std::string, TypeContext> _id_types;
};
