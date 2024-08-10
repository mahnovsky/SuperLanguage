#pragma once

#include "nodes.hpp"
#include <format>

class Interpreter :public NodeVisitor
{
public:
	Interpreter(Node* scope);

	~Interpreter() override;

	void run();

private:

	void visit(Scope* node) override;

	void visit(BinaryOperation* node) override;

	void visit(NumberLiteral* node) override;

	void visit(Variable* node) override;

	void visit(Assign* node) override;

	void visit(StringLiteral* node) override;

	void visit(Function* node) override;

	void visit(Call* node) override;

	void eval_plus();

	void eval_minus();

	void eval_mul();

	void eval_div();

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

	std::string print_stack_value() const;

private:
	Node* _root_scope;
	Scope* _current_scope;
	std::map<std::string, Function*> _functions;
	std::vector<ObjectPtr> _stack;
};