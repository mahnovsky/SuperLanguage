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

	Variable* create_variable();

	Variable* get_variable();

private:
	enum class TypeContext
	{
		None,
		Number,
		String
	};

	struct VariableInfo
	{
		TypeContext context;
		Variable* variable;
	};

	TypeContext get_expression_context() const;

	Scope* _current_scope = nullptr;
	std::vector<Token> _tokens;
	std::vector<Token>::const_iterator _current;
	bool _skip_semicolon = false;
	TypeContext _current_context = TypeContext::None;
	std::map<std::string, VariableInfo> _variables;
	size_t _index_counter = 0;
	int _scope_level = 0;
};
