#include "parser.hpp"

#include <cassert>
#include <format>

Parser::Parser(std::vector<Token>&& tokens)
	:_tokens{std::move(tokens)}
	,_current(_tokens.begin())
{
}

Node* Parser::parse()
{
	auto nodes = statementList();
	return new Scope(0, std::move(nodes));
}

void Parser::eat(TokType tokType)
{
	if (_current != _tokens.end() && _current->type == tokType)
	{
		++_current;
	}
	else
	{
		puts("Parse error: expected ");
		assert(false);
	}
}

std::vector<Node*> Parser::statementList()
{
	std::vector<Node*> nodes;

	while(_current != _tokens.end())
	{
		if(_current->type == TT_ScopeEnd)
		{
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
		if (Variable* var = create_variable())
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
		std::string name = _current->name;
		const auto var = get_variable();

		if(!var && _current->type == TT_LParen)
		{
			eat(TT_LParen);
			eat(TT_RParen);
			return new Call(std::move(name));
		}

		eat(TT_Assign);

		return new Assign(var->get_stack_index(), expression());
	}

	if(_current->type == TT_ScopeBegin)
	{
		const auto base_index = _index_counter;
		++_scope_level;
		eat(TT_ScopeBegin);
		auto nodes = statementList();
		eat(TT_ScopeEnd);
		--_scope_level;
		_skip_semicolon = true;
		_index_counter = base_index;
		return new Scope(base_index + 1, std::move(nodes));
	}

	if(_current->type == TT_Fn)
	{
		eat(TT_Fn);
		std::string name;
		if(_current->type == TT_Id)
		{
			name = _current->name;
			eat(TT_Id);
		}

		eat(TT_LParen);

		eat(TT_RParen);

		const auto scope = dynamic_cast<Scope*>(statement());

		return new Function(scope, std::move(name));
	}

	return nullptr;
}

Node* Parser::expression()
{
	_current_context = get_expression_context();
	
	if(_current_context == TypeContext::String)
	{
		return string_expresson();
	}
	else if(_current_context == TypeContext::Number)
	{
		return number_expression();
	}
	else
	{
		assert(false);
	}

	return nullptr;
}

Node* Parser::number_expression()
{
	auto node = term();

	while (_current->type == TT_Plus || _current->type == TT_Minus)
	{
		const char op = _current->type == TT_Plus ? '+' : '-';
		eat(_current->type);
		node = new BinaryOperation(node, term(), op);
	}

	return node;
}

Node* Parser::string_expresson()
{
	Node* node = string_factor();
	while (_current->type == TT_Plus)
	{
		eat(TT_Plus);
		node = new BinaryOperation(node, string_factor(), '+');
	}

	return node;
}

Node* Parser::string_factor()
{
	if (_current->type == TT_Id)
	{
		return get_variable();
	}
	if (_current->type == TT_StringLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TT_StringLiteral);
		return new StringLiteral(f);
	}

	return nullptr;
}

Node* Parser::factor()
{
	if(_current->type == TT_LParen)
	{
		eat(TT_LParen);
		Node* expr = expression();
		eat(TT_RParen);
		return expr;
	}
	if(_current->type == TT_Id)
	{
		return get_variable();
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
		const char op = _current->type == TT_Mul ? '*' : '/';
		eat(_current->type);
		node = new BinaryOperation(node, factor(), op);
	}

	return node;
}

Variable* Parser::create_variable()
{
	eat(TT_Let);
	std::string name = std::format("{}_{}", _scope_level, _current->name);
	eat(TT_Id);
	const size_t var_offset = _index_counter++;
	auto* var = new Variable(std::move(name), var_offset);
	_variables.emplace(name, VariableInfo{ _current_context, var });

	return var;
}

Variable* Parser::get_variable()
{
	std::string name = _current->name;
	eat(TT_Id);

	auto scope = _scope_level;

	while (scope >= 0)
	{
		auto scope_name = std::format("{}_{}", scope, name);

		if(auto it = _variables.find(scope_name); it != _variables.end())
		{
			return it->second.variable;
		}
		--scope;
	}

	return nullptr;
}

Parser::TypeContext Parser::get_expression_context() const
{
	if(_current->type == TT_NumberLiteral)
	{
		return TypeContext::Number;
	}
	if (_current->type == TT_StringLiteral)
	{
		return TypeContext::String;
	}

	auto it = _variables.find(_current->name);
	if (_current->type == TT_Id && it != _variables.end())
	{
		return it->second.context;
	}
	return _current_context;
}
