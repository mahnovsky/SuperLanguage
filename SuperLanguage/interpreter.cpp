#include "interpreter.hpp"

#include <cassert>

Interpreter::Interpreter(Node* scope)
	:_root_scope(scope)
	,_current_scope(nullptr)
{}

Interpreter::~Interpreter()
{
	delete _root_scope;
}

void Interpreter::run()
{
	if (_root_scope)
	{
		_root_scope->accept(*this);
	}
}

void Interpreter::visit(Scope* node)
{
	const auto parent_scope = _current_scope;
	_current_scope = node;
	auto& nodes = node->get_nodes();

	for (Node* child : nodes)
	{
		child->accept(*this);
	}

	if (_current_scope)
	{
		_stack.resize(_stack.size() - _current_scope->get_variable_count());
	}
	_current_scope = parent_scope;
}

void Interpreter::visit(BinaryOperation* node)
{
	node->get_left()->accept(*this);
	node->get_right()->accept(*this);

	switch (node->get_operation())
	{
	case '+': eval_plus(); break;
	case '-': eval_minus(); break;
	case '*': eval_mul(); break;
	case '/': eval_div(); break;
	default: break;
	}
}

void Interpreter::visit(NumberLiteral* node)
{
	_stack.emplace_back(node->get_number());
}

void Interpreter::visit(Variable* node)
{
	const auto index = _current_scope->apply_index_offset(node->get_stack_index());

	if(index < _stack.size())
	{
		_stack.emplace_back(_stack[index]);
	}
	else
	{
		assert(false);
	}
}

void Interpreter::visit(Assign* node)
{
	if(auto scope = dynamic_cast<Scope*>(node->get_expression()))
	{
		_stack.push_back(std::make_shared<Callable>(scope));
	}
	else
	{
		node->get_expression()->accept(*this);
	}

	const auto val = _stack.back();
	_stack.pop_back();

	if (node->is_declaration())
	{
		allocate_stack_variable(node->get_var_index());
		set_stack_variable(node->get_var_index(), val);
	}
	else
	{
		if (!set_stack_variable(node->get_var_index(), val))
		{
			const auto err = std::format("Failed to assign, variable \'{}\' not exist in current scope", node->get_var_index());
			puts(err.c_str());
		}
	}
	
}

void Interpreter::visit(StringLiteral* node)
{
	_stack.emplace_back(node->get_string());
}

void Interpreter::visit(Function* node)
{
	const auto& name = node->get_name();
	if(_functions.find(name) == _functions.end())
	{
		_functions[name] = node;
	}
}

void Interpreter::visit(Call* node)
{
	if(const auto func = get_function(node))
	{
		const auto str = std::format("Call function {}", func->get_name());
		puts(str.c_str());
		const auto base_index = _stack.size();
		const auto prev_scope = _current_scope;
		_current_scope = func->get_scope();
		const auto& args = node->get_args();
		puts("Function args begin");
		for(const auto arg : args)
		{
			const auto prev_size = _stack.size();
			arg->accept(*this);
			if (_stack.size() > prev_size)
			{
				const auto arg_res = _stack.back();
				_stack.pop_back();
				const auto index = _stack.size();
				allocate_stack_variable(index);
				set_stack_variable(index, arg_res);
			}
		}
		puts("Function args end");
		_current_scope = prev_scope;

		func->run(this, base_index);

		puts("Function call end");
	}
}

void Interpreter::eval_plus()
{
	int right = 0;
	int left = 0;
	float fright = 0;
	float fleft = 0;

	if (pop_stack(right))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Integer>(left + right));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft + static_cast<float>(right)));
		}
	}
	else if (pop_stack(fright))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Float>(static_cast<float>(left) + fright));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft + fright));
		}
	}
	else
	{
		std::string rvalue;
		std::string lvalue;
		if (pop_stack(rvalue) && pop_stack(lvalue))
		{
			_stack.emplace_back(std::make_shared<String>(lvalue + rvalue));
		}
	}
}

void Interpreter::eval_minus()
{
	int right = 0;
	int left = 0;
	float fright = 0;
	float fleft = 0;

	if (pop_stack(right))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Integer>(left - right));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft - static_cast<float>(right)));
		}
	}
	else if (pop_stack(fright))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Float>(static_cast<float>(left) - fright));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft - fright));
		}
	}
}

void Interpreter::eval_mul()
{
	int right = 0;
	int left = 0;
	float fright = 0;
	float fleft = 0;

	if (pop_stack(right))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Integer>(left * right));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft * static_cast<float>(right)));
		}
	}
	else if (pop_stack(fright))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Float>(static_cast<float>(left) * fright));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft * fright));
		}
	}
}

void Interpreter::eval_div()
{
	int right = 0;
	int left = 0;
	float fright = 0;
	float fleft = 0;

	if (pop_stack(right))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Integer>(left / right));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft / static_cast<float>(right)));
		}
	}
	else if (pop_stack(fright))
	{
		if (pop_stack(left))
		{
			_stack.emplace_back(std::make_shared<Float>(static_cast<float>(left) / fright));
		}
		else if (pop_stack(fleft))
		{
			_stack.emplace_back(std::make_shared<Float>(fleft / fright));
		}
	}
}

std::string Interpreter::print_value(ObjectPtr value) const
{
	int ival = 0;
	float fval = 0;
	std::string s;
	if (value->get(&ival))
	{
		return std::format("value: {}", ival);
	}
	else if (value->get(&fval))
	{
		return std::format("value: {}", fval);
	}
	else if(value->get(&s))
	{
		return std::format("value: {}", s);
	}
	return {};
}

void Interpreter::allocate_stack_variable(size_t index)
{
	index = _current_scope->apply_index_offset(index);
	if(index >= _stack.size())
	{
		_stack.resize(index + 1);
		
		_current_scope->add_variable();

		const auto str = std::format("Allocate on stack {}", index);
		puts(str.c_str());
	}
}

bool Interpreter::set_stack_variable(size_t index, ObjectPtr object)
{
	index = _current_scope->apply_index_offset(index);
	const bool res = _stack.size() > index;
	if (res)
	{
		_stack[index] = object;

		const auto str = std::format("Var {} set to {}", index, print_value(object));
		puts(str.c_str());
	}
	return res;
}

Function* Interpreter::get_function(Call* node)
{
	const std::string name{node->get_function_name()};
	if (const auto it = _functions.find(name); it != _functions.end())
	{
		return it->second;
	}
	else
	{
		const auto index = _current_scope->apply_index_offset(node->get_var_index());

		if (_stack.size() < index)
		{
			Function* func = nullptr;
			if (_stack[index]->get(&func) && func)
			{
				return func;
			}
		}
	}
	return nullptr;
}
