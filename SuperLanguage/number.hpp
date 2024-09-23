#pragma once
#include <optional>

#include "lexer.hpp"



class Number
{
public:
	static std::optional<Number> get_from_object(ObjectPtr obj)
	{
		int val;
		if(obj->get(&val))
		{
			return Number{ val };
		}

		float fval;
		if(obj->get(&fval))
		{
			return Number{ fval };
		}
		return {};
	}

	ObjectPtr as_object() const
	{
		if(_is_int)
		{
			return std::make_shared<Integer>(_value.i_num);
		}

		return std::make_shared<Float>(_value.f_num);
	}

	Number()
		:_value{ .i_num = 0 }
		, _is_int(true)
	{}

	explicit Number(int v)
		:_value{ .i_num = v }
		, _is_int(true)
	{}

	explicit Number(float v)
		:_value{ .f_num = v }
		, _is_int(false)
	{}

	template <class Op>
	Number perform_op(float other) const
	{
		if (_is_int)
		{
			return Op::eval(other, _value.i_num);
		}
		return Op::eval(other, _value.f_num);
	}

	template <class Op>
	Number perform_op(int other) const
	{
		if (_is_int)
		{
			return Op::eval(other, _value.i_num);
		}
		return Op::eval(other, _value.f_num);
	}

	template <class Op>
	Number perform_op(Number other) const
	{
		if(_is_int)
		{
			return other.perform_op<Op>(_value.i_num);
		}
		return other.perform_op<Op>(_value.f_num);
	}

	template <class Op>
	bool perform_bool_op(int other) const
	{
		if (_is_int)
		{
			return Op::eval(other, _value.i_num);
		}
		return Op::eval(other, _value.f_num);
	}

	template <class Op>
	bool perform_bool_op(float other) const
	{
		if (_is_int)
		{
			return Op::eval(other, _value.i_num);
		}
		return Op::eval(other, _value.f_num);
	}

	template <class Op>
	bool perform_bool_op(Number other) const
	{
		if (_is_int)
		{
			return other.perform_bool_op<Op>(_value.i_num);
		}
		return other.perform_bool_op<Op>(_value.f_num);
	}

private:
	union Value
	{
		int i_num;
		float f_num;
	};
	Value _value;
	bool _is_int;
};

#define GENERATE_OP(name, op) \
struct name ## Op \
{ \
	static Number eval(int a, int b) { return Number{ a op b }; } \
	static Number eval(float a, float b) { return Number{ a op b }; } \
	static Number eval(int a, float b) { return Number{ a op b }; } \
	static Number eval(float a, int b) { return Number{ a op b }; } \
};

GENERATE_OP(Plus, +);
GENERATE_OP(Minus, -);
GENERATE_OP(Mul, *);
GENERATE_OP(Div, /);

#define GENERATE_BOOL_OP(name, op) \
struct name ## Op \
{ \
	static bool eval(int a, int b) { return a op b; } \
	static bool eval(float a, float b) { return a op b; } \
	static bool eval(int a, float b) { return a op b; } \
	static bool eval(float a, int b) { return a op b ; } \
};

GENERATE_BOOL_OP(Greater, >);
GENERATE_BOOL_OP(Less, <);
GENERATE_BOOL_OP(Equal, ==);
GENERATE_BOOL_OP(EqualGreater, >=);
GENERATE_BOOL_OP(EqualLess, <= );