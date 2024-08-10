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
	case '+': return TT_Plus;
	case '-': return TT_Minus;
	case '*': return TT_Mul;
	case '/': return TT_Div;
	default: return {};
	}
}

std::string_view Lexer::read_word() const
{
	std::string::const_iterator it = _current;
	while(it != _end)
	{
		if(isalpha(*it) > 0)
		{
			++it;
		}
		else
		{
			break;
		}
	}

	return std::string_view{_current, it };
}

std::string_view Lexer::read_number() const
{
	std::string::const_iterator it = _current;
	while (it != _end)
	{
		if (isdigit(*it) == 0 && (*it) != '.')
		{
			break;
		}
		++it;
	}

	return { _current, it };
}

std::string::const_iterator Lexer::read_until(char end_ch) const
{
	std::string::const_iterator it = _current;
	bool end_found = false;
	while (it != _end)
	{
		end_found = (*it) == end_ch;
		if (end_found)
		{
			break;
		}

		++it;
	}

	return end_found ? it : _current;
}

ObjectPtr convert(const std::string_view& number)
{
	if (number.find_first_of('.') == std::string_view::npos)
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

	return {};
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
	while (_current != _end)
	{
		const char ch = *_current;
		if(ch == ' ' || ch == '\t')
		{
			++_current;
		}
		else
		{
			break;
		}
	}

	uint32_t expect = TT_Let | TT_Id | TT_ScopeBegin | TT_ScopeEnd;
 	while (_current != _end)
	{
		eat_until_not(' ');

		if((*_current) == '\n')
		{
			break;
		}

		if ((expect & TT_Semicolon) && try_put_token(TT_Semicolon, ';'))
		{
			break;
		}

		if ((expect & TT_ScopeBegin) && try_put_token(TT_ScopeBegin, '{'))
		{
			expect = TT_Let | TT_Id | TT_ScopeBegin | TT_ScopeEnd;
		}
		else if ((expect & TT_ScopeEnd) && try_put_token(TT_ScopeEnd, '}'))
		{
			expect = TT_Let | TT_Id | TT_ScopeBegin;
		}
		else if ((expect & TT_LParen) && try_put_token(TT_LParen, '('))
		{
			expect = TT_Id | TT_NumberLiteral | TT_StringLiteral | TT_LParen | TT_RParen;
		}
		else if ((expect & TT_RParen) && try_put_token(TT_RParen, ')'))
		{
			expect = TT_Operation | TT_Semicolon | TT_RParen | TT_ScopeBegin;
		}
		else if((expect & TT_Coma) && try_put_token(TT_Coma, ','))
		{
			expect = TT_Id | TT_NumberLiteral | TT_StringLiteral;
		}
		else if ((expect & TT_Assign) && try_put_token(TT_Assign, '='))
		{
			expect = TT_Id | TT_NumberLiteral | TT_LParen | TT_Fn | TT_StringLiteral;
		}
		else if ((expect & TT_Let) && try_put_token(TT_Let, "let"))
		{
			expect = TT_Id;
		}
		else if((expect & TT_Fn) && try_put_token(TT_Fn, "fn"))
		{
			expect = TT_LParen | TT_NumberLiteral | TT_StringLiteral | TT_Id;
		}
		else if ((expect & TT_NumberLiteral) && try_put_number_literal())
		{
			expect = TT_Operation | TT_Semicolon | TT_RParen | TT_Coma;
		}
		else if ((expect & TT_StringLiteral) && try_put_string_literal())
		{
			expect = TT_Operation | TT_Semicolon | TT_RParen | TT_Coma;
		}
		else if ((expect & TT_Operation) && try_put_operation())
		{
			expect = TT_Id | TT_NumberLiteral | TT_LParen | TT_StringLiteral;
		}
		else if ((expect & TT_Id) && try_put_id())
		{
			expect = TT_Assign | TT_Operation | TT_Semicolon | TT_LParen | TT_Coma;
		}
		else if((*_current) != ' ' && (*_current) != '\n')
		{
			auto err_msg = std::format("Unexpected token type {}", *_current);
			fatal_error(err_msg);
		}
	}

	eat_until_not('\r');
	eat_until_not('\n');

	if (_current != _end)
	{
		fatal_error("Unexpected characters after semicolon");
	}
}

bool Lexer::try_put_token(TokType tok, char ch)
{
	if ((*_current) == ch)
	{
		eat_current();
		_tokens.emplace_back(tok);
		return true;
	}
	return false;
}

bool Lexer::try_put_token(TokType tok, const std::string_view& match_word)
{
	if (isalpha(*_current) > 0)
	{
		const auto word = read_word();

		if (word == match_word)
		{
			eat(word);
			_tokens.emplace_back(tok);

			return true;
		}
	}
	return false;
}

bool Lexer::try_put_number_literal()
{
	if(isdigit(*_current) > 0)
	{
		const auto number = read_number();
		eat(number);
		_tokens.emplace_back(TT_NumberLiteral, convert(number));
		
		return true;
	}
	return false;
}

bool Lexer::try_put_string_literal()
{
	constexpr char quote = '\"';
	if((*_current) == quote)
	{
		eat(quote);
		const auto end = read_until(quote);
		if (_current != end)
		{
			_tokens.emplace_back(TT_StringLiteral, std::make_shared<String>(std::string{_current, end}));

			_current = end;
			eat(quote);

			return true;
		}
	}
	return false;
}

bool Lexer::try_put_operation()
{
	if(auto op = match_op(*_current))
	{
		_tokens.emplace_back(op.value());
		eat_current();
		return true;
	}

	return false;
}

bool Lexer::try_put_id()
{
	if(isalpha(*_current) > 0)
	{
		const auto word = read_word();
		eat(word);
		_tokens.emplace_back(TT_Id, std::string{word});
		
		return true;
	}

	return false;
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

void Lexer::eat_current()
{
	eat(*_current);
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

void Lexer::eat(const std::string_view& word)
{
	for(auto& ch : word)
	{
		eat(ch);
	}
}

void Lexer::fatal_error(const std::string& error_msg)
{
	auto offset = _current - _begin;
	auto msg = std::format("{}:{} > {}", _current_line, offset, error_msg);
	puts(msg.c_str());
	exit(1);
}
