#pragma once

#include <functional>
#include <vector>
#include <string>
#include <map>
#include <iostream>

#include "lexer.hpp"

class NodeVisitor;
class Interpreter;

class Node
{
public:
	virtual ~Node() = default;

	virtual void accept(NodeVisitor& visitor) = 0;
};

enum class Operation
{
	Plus,
	Minus,
	Mul,
	Div,
	Greater,
	Less,
	Equal,
	EqualGreater,
	EqualLess
};


class BinaryOperation : public Node
{
public:
	BinaryOperation(Node* left, Node* right, Operation op)
		:_left(left)
		, _right(right)
		, _operation(op)
	{}

	void accept(NodeVisitor& visitor) override;

	Node* get_left() const { return _left; }

	Node* get_right() const { return _right; }

	Operation get_operation() const { return _operation; }

private:
	Node* _left = nullptr;
	Node* _right = nullptr;
	Operation _operation;
};

class Scope : public Node
{
public:
	Scope(std::vector<Node*>&& nodes);

	~Scope() override;

	void reset();

	void accept(NodeVisitor& visitor) override;

	const std::vector<Node*>& get_nodes() const { return _nodes; }

	size_t get_stack_base() const { return _base_index; }

	void add_variable();

	size_t get_variable_count() const;

	size_t apply_index_offset(size_t index) const;

	void set_variable_count(size_t var_count);

	std::vector<size_t> get_variables() const;

	void set_stack_base(size_t base);

private:
	size_t _base_index = 0;
	size_t _variable_count = 0;
	std::vector<Node*> _nodes;
};

class Variable : public Node
{
public:
	Variable(std::string&& name, size_t offset)
		:_name(name)
		,_index(offset)
	{}

	void accept(NodeVisitor& visitor) override;

	std::string get_name() const { return _name; }

	size_t get_stack_index() const
	{
		return _index;
	}

private:
	std::string _name;
	size_t _index;
};

class Assign : public Node
{
public:
	Assign(size_t var_index, Node* expr, bool declaration = false)
		:_var_index(var_index)
		,_expression(expr)
		,_declaration(declaration)
	{}

	void accept(NodeVisitor& visitor) override;

	size_t get_var_index() const { return _var_index; }

	Node* get_expression() const { return _expression; }

	bool is_declaration() const { return _declaration; }
private:
	size_t _var_index = 0;
	Node* _expression;
	bool _declaration;
};

class StackValue : public Node
{
public:
	StackValue(ObjectPtr val)
		:_value(std::move(val))
	{}

	void accept(NodeVisitor& visitor) override;

	ObjectPtr get_object()
	{
		return _value;
	}

private:
	ObjectPtr _value;
};

class Function : public Node
{
public:
	Function(Scope* scope, std::string&& name, int params);

	void accept(NodeVisitor& visitor) override;

	const std::string& get_name() const{
		return _name;
	}

	int get_params_count() const
	{
		return _param_count;
	}

	Scope* get_scope() const { return _scope; }

	virtual void run(Interpreter* interp, size_t stack_base);

private:
	Scope* _scope;
	std::string _name;
	int _param_count;
};

class InternalFunction : public Function
{
public:
	using Func = std::function<void(Interpreter*, Scope*)>;

	InternalFunction(std::string&& name, Func f);

	void run(Interpreter* interp, size_t stack_base) override;

	void accept(NodeVisitor& visitor) override;
private:
	Func _func;
};

class Call : public Node
{
public:
	Call(std::vector<Node*>&& args, std::string&& func_name)
		:_args(std::move(args))
		,_function_name(std::move(func_name))
	{}

	void accept(NodeVisitor& visitor) override;

	std::string_view get_function_name() const
	{
		return _function_name;
	}

	size_t get_var_index() const
	{
		return _var_index;
	}

	const std::vector<Node*>& get_args() const
	{
		return _args;
	}

private:
	std::vector<Node*> _args;
	std::string _function_name;
	size_t _var_index = 0;
};


class Return : public Node
{
public:
	Return(Node* expression);

	void accept(NodeVisitor& visitor) override;

	Node* get_expression() const;

private:
	Node* _expression;
};

class BranchIfElse : public Node
{
public:
	BranchIfElse(Node* expression, Scope* scope, Scope* else_scope);

	void accept(NodeVisitor& visitor) override;

	Node* get_expression() const;

	void execute(NodeVisitor& visitor, bool main_branch);

private:
	Node* _expression;
	Scope* _scope;
	Scope* _else_scope;
};

class Loop : public Node
{
public:
	Loop(Node* expr, Scope* scope);

	void accept(NodeVisitor& visitor) override;

	Node* get_expression() const;

	Scope* get_scope() const { return _scope; }

private:
	Node* _expression;
	Scope* _scope;
};

class ArrayNode : public Node
{
public:
	ArrayNode(std::vector<Node*> array_nodes);

	void accept(NodeVisitor& visitor) override;

	const std::vector<Node*>& get_array_nodes() const { return _array_nodes; }

private:
	std::vector<Node*> _array_nodes;
};

class NodeVisitor
{
public:
	virtual ~NodeVisitor() = default;

	virtual void visit(Scope* node) = 0;
	virtual void visit(BinaryOperation* node) = 0;
	virtual void visit(Assign* node) = 0;
	virtual void visit(Variable* node) = 0;
	virtual void visit(StackValue* node) = 0;
	virtual void visit(ArrayNode* node) = 0;
	virtual void visit(Function* node) = 0;
	virtual void visit(InternalFunction* node) = 0;
	virtual void visit(Call* node) = 0;
	virtual void visit(Return* node) = 0;
	virtual void visit(BranchIfElse* node) = 0;
	virtual void visit(Loop* node) = 0;
 };




