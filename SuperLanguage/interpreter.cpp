#include "interpreter.hpp"
#include "log.hpp"
#include <cassert>

using namespace data;
using namespace new_ast;

Interpreter::Interpreter(new_ast::Node scope)
	:_root_scope(scope)
	, _current_scope()
{
	
	/*Id = 0,
			Assign,
			BinaryOp,
			Scope,
			Variable,
			StackValue,
			Function,
			Call,
			Return,
			BranchIfElse,
			Loop,
			Array,*/
	_table[Node::index_as_int(NodeType::Assign)] = &Interpreter::run_visit<Assign>;
	_table[Node::index_as_int(NodeType::BinaryOp)] = &Interpreter::run_visit<BinaryOperation>;
	_table[Node::index_as_int(NodeType::Scope)] = &Interpreter::run_visit<Scope>;
	_table[Node::index_as_int(NodeType::Variable)] = &Interpreter::run_visit<Variable>;
	_table[Node::index_as_int(NodeType::StackValue)] = &Interpreter::run_visit<StackValue>;
	_table[Node::index_as_int(NodeType::Function)] = &Interpreter::run_visit<Function>;
	_table[Node::index_as_int(NodeType::Call)] = &Interpreter::run_visit<Call>;
	_table[Node::index_as_int(NodeType::Return)] = &Interpreter::run_visit<Return>;
	_table[Node::index_as_int(NodeType::BranchIfElse)] = &Interpreter::run_visit<BranchIfElse>;
	_table[Node::index_as_int(NodeType::Loop)] = &Interpreter::run_visit<Loop>;
	_table[Node::index_as_int(NodeType::Array)] = &Interpreter::run_visit<Array>;
}

Interpreter::~Interpreter()
{
	//delete _root_scope;
}

void Interpreter::run()
{
	const auto* scope = storage.get_data<Scope>(_root_scope);

	for(const auto& node : scope->_nodes)
	{
		const auto method = _table[Node::index_as_int(node.index)];

		(this->*method)(node);
	}
}

ObjectPtr Interpreter::get_stack_variable(size_t index) const
{
	if (index < _stack.size())
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

void Interpreter::add_internal_function(const std::string& name, data::Function::InternalFunc func)
{
	const auto scope_node = storage.create_node<Scope>(std::vector<Node>{});

	auto func_node = storage.create_node<Function>(name, std::move(func), storage.get_data_mut<Scope>(scope_node));

	_functions[name] = storage.get_data_mut<Function>(func_node);
}

void Interpreter::run_once(new_ast::Node node)
{
	//node->accept(*this);
}

void Interpreter::execute(new_ast::Node node)
{
	if (const auto method = _table[Node::index_as_int(node.index)]) 
	{
		(this->*method)(node);
	}
}

void Interpreter::visit(Scope* scope)
{
	const auto parent_scope = _current_scope;
	_current_scope = scope;
	if (parent_scope)
	{
		_current_scope->_base_index = parent_scope->_base_index;
		//_current_scope->set_stack_base(parent_scope->get_stack_base());
	}

	for (Node node : scope->_nodes)
	{
		//child->accept(*this);
		execute(node);
	}

	if (_current_scope)
	{
		_stack.resize(_stack.size() - _current_scope->_variable_count);
	}
	_current_scope = parent_scope;
}

void Interpreter::visit(BinaryOperation* bin_op)
{
	//node->get_left()->accept(*this);
	//node->get_right()->accept(*this);
	execute(bin_op->_left);
	execute(bin_op->_right);

	switch (bin_op->_operation)
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

void Interpreter::visit(Variable* var)
{
	const auto index = get_absolute_address(var->_index);

	if (index < _stack.size())
	{
		_stack.emplace_back(_stack[index]);
	}
	else
	{
		assert(false);
	}
}

void Interpreter::visit(Assign* assign)
{
	if (assign->_expression.index == NodeType::Scope)
	{
		Scope* scope = storage.get_data_mut<Scope>(assign->_expression);
		_stack.push_back(std::make_shared<Callable>(scope));
	}
	else
	{
		//node->get_expression()->accept(*this);
		execute(assign->_expression);
	}

	const auto val = _stack.back();
	_stack.pop_back();

	if (assign->_declaration)
	{
		allocate_stack_variable(assign->_var_index);
		set_stack_variable(assign->_var_index, val);
	}
	else if (!set_stack_variable(assign->_var_index, val))
	{
		LOG_INFO("Failed to assign, variable \'{}\' not exist in current scope", node->get_var_index());
	}
}

void Interpreter::visit(StackValue* sval)
{
	_stack.emplace_back(sval->_value);
}

void Interpreter::visit(Array* arr)
{
	const auto& array_nodes = arr->_array_nodes;

	std::vector<ObjectPtr> array_objects;
	for (const auto& node : array_nodes)
	{
		const auto stack_size = _stack.size();

		execute(node);

		if (stack_size < _stack.size())
		{
			array_objects.emplace_back(_stack.back());
			_stack.pop_back();
		}
	}

	_stack.emplace_back(std::make_shared<ArrayObj>(array_objects));
}

void Interpreter::visit(Function* func)
{
	const auto& name = func->_name;
	if (_functions.find(name) == _functions.end())
	{
		_functions[name] = func;
	}
}

void Interpreter::visit(Call* call)
{
	LOG_INFO("Call function {}", func->get_name());

	if (const auto func = get_function(call))
	{
		const auto base_index = _stack.size();
		func->_scope->reset();

		prepare_function_args(func->_scope, call->_args);

		_call_stack.emplace_back(func->_name, base_index);
		//func->run(this, base_index);
		func->_scope->set_stack_base(base_index);
		if(func->_internal_fn)
		{
			func->_internal_fn(this, func->_scope);
		}
		else {
			visit(func->_scope);
		}

		if (_return_value)
		{
			constexpr auto ret_index = 0;
			allocate_stack_variable(ret_index);
			set_stack_variable(ret_index, std::move(_return_value));
		}
		_call_stack.pop_back();

		LOG_INFO("Function call end {}", func->get_name());
	}
}

void Interpreter::visit(Return* ret)
{
	const auto expr = ret->_expression;
	const auto prev_size = _stack.size();
	//expr->accept(*this);
	execute(expr);
	if (_stack.size() > prev_size)
	{
		_return_value = _stack.back();
		_stack.pop_back();
	}
}

void Interpreter::visit(BranchIfElse* branch)
{
	//node->get_expression()->accept(*this);
	execute(branch->_expression);
	bool value;
	if (pop_stack(value))
	{
		//node->execute(*this, value);
		Scope* s = value ? branch->_scope : branch->_else_scope;
		if (s) {
			visit(s);
		}
	}
	else
	{
		LOG_ERROR("Failed to execute if statement, bool value expected");
	}
}

void Interpreter::visit(Loop* loop)
{
	const auto expr = loop->_expression;
	const auto scope = loop->_scope;
	if (!scope)
	{
		LOG_ERROR("Failed to execute loop, no scope to execute");
		return;
	}

	bool value = false;
	do
	{
		execute(expr);
		//expr->accept(*this);
		if (pop_stack(value))
		{
			if (!value)
			{
				break;
			}
			//scope->accept(*this);
			visit(scope);
		}
		else
		{
			LOG_ERROR("Failed to execute loop statement, bool value expected");
			break;
		}
	} while (value);
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
	if (!try_perform_op<MinusOp>())
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
		LOG_ERROR("Failed to perform div operation");
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
	return std::format("value: {}", value->to_string());
}

size_t Interpreter::get_absolute_address(size_t index) const
{
	if (!_call_stack.empty())
	{
		return _call_stack.back().second + index;
	}
	return index;
}

void Interpreter::allocate_stack_variable(size_t index)
{
	index = get_absolute_address(index);
	if (index >= _stack.size())
	{
		_stack.resize(index + 1);

		//_current_scope->add_variable();

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

data::Function* Interpreter::get_function(data::Call* node)
{
	const std::string name{ node->_function_name };
	if (const auto it = _functions.find(name); it != _functions.end())
	{
		return it->second;
	}

	const auto index = get_absolute_address(node->_var_index);

	if (_stack.size() < index)
	{
		data::Function* func = nullptr;
		_stack[index]->get(&func);
		return func;
	}
	return nullptr;
}

void Interpreter::prepare_function_args(Scope* scope, const std::vector<new_ast::Node>& args)
{
	LOG_INFO("Function args begin");
	for (const auto arg : args)
	{
		const auto prev_size = _stack.size();
		//arg->accept(*this);
		execute(arg);
		scope->add_variable();
		const auto str_val = print_value(_stack.back());
		LOG_INFO("Arg {} set value to {}", prev_size, str_val);
	}
	LOG_INFO("Function args end");
}
