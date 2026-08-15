#pragma once

#include "lexer.hpp"
#include "new_ast.h"
#include "nodes.hpp"
#include "node_data.h"

class Parser
{
public:
	enum class TypeContext
	{
		None,
		Number,
		String,
		Bool,
		Array
	};

	Parser(std::vector<Token>&& tokens);

	new_ast::Node add_tokens(const std::vector<Token>& tokens);

	new_ast::Node parse();

private:
	void eat(TokType tok_type);

	std::vector<new_ast::Node> statement_list();

	new_ast::Node statement();

	new_ast::Node expression();

	new_ast::Node bool_expression();

	new_ast::Node number_expression();

	new_ast::Node string_expression();

	new_ast::Node array_expression();

	new_ast::Node array_element();

	new_ast::Node string_factor();

	new_ast::Node factor();

	new_ast::Node term();

	new_ast::Node bool_term();

	new_ast::Node create_variable();

	std::optional<new_ast::Node> get_variable();

	TypeContext get_variable_context(const std::string& name) const;

	new_ast::Node resolve_id();

private:
	

	struct VariableInfo
	{
		TypeContext context;
		new_ast::Node var_node;
	};

	TypeContext get_expression_context() const;

	data::Scope* _current_scope = nullptr;
	std::vector<Token> _tokens;
	std::vector<Token>::const_iterator _current;
	bool _skip_semicolon = false;
	TypeContext _current_context = TypeContext::None;
	std::map<std::string, VariableInfo> _variables;
	size_t _index_counter = 0;
	int _scope_level = 0;
	std::string _current_func;
};
