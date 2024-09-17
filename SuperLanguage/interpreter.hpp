#pragma once

#include "nodes.hpp"
#include <format>

#include "log.hpp"
#include "number.hpp"


class Interpreter final : public NodeVisitor
{
public:
	Interpreter(Node* scope);
	Interpreter(const Interpreter&) = delete;
	Interpreter(Interpreter&&) = delete;
	~Interpreter() override;

	void run();

	ObjectPtr get_stack_variable(size_t index) const;

	size_t get_stack_size() const;

	void add_internal_function(InternalFunction* func);

	void run_once(Node* node);

	const std::vector<std::pair<std::string, size_t>>& get_call_stack() const
	{
		return _call_stack;
	}

private:

	void visit(Scope* node) override;

	void visit(BinaryOperation* node) override;

	void visit(NumberLiteral* node) override;

	void visit(Variable* node) override;

	void visit(Assign* node) override;

	void visit(StringLiteral* node) override;

	void visit(Function* node) override;

	void visit(InternalFunction* node) override;

	void visit(Call* node) override;

	void visit(Return* node) override;

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
				const auto res = left_num->do_op<Op>(*right_num);

				_stack.emplace_back(res.as_object());

				return true;
			}
		}

		return false;
	}

	std::string print_value(ObjectPtr value) const;

	size_t get_absolute_address(size_t index) const;

	void allocate_stack_variable(size_t index);

	bool set_stack_variable(size_t index, ObjectPtr object);

	Function* get_function(Call* node);
private:
	Node* _root_scope;
	Scope* _current_scope;
	std::map<std::string, Function*> _functions;
	std::vector<ObjectPtr> _stack;
	ObjectPtr _return_value;
	std::vector<std::pair<std::string, size_t>> _call_stack;
};
