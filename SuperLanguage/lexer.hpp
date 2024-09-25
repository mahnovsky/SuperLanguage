#pragma once

#include <memory>

#include "utils.hpp"
#include "object.hpp"

#include <variant>

#include "number.hpp"

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
	TT_And = 1 << 25,

	TT_If = 1 << 26,
	TT_Else = 1 << 27,
	TT_Loop = 1 << 28,
	TT_ArrayBegin = 1 << 29,
	TT_ArrayEnd = 1 << 30
};

struct StackObject
{
	static constexpr uint32_t magic_number = 0xFFFAAFFF;

	struct NumberBox
	{
		uint32_t x = sizeof(Number);//magic_number;
		Number number;
	};

	union Value
	{
		ObjectPtr ptr; // 16 byte
		NumberBox box;
	};



	Value val;
};

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
	std::optional<TokType> match_op(char ch);

	std::string_view read_word() const;

	std::string_view read_number() const;

	std::string::const_iterator read_until(char end_ch) const;

	void process_line();

	bool try_put_token(TokType tok);

	bool try_put_keyword_token(TokType tok);

	bool try_put_bool_literal();

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

	void skip_fillers();

	void end_line();

	bool find_keyword(uint32_t& expect);

	bool find_char(uint32_t& expect);

	bool find_literal(uint32_t& expect);

	void process_begin();

	void process_assign();

	void process_expression();

private:
	std::vector<Token> _tokens;
	int _current_line = 0;
	std::string::const_iterator _begin;
	std::string::const_iterator _current;
	std::string::const_iterator _end;
	//uint32_t _expect;
};
