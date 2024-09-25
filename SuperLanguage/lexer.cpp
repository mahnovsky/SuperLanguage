#include "lexer.hpp"

#include <cassert>
#include <cctype>
#include <charconv>
#include <format>
#include <map>

#include "log.hpp"



namespace expects {
	constexpr uint32_t LITERALS = TT_NumberLiteral | TT_StringLiteral | TT_BoolLiteral;
	constexpr uint32_t LET = TT_Id;
	constexpr uint32_t SCOPE_BEGIN = TT_Let | TT_Id | TT_ScopeBegin | TT_Fn | TT_Ret | TT_If | TT_Loop;
	constexpr uint32_t SCOPE_END = TT_Let | TT_Id | TT_ScopeBegin | TT_Fn | TT_Ret | TT_If | TT_Else | TT_Loop;
	//TT_Id | TT_NumberLiteral | TT_LParen | TT_Fn | TT_StringLiteral;
	constexpr uint32_t ASSIGN = TT_Id | TT_LParen | TT_Fn | LITERALS | TT_ArrayBegin;
	constexpr uint32_t ID = TT_Assign | TT_Operation | TT_Semicolon | TT_LParen | TT_Coma | TT_RParen;
	constexpr uint32_t LPAREN = TT_Id | LITERALS | TT_LParen | TT_RParen;
	constexpr uint32_t RPAREN = TT_LParen | TT_RParen | TT_Operation | TT_ScopeBegin | TT_Semicolon;
	constexpr uint32_t COMA = TT_Id | LITERALS;
	constexpr uint32_t FN = TT_LParen | TT_NumberLiteral | TT_StringLiteral | TT_Id | TT_Ret;
	constexpr uint32_t RETURN = TT_LParen | TT_NumberLiteral | TT_StringLiteral | TT_Id;
	constexpr uint32_t NUMBER_LITERAL = TT_Operation | TT_Semicolon | TT_RParen | TT_Coma | TT_ArrayEnd;
	constexpr uint32_t STRING_LITERAL = TT_Operation | TT_Semicolon | TT_RParen | TT_Coma | TT_ArrayEnd;
	constexpr uint32_t BOOL_LITERAL = TT_Operation | TT_Semicolon | TT_RParen | TT_Coma | TT_ArrayEnd;
	constexpr uint32_t OPERATION = TT_Id | TT_NumberLiteral | TT_LParen | TT_StringLiteral;
	constexpr uint32_t IF = TT_LParen;
	constexpr uint32_t ELSE = TT_ScopeBegin;
	constexpr uint32_t LOOP = TT_LParen;
	constexpr uint32_t AND = TT_Id | LITERALS;
	constexpr uint32_t OR = TT_Id | LITERALS;
	constexpr uint32_t GREATER = TT_Id | TT_NumberLiteral;
	constexpr uint32_t LESS = TT_Id | TT_NumberLiteral;
	constexpr uint32_t SEMICOLON = TT_ScopeEnd | TT_Let | TT_Id;
	constexpr uint32_t ARRAY_BEGIN = TT_Id | LITERALS;
}

using CharTokenInfo = std::tuple< char, uint32_t>;
std::map<uint32_t, CharTokenInfo> char_map = {
	{ TT_LParen,  { '(', expects::LPAREN } },
	{ TT_RParen, { ')', expects::RPAREN } },
	{ TT_ScopeBegin, { '{', expects::SCOPE_BEGIN} },
	{ TT_ScopeEnd, { '}', expects::SCOPE_END} },
	{ TT_Assign, { '=', expects::ASSIGN} },
	{ TT_Coma, { ',', expects::COMA} },
	{ TT_Semicolon, { ';', 0 } },
	{ TT_ArrayBegin, { '[', expects::ARRAY_BEGIN } },
	{ TT_ArrayEnd, { ']', TT_Semicolon } }
};

using TokenInfo = std::tuple< std::string_view, uint32_t>;
std::map<uint32_t, TokenInfo> string_map = {
	{ TT_Let,  { "let", expects::LET } },
	{ TT_Fn, { "fn", expects::FN } },
	{ TT_Ret, { "return", expects::RETURN } },
	{ TT_If, { "if", expects::IF } },
	{ TT_Else, { "else", expects::ELSE } },
	{ TT_Loop, { "while", expects::LOOP } },
	{ TT_And, { "and", expects::AND } },
	{ TT_Or, {"or", expects::OR } }
};

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
	case '>': return TT_Greater;
	case '<': return TT_Less;
	case '=': return TT_Equal;
	default: return {};
	}
}

std::string_view Lexer::read_word() const
{
	std::string::const_iterator it = _current;
	while(it != _end)
	{
		if(isalpha(*it) > 0 || (it != _current && isdigit(*it)) || (*it) == '_')
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
		++_current_line;
		if(line.front() == '\n' || line.front() == '#')
		{
			continue;
		}
		_begin = line.begin();
		_current = line.begin();
		_end = line.end();
		process_line();
	}

	return _tokens;
}

void Lexer::process_line()
{
	while (_current != _end)
	{
		const char ch = *_current;
		if (ch == '#')
		{
			return;
		}

		if(ch != ' ' && ch != '\t')
		{
			break;
		}
		
		++_current;
	}

	uint32_t expect = TT_Let | TT_Id | TT_ScopeBegin | TT_Fn | TT_Ret | TT_ScopeEnd | TT_If | TT_Else | TT_Loop;

	struct ScopeEnd
	{
		ScopeEnd(Lexer* l)
			:lexer(l)
		{}

		ScopeEnd(const ScopeEnd&) = delete;
		ScopeEnd(ScopeEnd&&) = delete;

		~ScopeEnd()
		{
			if(lexer)
			{
				lexer->fill_last_token();
			}
		}

		Lexer* lexer;
	};

 	while (_current != _end)
	{
		eat_until_not(' ');

		if((*_current) == '\n')
		{
			break;
		}
		const auto prev_size = _tokens.size();
		if ((expect & TT_Semicolon) && try_put_token(TT_Semicolon))
		{
			break;
		}

		ScopeEnd scope_end{ this };

		if( find_keyword(expect) )
		{
			continue;
		}

		if(find_char(expect))
		{
			continue;
		}

		if(find_literal(expect))
		{
			continue;
		}

		if ((expect & TT_Operation) && try_put_operation())
		{
			expect = expects::OPERATION;
			const auto last_type = _tokens.back().type;
			if (last_type == TT_Greater || last_type == TT_Less)
			{
				expect |= TT_Operation;
			}
			continue;
		}

		if ((expect & TT_Id) && try_put_id())
		{
			expect = expects::ID;
			continue;
		}

 		if((*_current) != ' ' && (*_current) != '\n')
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

bool Lexer::try_put_token(TokType tok)
{
	if(const auto it = char_map.find(tok); it != char_map.end())
	{
		skip_fillers();

		if ((*_current) == std::get<char>(it->second))
		{
			eat_current();
			_tokens.emplace_back(tok);
			return true;
		}
		
	}

	LOG_INFO("Cant convert token {} to char", tok);
	return false;
}

bool Lexer::try_put_keyword_token(TokType tok)
{
	if (const auto it = string_map.find(tok); it != string_map.end())
	{
		skip_fillers();
		if (isalpha(*_current) > 0)
		{
			const auto word = read_word();

			if (word == std::get<std::string_view>(it->second))
			{
				eat(word);
				_tokens.emplace_back(tok);

				return true;
			}
		}
	}
	LOG_INFO("Cant convert token {} to string", tok);
	return false;
}

bool Lexer::try_put_bool_literal()
{
	skip_fillers();
	if ((*_current == 'T') || (*_current) == 'F')
	{
		const auto word = read_word();
		const bool is_true = word == "True";
		if(is_true || word == "False")
		{
			eat(word);
			_tokens.emplace_back(TT_BoolLiteral, std::make_shared<Bool>(is_true));
		}
		

		return true;
	}
	return false;
}

bool Lexer::try_put_number_literal()
{
	skip_fillers();
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
	skip_fillers();
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
	skip_fillers();
	if(auto op = match_op(*_current))
	{
		_tokens.emplace_back(op.value());
		eat_current();
		/*
		const auto prev_token = _tokens.back();
		if (op.value() == TT_Equal && prev_token.type == TT_Greater || prev_token.type == TT_Less || prev_token.type == TT_Equal)
		{
			eat('=');
		}*/
		return true;
	}

	return false;
}

bool Lexer::try_put_id()
{
	if(isalpha(*_current) > 0 || (*_current) == '_')
	{
		skip_fillers();
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

void Lexer::fill_last_token()
{
	if (!_tokens.empty())
	{
		_tokens.back().line = _current_line;
		_tokens.back().pos = _current - _begin;
	}
}

void Lexer::skip_fillers()
{
	eat_until_not('\t');
	eat_until_not(' ');
}

void Lexer::end_line()
{
	skip_fillers();
	if(!try_put_token(TT_Semicolon))
	{
		fatal_error("Semicolon expected");
	}
}

bool Lexer::find_keyword(uint32_t& expect)
{
	constexpr uint32_t keyword_tokens[] = { TT_Let, TT_Fn, TT_Ret, TT_If, TT_Else, TT_Loop };
	for (uint32_t i = 0; i < sizeof(keyword_tokens); ++i)
	{
		auto token = keyword_tokens[i];
		if ((expect & token) && try_put_keyword_token(static_cast<TokType>(token)))
		{
			if (auto it = string_map.find(token); it != string_map.end())
			{
				expect = std::get<uint32_t>(it->second);
				return true;
			}
		}
	}
	return false;
}

bool Lexer::find_char(uint32_t& expect)
{
	constexpr uint32_t char_tokens[] = { TT_ScopeBegin, TT_ScopeEnd, TT_Assign, TT_LParen, TT_RParen, TT_Coma, TT_ArrayBegin, TT_ArrayEnd };
	for (uint32_t token : char_tokens)
	{
		if ((expect & token) && try_put_token(static_cast<TokType>(token)))
		{
			if (auto it = char_map.find(token); it != char_map.end())
			{
				expect = std::get<uint32_t>(it->second);
				return true;
			}
		}
	}
	return false;
}

bool Lexer::find_literal(uint32_t& expect)
{
	bool res = (expect & TT_BoolLiteral) && try_put_bool_literal();
	if(res) 
	{
		expect = expects::BOOL_LITERAL;
		return res;
	}

	res = (expect & TT_NumberLiteral) && try_put_number_literal();
	if (res)
	{
		expect = expects::NUMBER_LITERAL;
		return res;
	}

	res = (expect & TT_StringLiteral) && try_put_string_literal();
	if (res)
	{
		expect = expects::STRING_LITERAL;
		return res;
	}

	return res;
}

void Lexer::process_begin()
{
	if(try_put_token(TT_ScopeBegin))
	{
		process_begin();
	}
	else if (try_put_token(TT_ScopeEnd))
	{
		
	}
	else if(try_put_keyword_token(TT_Let))
	{
		process_assign();
	}
	else if(try_put_keyword_token(TT_If))
	{
		process_expression();
	}
	else if (try_put_keyword_token(TT_Else))
	{
		
	}
	else
	{
		process_assign();
	}
}

void Lexer::process_assign()
{
	if (try_put_id())
	{
		try_put_token(TT_Assign);

		process_expression();
	}
	else
	{
		fatal_error("Failed process assign, variable name expected");
	}
}

void Lexer::process_expression()
{
	if(try_put_token(TT_LParen) || try_put_token(TT_RParen))
	{
		process_expression();
	}
	else if(try_put_id() || try_put_number_literal() || try_put_string_literal())
	{
		if(try_put_operation())
		{
			process_expression();
		}
	}
	else
	{
		fatal_error("Error");
	}
}
