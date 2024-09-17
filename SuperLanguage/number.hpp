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
	Number do_op(float other) const
	{
		if (_is_int)
		{
			return Op::eval(other, _value.i_num);
		}
		return Op::eval(other, _value.f_num);
	}

	template <class Op>
	Number do_op(int other) const
	{
		if (_is_int)
		{
			return Op::eval(other, _value.i_num);
		}
		return Op::eval(other, _value.f_num);
	}

	template <class Op>
	Number do_op(Number other) const
	{
		if(_is_int)
		{
			return other.do_op<Op>(_value.i_num);
		}
		return other.do_op<Op>(_value.f_num);
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
