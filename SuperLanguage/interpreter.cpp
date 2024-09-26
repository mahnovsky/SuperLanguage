#include "interpreter.hpp"
#include "log.hpp"
#include <cassert>

Interpreter::Interpreter(Node* scope)
	:_root_scope(scope)
	,_current_scope(dynamic_cast<Scope*>(scope))
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

ObjectPtr Interpreter::get_stack_variable(size_t index) const
{
	if(index < _stack.size())
	{
		return _stack[index];
	}

	return {};
}

void Interpreter::put_on_stack(ObjectPtr obj)
{
	_stack.emplace_back(obj);
}

size_t Interpreter::get_stack_size() const
{
	return _stack.size();
}

void Interpreter::add_internal_function(InternalFunction* func)
{
	_functions[func->get_name()] = func;
}

void Interpreter::run_once(Node* node)
{
	node->accept(*this);
}

void Interpreter::visit(Scope* node)
{
	const auto parent_scope = _current_scope;
	_current_scope = node;
	if (parent_scope)
	{
		_current_scope->set_stack_base(parent_scope->get_stack_base());
	}
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
	case Operation::Plus:			eval_plus();			break;
	case Operation::Minus:			eval_minus();			break;
	case Operation::Mul:			eval_mul();				break;
	case Operation::Div:			eval_div();				break;
	case Operation::Equal:			eval_equal();			break;
	case Operation::Greater:		eval_greater();			break;
	case Operation::Less:			eval_less();			break;
	case Operation::EqualGreater:	eval_equal_greater();	break;
	case Operation::EqualLess:		eval_equal_less();		break;
	}
}

void Interpreter::visit(Variable* node)
{
	const auto index = get_absolute_address(node->get_stack_index());

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
	else if (!set_stack_variable(node->get_var_index(), val))
	{
		LOG_INFO("Failed to assign, variable \'{}\' not exist in current scope", node->get_var_index());
	}
}

void Interpreter::visit(StackValue* node)
{
	_stack.emplace_back(node->get_object());
}

void Interpreter::visit(ArrayNode* node)
{
	const auto& array_nodes = node->get_array_nodes();

	std::vector<ObjectPtr> array_objects;
	for(const auto& node : array_nodes)
	{
		const auto stack_size = _stack.size();

		node->accept(*this);

		if(stack_size < _stack.size())
		{
			array_objects.emplace_back(_stack.back());
			_stack.pop_back();
		}
	}

	_stack.emplace_back(std::make_shared<ArrayObj>(array_objects));
}

void Interpreter::visit(Function* node)
{
	const auto& name = node->get_name();
	if(_functions.find(name) == _functions.end())
	{
		_functions[name] = node;
	}
}

void Interpreter::visit(InternalFunction* node)
{
	// ? 
}

void Interpreter::visit(Call* node)
{
	if(const auto func = get_function(node))
	{
		LOG_INFO("Call function {}", func->get_name());
		
		const auto base_index = _stack.size();
		const auto fn_scope = func->get_scope();
		fn_scope->reset();
		const auto& args = node->get_args();
		LOG_INFO("Function args begin");
		for(const auto arg : args)
		{
			const auto prev_size = _stack.size();
			arg->accept(*this);
			fn_scope->add_variable();
			const auto str_val = print_value(_stack.back());
			LOG_INFO("Arg {} set value to {}", prev_size, str_val);
		}
		LOG_INFO("Function args end");

		_call_stack.emplace_back(func->get_name(), base_index);
		func->run(this, base_index);

		if(_return_value)
		{
			constexpr auto ret_index = 0;
			allocate_stack_variable(ret_index);
			set_stack_variable(ret_index, std::move(_return_value));
		}
		_call_stack.pop_back();

		LOG_INFO("Function call end {}", func->get_name());
	}
}

void Interpreter::visit(Return* node)
{
	if(const auto expr = node->get_expression())
	{
		const auto prev_size = _stack.size();
		expr->accept(*this);

		if(_stack.size() > prev_size)
		{
			_return_value = _stack.back();
			_stack.pop_back();
		}
	}
}

void Interpreter::visit(BranchIfElse* node)
{
	node->get_expression()->accept(*this);
	bool value;
	if(pop_stack(value))
	{
		node->execute(*this, value);
	}
	else
	{
		LOG_ERROR("Failed to execute if statement, bool value expected");
	}
}

void Interpreter::visit(Loop* node)
{
	const auto expr = node->get_expression();
	const auto scope = node->get_scope();
	if (!scope)
	{
		LOG_ERROR("Failed to execute loop, no scope to execute");
		return;
	}

	bool value = false;
	do
	{
		expr->accept(*this);
		if (pop_stack(value))
		{
			if (!value)
			{
				break;
			}
			scope->accept(*this);
		}
		else
		{
			LOG_ERROR("Failed to execute loop statement, bool value expected");
			break;
		}
	}
	while (value);
}

void Interpreter::eval_plus()
{
	if (!try_perform_op<PlusOp>()) 
	{
		std::string rvalue;
		std::string lvalue;
		if (pop_stack(rvalue) && pop_stack(lvalue))
		{
			_stack.emplace_back(std::make_shared<String>(lvalue + rvalue));
		}
		else
		{
			LOG_ERROR("Failed to perform plus operation");
		}
	}
}

void Interpreter::eval_minus()
{
	if(!try_perform_op<MinusOp>())
	{
		LOG_ERROR("Failed to perform minus operation");
	}
}

void Interpreter::eval_mul()
{
	if (!try_perform_op<MulOp>())
	{
		LOG_ERROR("Failed to perform mul operation");
	}
}

void Interpreter::eval_div()
{
	if (!try_perform_op<DivOp>())
	{
		LOG_ERROR("Failed to perform mul operation");
	}
}

void Interpreter::eval_greater()
{
	perform_bool_op<GreaterOp>();
}

void Interpreter::eval_less()
{
	perform_bool_op<LessOp>();
}

void Interpreter::eval_equal()
{
	perform_bool_op<EqualOp>();
}

void Interpreter::eval_equal_greater()
{
	perform_bool_op<EqualGreaterOp>();
}

void Interpreter::eval_equal_less()
{
	perform_bool_op<EqualLessOp>();
}

std::string Interpreter::print_value(ObjectPtr value) const
{
	int ival;
	if (value->get(&ival))
	{
		return std::format("value: {}", ival);
	}

	float fval;
	if (value->get(&fval))
	{
		return std::format("value: {}", fval);
	}

	std::string s;
	if(value->get(&s))
	{
		return std::format("value: {}", s);
	}
	bool bval;
	if(value->get(&bval))
	{
		return std::format("value: {}", bval);
	}
	return {};
}

size_t Interpreter::get_absolute_address(size_t index) const
{
	if(!_call_stack.empty())
	{
		return _call_stack.back().second + index;
	}
	return index;
}

void Interpreter::allocate_stack_variable(size_t index)
{
	index = get_absolute_address(index);
	if(index >= _stack.size())
	{
		_stack.resize(index + 1);
		
		_current_scope->add_variable();

		LOG_INFO("Allocate on stack {}", index);
	}
}

bool Interpreter::set_stack_variable(size_t index, ObjectPtr object)
{
	index = get_absolute_address(index);
	const bool res = _stack.size() > index;
	if (res)
	{
		_stack[index] = object;

		LOG_INFO("Var {} set to {}", index, print_value(object));
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
		const auto index = get_absolute_address(node->get_var_index());

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
