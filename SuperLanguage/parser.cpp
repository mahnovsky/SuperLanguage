#include "parser.hpp"

#include <cassert>

Parser::Parser(std::vector<Token>&& tokens)
	:_tokens{std::move(tokens)}
	,_current(_tokens.begin())
{
}

Node* Parser::parse()
{
	auto nodes = statementList();
	return new Scope(std::move(nodes));
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
		eat(TT_Let);
		std::string var = _current->name;
		eat(TT_Id);
		eat(TT_Assign);
		if(_current->type == TT_Fn)
		{
			return new Assign(std::move(var), statement(), true);
		}
		return new Assign(std::move(var), expression(), true);
	}

	if(_current->type == TT_Id)
	{
		std::string var = _current->name;
		eat(TT_Id);

		if(_current->type == TT_LParen)
		{
			eat(TT_LParen);
			eat(TT_RParen);
			return new Call(std::move(var));
		}

		eat(TT_Assign);
		return new Assign(std::move(var), expression());
	}

	if(_current->type == TT_ScopeBegin)
	{
		eat(TT_ScopeBegin);
		auto nodes = statementList();
		eat(TT_ScopeEnd);
		_skip_semicolon = true;
		return new Scope(std::move(nodes));
	}

	if(_current->type == TT_Fn)
	{
		eat(TT_Fn);
		eat(TT_LParen);
		eat(TT_RParen);

		return statement();
	}

	return nullptr;
}

Node* Parser::expression()
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
		std::string name = _current->name;
		eat(TT_Id);
		return new Variable(std::move(name));
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
