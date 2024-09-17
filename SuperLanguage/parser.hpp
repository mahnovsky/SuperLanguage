#pragma once

#include "lexer.hpp"
#include "nodes.hpp"

class Parser
{
public:
	enum class TypeContext
	{
		None,
		Number,
		String,
		Bool
	};

	Parser(std::vector<Token>&& tokens);

	Node* add_tokens(const std::vector<Token>& tokens);

	Node* parse();

private:
	void eat(TokType tokType);

	std::vector<Node*> statementList();

	Node* statement();

	Node* expression();

	Node* bool_expression();

	Node* number_expression();

	Node* string_expression();

	Node* string_factor();

	Node* factor();

	Node* term();

	Node* bool_term();

	Variable* create_variable();

	Variable* get_variable();

	TypeContext get_variable_context(const std::string& name) const;

	Node* resolve_id();

private:
	

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
	std::string _current_func;
};
