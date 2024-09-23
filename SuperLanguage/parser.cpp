#include "parser.hpp"

#include <cassert>
#include <format>

Parser::Parser(std::vector<Token>&& tokens)
	:_tokens{std::move(tokens)}
	,_current(_tokens.begin())
{
}

Node* Parser::add_tokens(const std::vector<Token>& tokens)
{
	const auto size = _tokens.size();
	_tokens.insert(_tokens.end(), tokens.begin(), tokens.end());
	_current = _tokens.begin() + size;

	return statement();
}

Node* Parser::parse()
{
	auto nodes = statement_list();
	return new Scope(std::move(nodes));
}

void Parser::eat(TokType tok_type)
{
	if (_current != _tokens.end() && _current->type == tok_type)
	{
		++_current;
	}
	else
	{
		puts("Parse error: expected ");
		assert(false);
	}
}

std::vector<Node*> Parser::statement_list()
{
	std::vector<Node*> nodes;

	while(_current != _tokens.end())
	{
		if(_current->type == TT_ScopeEnd)
		{
			_skip_semicolon = false;
			break;
		}

		nodes.push_back(statement());
		if (!_skip_semicolon)
		{
			eat(TT_Semicolon);
		}
		else
		{
			_skip_semicolon = false;
		}
	}

	return nodes;
}

Node* Parser::statement()
{
	if(_current == _tokens.end())
	{
		return nullptr;
	}

	if(_current->type == TT_Let)
	{
		if (const Variable* var = create_variable())
		{
			eat(TT_Assign);

			Assign* res;
			if (_current->type == TT_Fn)
			{
				res = new Assign(var->get_stack_index(), statement(), true);
			}
			else
			{
				res = new Assign(var->get_stack_index(), expression(), true);
			}

			_variables.emplace(var->get_name(), _current_context);

			return res;
		}
	}

	if(_current->type == TT_Id)
	{
		auto node = resolve_id();

		const auto var = dynamic_cast<Variable*>(node);
		if(!var)
		{
			return node;
		}

		eat(TT_Assign);

		return new Assign(var->get_stack_index(), expression());
	}

	if(_current->type == TT_ScopeBegin)
	{
		const auto base_index = _index_counter;
		++_scope_level;
		eat(TT_ScopeBegin);
		auto nodes = statement_list();
		eat(TT_ScopeEnd);
		--_scope_level;
		_skip_semicolon = true;
		_index_counter = base_index;
		return new Scope(std::move(nodes));
	}

	if(_current->type == TT_Fn)
	{
		eat(TT_Fn);
		if(_current->type == TT_Id)
		{
			_current_func = _current->name;
			eat(TT_Id);
		}

		eat(TT_LParen);
		const auto prev_counter = _index_counter;
		_index_counter = 0;
		int param_index = 0;
		while (_current->type == TT_Id)
		{
			const std::string param_name = std::format("param_{}_{}", _current_func, _current->name);
			auto* var = new Variable(std::string{param_name}, param_index);
			_variables.emplace(param_name, VariableInfo{ TypeContext::None, var });
			++_index_counter;
			eat(TT_Id);
			if (_current->type != TT_RParen)
			{
				eat(TT_Coma);
			}

			++param_index;
		}
		eat(TT_RParen);
		
		const auto scope = dynamic_cast<Scope*>(statement());
		_index_counter = prev_counter;
		return new Function(scope, std::move(_current_func), param_index);
	}

	if(_current->type == TT_Ret)
	{
		eat(TT_Ret);
		return new Return(expression());
	}

	if(_current->type == TT_If)
	{
		eat(TT_If);
		eat(TT_LParen);
		Node* expr = expression();
		eat(TT_RParen);
		const auto scope = dynamic_cast<Scope*>(statement());
		Scope* else_branch = nullptr;
		if(_current != _tokens.end() && _current->type == TT_Else)
		{
			eat(TT_Else);
			_skip_semicolon = false;
			else_branch = dynamic_cast<Scope*>(statement());
		}

		return new BranchIfElse(expr, scope, else_branch);
	}

	if(_current->type == TT_Loop)
	{
		eat(TT_Loop);
		eat(TT_LParen);
		Node* expr = expression();
		eat(TT_RParen);
		const auto scope = dynamic_cast<Scope*>(statement());
		return new Loop(expr, scope);
	}

	return nullptr;
}

Node* Parser::expression()
{
	_current_context = get_expression_context();
	
	if(_current_context == TypeContext::String)
	{
		return string_expression();
	}
	if(_current_context == TypeContext::Number)
	{
		return number_expression();
	}
	if(_current_context == TypeContext::Bool)
	{
		return bool_expression();
	}

	if(_current->type == TT_Id)
	{
		return resolve_id();
	}
	assert(false);

	return nullptr;
}

Node* Parser::bool_expression()
{
	auto node = bool_term();

	while (_current->type == TT_And || _current->type == TT_Or)
	{
		const auto op = _current->type == TT_Plus ? Operation::Plus : Operation::Minus;
		eat(_current->type);
		node = new BinaryOperation(node, term(), op);
	}

	return node;
}

Node* Parser::number_expression()
{
	auto node = term();

	while (_current->type == TT_Plus || _current->type == TT_Minus)
	{
		const auto op = _current->type == TT_Plus ? Operation::Plus : Operation::Minus;
		eat(_current->type);
		node = new BinaryOperation(node, term(), op);
	}

	return node;
}

Node* Parser::string_expression()
{
	Node* node = string_factor();
	while (_current->type == TT_Plus)
	{
		eat(TT_Plus);
		node = new BinaryOperation(node, string_factor(), Operation::Plus);
	}

	return node;
}

Node* Parser::string_factor()
{
	if (_current->type == TT_Id)
	{
		return resolve_id();
	}
	if (_current->type == TT_StringLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TT_StringLiteral);
		return new Literal(f);
	}

	return nullptr;
}

Node* Parser::factor()
{
	if(_current->type == TT_BoolLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TT_BoolLiteral);
		return new Literal(f);
	}
	if(_current->type == TT_LParen)
	{
		eat(TT_LParen);
		Node* expr = expression();
		eat(TT_RParen);
		return expr;
	}
	if(_current->type == TT_Id)
	{
		return resolve_id();
	}
	if(_current->type == TT_NumberLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TT_NumberLiteral);
		return new NumberLiteral(f);
	}
	
	return nullptr;
}

Node* Parser::term()
{
	auto node = factor();

	while(_current->type == TT_Mul || _current->type == TT_Div)
	{
		const auto op = _current->type == TT_Mul ? Operation::Mul : Operation::Div;
		eat(_current->type);
		node = new BinaryOperation(node, factor(), op);
	}

	return node;
}

Node* Parser::bool_term()
{
	auto node = factor();

	if (_current->type == TT_Equal || _current->type == TT_Greater || _current->type == TT_Less)
	{
		const auto prev_type = _current->type;
		eat(_current->type);

		Operation op = Operation::Equal;
		if(prev_type == TT_Greater)
		{
			op = Operation::Greater;
		}
		else if (prev_type == TT_Less)
		{
			op = Operation::Less;
		}

		if(_current->type == TT_Equal && prev_type == TT_Greater)
		{
			op = Operation::EqualGreater;
			eat(_current->type);
		}
		if (_current->type == TT_Equal && prev_type == TT_Less)
		{
			op = Operation::EqualLess;
			eat(_current->type);
		}

		node = new BinaryOperation(node, factor(), op);
	}

	return node;
}

Variable* Parser::create_variable()
{
	eat(TT_Let);

	std::string name = std::format("{}_{}_{}", _scope_level, _current->name, _current_func);
	eat(TT_Id);
	const size_t var_offset = _index_counter++;
	auto* var = new Variable(std::move(name), var_offset);
	_variables.emplace(name, VariableInfo{ get_expression_context(), var });

	return var;
}

Variable* Parser::get_variable()
{
	std::string name = _current->name;
	eat(TT_Id);

	auto find_var = [this](const std::string& name) -> Variable*
	{
		if (const auto it = _variables.find(name); it != _variables.end())
		{
			return it->second.variable;
		}
		return nullptr;
	};

	if(!_current_func.empty())
	{
		const auto param_name = std::format("param_{}_{}", _current_func, name);

		if(const auto var = find_var(param_name))
		{
			return var;
		}
	}

	auto scope = _scope_level;

	while (scope >= 0)
	{
		const auto var_name = std::format("{}_{}_{}", scope, name, _current_func);
		if (const auto var = find_var(var_name))
		{
			return var;
		}
		--scope;
	}

	return nullptr;
}

Parser::TypeContext Parser::get_variable_context(const std::string& name) const
{
	if(_variables.empty())
	{
		return TypeContext::None;
	}

	auto find_var = [this](const std::string& name)
	{
		if (const auto it = _variables.find(name); it != _variables.end())
		{
			return it->second.context;
		}
		return TypeContext::None;
	};

	if (!_current_func.empty())
	{
		const auto param_name = std::format("param_{}_{}", _current_func, name);

		if (const auto context = find_var(param_name); context != TypeContext::None)
		{
			return context;
		}
	}
	else
	{
		auto scope = _scope_level;

		while (scope >= 0)
		{
			auto scope_name = std::format("{}_{}_{}", scope, name, _current_func);

			if (const auto context = find_var(scope_name); context != TypeContext::None)
			{
				return context;
			}
			--scope;
		}
	}
	return TypeContext::None;
}

Node* Parser::resolve_id()
{
	std::string name = _current->name;
	const auto var = get_variable();

	if (!var && _current->type == TT_LParen)
	{
		eat(TT_LParen);
		std::vector<Node*> args;
		while (_current->type != TT_RParen)
		{
			args.push_back(expression());
			if (_current->type != TT_RParen)
			{
				eat(TT_Coma);
			}
		}
		eat(TT_RParen);
		return new Call(std::move(args), std::move(name));
	}
	return var;
}

Parser::TypeContext Parser::get_expression_context() const
{
	auto it = _current;
	TypeContext context = TypeContext::None;
	while (it != _tokens.end() && it->type != TT_Semicolon && it->type != TT_Coma)
	{
		if(it->type == TT_ScopeBegin)
		{
			return context;
		}

		if(it->type == TT_Equal || it->type == TT_Greater || it->type == TT_Less)
		{
			return TypeContext::Bool;
		}

		if(it->type == TT_Mul || it->type == TT_Div || it->type == TT_Minus)
		{
			return TypeContext::Number;
		}

		if(it->type == TT_Plus)
		{
			if(context == TypeContext::Number)
			{
				return TypeContext::Number;
			}

			if(context == TypeContext::String)
			{
				return TypeContext::String;
			}
		}

		if(it->type == TT_NumberLiteral)
		{
			context = TypeContext::Number;
		}

		if (it->type == TT_StringLiteral)
		{
			context = TypeContext::String;
		}

		if(it->type == TT_BoolLiteral)
		{
			context = TypeContext::Bool;
		}

		if (it->type == TT_Id)
		{
			const auto res = get_variable_context(it->name);
			if (res != TypeContext::None)
			{
				context = res;
			}
		}
		++it;
	}
	return context;
}
