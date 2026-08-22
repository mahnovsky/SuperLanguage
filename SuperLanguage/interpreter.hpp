#pragma once

#include "nodes.hpp"
#include <format>

#include "log.hpp"
#include "number.hpp"
#include "new_ast.h"
#include "node_data.h"


class Interpreter final
{
public:
	Interpreter(new_ast::Node scope);
	Interpreter(const Interpreter&) = delete;
	Interpreter(Interpreter&&) = delete;
	~Interpreter();

	void run();

	ObjectPtr get_stack_variable(size_t index) const;

	void put_on_stack(ObjectPtr obj);

	size_t get_stack_size() const;

	void add_internal_function(const std::string& name, data::Function::InternalFunc func);

	void run_once(new_ast::Node node);

	const std::vector<std::pair<std::string, size_t>>& get_call_stack() const
	{
		return _call_stack;
	}

	void set_return_value(ObjectPtr return_value)
	{
		_return_value = return_value;
	}

private:
	void execute(new_ast::Node node);

	void visit(data::Scope* node);

	void visit(data::BinaryOperation* node);

	void visit(data::Variable* node);

	void visit(data::Assign* node);

	void visit(data::StackValue* node);

	void visit(data::Array* node);

	void visit(data::Function* node);

	void visit(data::Call* node);

	void visit(data::Return* node);

	void visit(data::BranchIfElse* node);

	void visit(data::Loop* node);
	
	void eval_plus();

	void eval_minus();

	void eval_mul();

	void eval_div();

	void eval_greater();

	void eval_less();

	void eval_equal();

	void eval_equal_greater();

	void eval_equal_less();

	template <class T>
	bool pop_stack(T& val)
	{
		const auto number = _stack.back();
		T v;
		if(number->get(&v))
		{
			val = v;
			_stack.pop_back();
			return true;
		}

		return false;
	}

	std::optional<Number> pop_stack_number()
	{
		if (!_stack.empty())
		{
			const auto res = Number::get_from_object(_stack.back());
			if (res.has_value())
			{
				_stack.pop_back();
			}
			return res;
		}
		return {};
	}

	template <class Op>
	bool try_perform_op()
	{
		if (const auto right_num = pop_stack_number())
		{
			if (const auto left_num = pop_stack_number())
			{
				const auto res = left_num->perform_op<Op>(*right_num);

				_stack.emplace_back(res.as_object());

				return true;
			}
		}

		return false;
	}

	template <class Op>
	bool perform_bool_op()
	{
		if (const auto right_num = pop_stack_number())
		{
			if (const auto left_num = pop_stack_number())
			{
				const auto res = left_num->perform_bool_op<Op>(*right_num);

				_stack.emplace_back(std::make_shared<Bool>(res));

				return true;
			}
		}

		return false;
	}

	std::string print_value(ObjectPtr value) const;

	size_t get_absolute_address(size_t index) const;

	void allocate_stack_variable(size_t index);

	bool set_stack_variable(size_t index, ObjectPtr object);

	data::Function* get_function(data::Call* node);

	void prepare_function_args(data::Scope* scope, const std::vector<new_ast::Node>& args);

	template <typename T>
	void run_visit(new_ast::Node node)
	{
		T* d = data::storage.get_data_mut<T>(node);
		visit(d);
	}
private:
	new_ast::Node _root_scope;
	data::Scope* _current_scope;
	std::map<std::string, data::Function*> _functions;
	std::vector<ObjectPtr> _stack;
	ObjectPtr _return_value;
	std::vector<std::pair<std::string, size_t>> _call_stack;
	using Func = void (Interpreter::*)(new_ast::Node);
	Func _table[new_ast::Node::get_nodes_count()] = {nullptr};
};
