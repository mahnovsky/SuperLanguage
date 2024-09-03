#pragma once

#include <memory>

#include "utils.hpp"
#include "object.hpp"

#include <variant>

enum TokType : uint32_t
{
	TT_Let = 1 << 1,
	TT_Operation = 1 << 2,
	TT_Id = 1 << 3,
	TT_NumberLiteral = 1 << 4,
	TT_Assign = 1 << 5,
	TT_Semicolon = 1 << 6,

	TT_Plus = 1 << 7,
	TT_Minus = 1 << 8,
	TT_Mul = 1 << 9,
	TT_Div = 1 << 10,

	TT_LParen = 1 << 11,
	TT_RParen = 1 << 12,

	TT_ScopeBegin = 1 << 13,
	TT_ScopeEnd = 1 << 14,

	TT_StringLiteral = 1 << 15, 

	TT_Coma = 1 << 16,

	TT_Fn = 1 << 17,
	TT_Ret = 1 << 18,

	TT_BoolLiteral = 1 << 19,
	TT_Equal = 1 << 20,
	TT_Greater = 1 << 21,
	TT_Less = 1 << 22,

	TT_Not = 1 << 23,
	TT_Or = 1 << 24,
	TT_And = 1 << 25
};

using ObjectPtr = std::shared_ptr<Object>;

struct Token
{
	TokType type;
	ObjectPtr object;
	std::string name;
	int line = 0;
	int pos = 0;

	Token(TokType t)
		:type(t)
	{}

	Token(TokType t, std::string&& n)
		:type(t)
		,name(std::move(n))
	{}

	Token(TokType t, ObjectPtr v);
};

class Lexer
{
public:
	std::vector<Token> tokenize(const std::string& expression);

private:
	static std::optional<TokType> match_op(char ch);

	std::string_view read_word() const;

	std::string_view read_number() const;

	std::string::const_iterator read_until(char end_ch) const;

	void process_line();

	bool try_put_token(TokType tok, char ch);

	bool try_put_token(TokType tok, const std::string_view& match_word);

	bool try_put_number_literal();

	bool try_put_string_literal();

	bool try_put_operation();

	bool try_put_id();

	void eat(char ch);

	void eat_current();

	void eat_until_not(char ch, bool expect_once = false);

	void eat(const std::string_view& word);

	void fatal_error(const std::string& error_msg);

	void fill_last_token();

private:
	std::vector<Token> _tokens;
	int _current_line = 0;
	std::string::const_iterator _begin;
	std::string::const_iterator _current;
	std::string::const_iterator _end;
	//uint32_t _expect;
};
