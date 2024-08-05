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
		if(_current->type == TokType::ScopeEnd)
		{
			break;
		}

		nodes.push_back(statement());
		if (!_skip_semicolon)
		{
			eat(TokType::Semicolon);
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

	if(_current->type == TokType::Let)
	{
		eat(TokType::Let);
		std::string var = _current->name;
		eat(TokType::Assign);
		if(_current->type == TokType::ScopeBegin)
		{
			return new Assign(std::move(var), statement(), true);
		}
		return new Assign(std::move(var), expression(), true);
	}
	if(_current->type == TokType::Assign)
	{
		std::string var = _current->name;
		eat(TokType::Assign);
		return new Assign(std::move(var), expression());
	}

	if(_current->type == TokType::ScopeBegin)
	{
		eat(TokType::ScopeBegin);
		auto nodes = statementList();
		eat(TokType::ScopeEnd);
		_skip_semicolon = true;
		return new Scope(std::move(nodes));
	}

	if(_current->type == TokType::Call)
	{
		auto name = _current->name;
		eat(TokType::Call);

		return new Call(std::move(name));
	}

	return nullptr;
}

Node* Parser::expression()
{
	auto node = term();

	while (_current->type == TokType::Plus || _current->type == TokType::Minus)
	{
		char op = _current->type == TokType::Plus ? '+' : '-';
		eat(_current->type);
		node = new BinaryOperation(node, term(), op);
	}

	return node;
}

Node* Parser::factor()
{
	if(_current->type == TokType::LParen)
	{
		eat(TokType::LParen);
		Node* expr = expression();
		eat(TokType::RParen);
		return expr;
	}
	if(_current->type == TokType::Variable)
	{
		std::string name = _current->name;
		eat(TokType::Variable);
		return new Variable(std::move(name));
	}
	if(_current->type == TokType::NumberLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TokType::NumberLiteral);
		return new NumberLiteral(f);
	}
	
	return nullptr;
}

Node* Parser::term()
{
	auto node = factor();

	while(_current->type == TokType::Mul || _current->type == TokType::Div)
	{
		const char op = _current->type == TokType::Mul ? '*' : '/';
		eat(_current->type);
		node = new BinaryOperation(node, factor(), op);
	}

	return node;
}
