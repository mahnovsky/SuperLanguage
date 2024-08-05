#pragma once

#include <memory>

#include "utils.hpp"
#include "object.hpp"

#include <variant>

enum class TokType : uint32_t
{
	Let				= 1 << 1,
	Operation		= 1 << 2,
	Id				= 1 << 3,
	NumberLiteral	= 1 << 4,
	Assign			= 1 << 5,
	Semicolon		= 1 << 6,

	Plus			= 1 << 7,
	Minus			= 1 << 8,
	Mul				= 1 << 9,
	Div				= 1 << 10,

	LParen			= 1 << 11,
	RParen			= 1 << 12,

	ScopeBegin		= 1 << 13,
	ScopeEnd		= 1 << 14
};

#define TOK_INT(tok) static_cast<uint32_t>(TokType:: tok)

using ObjectPtr = std::shared_ptr<Object>;

struct Token
{
	TokType type;
	ObjectPtr object;
	std::string name;

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

	std::string_view read_word();

	ObjectPtr read_number();

	void process_line();

	void put_number_literal(ObjectPtr number);

	void put_operation(char op);

	void put_declaration();

	void put_semicolon();

	void put_assign(std::string&& name);

	void put_id(std::string&& name);

	void put_lparen();

	void put_rparen();

	void put_scope_begin();

	void put_scope_end();

	void eat(char ch);

	void eat_until_not(char ch, bool expect_once = false);

	void fatal_error(const std::string& error_msg);

private:
	std::vector<Token> _tokens;
	int _current_line = 0;
	std::string::const_iterator _begin;
	std::string::const_iterator _current;
	std::string::const_iterator _end;
};
