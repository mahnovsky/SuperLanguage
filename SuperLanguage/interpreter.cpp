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
	auto parent_scope = _current_scope;
	_current_scope = node;
	auto& nodes = node->get_nodes();

	for (Node* child : nodes)
	{
		child->accept(*this);
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
	if(const auto object = _current_scope->get_variable(node->get_name()))
	{
		_stack.emplace_back(object);
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

	if (node->is_declaration())
	{
		_current_scope->set_variable(node->get_var_name(), _stack.back(), true);
		
		const auto str = std::format("Var {} set to {}", node->get_var_name(), print_stack_value());
		puts(str.c_str());
	}
	else
	{
		Scope* src_scope = nullptr;
		const auto var_name = node->get_var_name();
		auto it = _current_scope->get_variable(var_name, &src_scope);
		if (it && src_scope)
		{
			src_scope->set_variable(var_name, _stack.back());

			const auto str = std::format("Var {} set to {}", node->get_var_name(), print_stack_value());
			puts(str.c_str());
		}
		else
		{
			const auto err = std::format("Failed to assign, variable \'{}\' not exist in current scope", node->get_var_name());
			puts(err.c_str());
		}
	}
	_stack.pop_back();
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
	if (const auto object = _current_scope->get_variable(node->get_name()))
	{
		Scope* scope = nullptr;
		if(object->get(&scope) && scope)
		{
			scope->set_parent(_current_scope);
			scope->accept(*this);
		}
		else
		{
			assert(false);
		}
	}
	else
	{
		assert(false);
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

std::string Interpreter::print_stack_value() const
{
	const auto value = _stack.back();
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
