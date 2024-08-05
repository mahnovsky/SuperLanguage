#include "lexer.hpp"

#include <cassert>
#include <cctype>
#include <charconv>
#include <format>

Token::Token(TokType t, ObjectPtr v)
	: type(t)
	, object(std::move(v))
{}

std::optional<TokType> Lexer::match_op(char ch)
{
	switch(ch)
	{
	case '+': return TokType::Plus;
	case '-': return TokType::Minus;
	case '*': return TokType::Mul;
	case '/': return TokType::Div;
	default: return {};
	}
}

std::string_view Lexer::read_word()
{
	std::string::const_iterator it = _current;
	while(_current != _end)
	{
		if(isalpha(*_current) > 0)
		{
			++_current;
		}
		else
		{
			break;
		}
	}

	return std::string_view{it, _current};
}

ObjectPtr Lexer::read_number()
{
	std::string::const_iterator it = _current;
	bool is_point = false;
	while (_current != _end)
	{
		if((*_current) == '.' && it != _current)
		{
			is_point = true;
			++_current;
		}
		else if (isdigit(*_current))
		{
			++_current;
		}
		else if(it != _current)
		{
			std::string_view number{it, _current};
			if (!is_point)
			{
				int value = 0;
				auto [ptr, ec] = std::from_chars(number.data(), 
					number.data() + number.size(), value);
				if (ec == std::errc())
				{
					return std::make_shared<Integer>(value);
				}
			}
			else
			{
				float value = 0;
				auto [ptr, ec] = std::from_chars(number.data(),
					number.data() + number.size(), value);
				if (ec == std::errc())
				{
					return std::make_shared<Float>(value);
				}
			}
		}
		else
		{
			break;
		}
	}

	return 0;
}

std::vector<Token> Lexer::tokenize(const std::string& expression)
{
	const auto lines = split_by_lines(expression);

	for (auto& line : lines)
	{
		if(line.front() == '\n')
		{
			continue;
		}
		_begin = line.begin();
		_current = line.begin();
		_end = line.end();
		process_line();
		++_current_line;
	}

	return _tokens;
}

void Lexer::process_line()
{
	eat_until_not('\t');

	const auto word = read_word();
	uint32_t expect = TOK_INT(Let) | TOK_INT(Assign);
	if(word == "let")
	{
		eat_until_not(' ', true);
		put_declaration();

		expect = TOK_INT(Assign);
	}
	else if(!word.empty())
	{
		eat_until_not(' ');
		if((*_current) == '(')
		{
			eat('(');
			eat(')');
			put_id(std::string{word});

			expect = TOK_INT(Semicolon);
		}
		else
		{
			eat('=');
			put_assign(std::string{word});

			expect = TOK_INT(Id) | TOK_INT(NumberLiteral) | TOK_INT(LParen);
		}
	}

	std::vector<bool> paren_stack;
 	while (_current != _end)
	{
		if ((*_current) == '{' || (*_current) == '}')
		{
			_tokens.emplace_back((*_current) == '{' ? TokType::ScopeBegin : TokType::ScopeEnd);
			eat(*_current);
			expect = TOK_INT(Let) | TOK_INT(Assign);
			continue;
		}

		if ((expect & TOK_INT(LParen)) && (*_current) == '(')
		{
			paren_stack.push_back(true);
			put_lparen();
			eat('(');
			expect = TOK_INT(Id) | TOK_INT(NumberLiteral) | TOK_INT(LParen);
		}
		else if ((expect & TOK_INT(RParen)) && (*_current) == ')')
		{
			if(paren_stack.empty())
			{
				fatal_error("Meet unexpected token \')\'");
			}
			paren_stack.pop_back();
			put_rparen();
			eat(')');
			expect = TOK_INT(Operation) | TOK_INT(Semicolon) | TOK_INT(RParen);
		}
		else if((expect & TOK_INT(Assign)) && isalpha(*_current) > 0)
		{
			put_assign(std::string{read_word()});
			eat_until_not(' ');
			eat('=');
			expect = TOK_INT(Id) | TOK_INT(NumberLiteral) | TOK_INT(LParen);
		}
		else if ((expect & TOK_INT(Id)) && isalpha(*_current) > 0)
		{
			put_id(std::string{ read_word() });
			expect = TOK_INT(Operation) | TOK_INT(Semicolon);
		}
		else if((expect & TOK_INT(NumberLiteral)) && isdigit(*_current) > 0)
		{
			put_number_literal(read_number());
			expect = TOK_INT(Operation) | TOK_INT(Semicolon);
		}
		else if((expect & TOK_INT(Semicolon)) && (*_current) == ';')
		{
			put_semicolon();
			eat(';');
			break;
		}
		else if((expect & TOK_INT(Operation)) && match_op(*_current).has_value())
		{
			put_operation(*_current);
			eat(*_current);
			expect = TOK_INT(Id) | TOK_INT(NumberLiteral) | TOK_INT(LParen);
		}
		else if((*_current) != ' ' && (*_current) != '\n')
		{
			auto err_msg = std::format("Unexpected token type {}", *_current);
			fatal_error(err_msg);
		}

		if (!_tokens.empty() && !paren_stack.empty())
		{
			const auto type = _tokens.back().type;
			if (type == TokType::Id || 
				type == TokType::NumberLiteral)
			{
				expect |= TOK_INT(RParen);
			}
		}

		eat_until_not(' ');
		eat_until_not('\n');
	}

	if(!paren_stack.empty())
	{
		fatal_error("No closed paren");
	}
}

void Lexer::put_number_literal(ObjectPtr number)
{
	_tokens.emplace_back(TokType::NumberLiteral, std::move(number));
}

void Lexer::put_operation(char op)
{
	if (auto tokOp = match_op(op); tokOp)
	{
		_tokens.emplace_back(tokOp.value());
	}
}

void Lexer::put_declaration()
{
	_tokens.emplace_back(TokType::Let);
}

void Lexer::put_semicolon()
{
	_tokens.emplace_back(TokType::Semicolon);
}

void Lexer::put_assign(std::string&& name)
{
	_tokens.emplace_back(TokType::Assign, std::move(name));
}

void Lexer::put_id(std::string&& name)
{
	_tokens.emplace_back(TokType::Id, std::move(name));
}

void Lexer::put_lparen()
{
	_tokens.emplace_back(TokType::LParen);
}

void Lexer::put_rparen()
{
	_tokens.emplace_back(TokType::RParen);
}

void Lexer::put_scope_begin()
{
	_tokens.emplace_back(TokType::ScopeBegin);
}

void Lexer::put_scope_end()
{
	_tokens.emplace_back(TokType::ScopeEnd);
}

void Lexer::eat(char ch)
{
	if(_current != _end && (*_current) == ch)
	{
		++_current;
	}
	else
	{
		assert(false);
	}
}

void Lexer::eat_until_not(char ch, bool expect_once)
{
	if (expect_once)
	{
		assert(_current != _end && (*_current) == ch);
	}
	while (_current != _end && (*_current) == ch)
	{
		++_current;
	}
}

void Lexer::fatal_error(const std::string& error_msg)
{
	auto offset = _current - _begin;
	auto msg = std::format("{}:{} > {}", _current_line, offset, error_msg);
	puts(msg.c_str());
	exit(1);
}
