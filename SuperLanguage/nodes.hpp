#pragma once

#include <vector>
#include <string>
#include <map>
#include <iostream>

#include "lexer.hpp"

class NodeVisitor;

class Node
{
public:
	virtual ~Node() = default;

	virtual void accept(NodeVisitor& visitor) = 0;
};


class BinaryOperation : public Node
{
public:
	BinaryOperation(Node* left, Node* right, char op)
		:_left(left)
		, _right(right)
		, _operation(op)
	{}

	void accept(NodeVisitor& visitor) override;

	Node* get_left() const { return _left; }

	Node* get_right() const { return _right; }

	char get_operation() const { return _operation; }

private:
	Node* _left = nullptr;
	Node* _right = nullptr;
	char _operation = 0;
};

class Scope : public Node
{
public:
	Scope(std::vector<Node*>&& nodes);

	~Scope() override;

	void link_scopes();

	void set_parent(Scope* parent);

	void accept(NodeVisitor& visitor) override;

	const std::vector<Node*>& get_nodes() const { return _nodes; }

	ObjectPtr get_variable(const std::string& name, Scope** scope = nullptr);

	void set_variable(const std::string& name, ObjectPtr var, bool declaration = false);

private:
	Scope* _parent;
	std::vector<Node*> _nodes;
	std::map<std::string, ObjectPtr> _variables;
};

class NumberLiteral : public Node
{
public:
	NumberLiteral(ObjectPtr n)
		:_number(std::move(n))
	{}

	void accept(NodeVisitor& visitor) override;

	ObjectPtr get_number() const { return _number; }

private:
	ObjectPtr _number;
};

class Variable : public Node
{
public:
	Variable(std::string&& name)
		:_name(name)
	{}

	void accept(NodeVisitor& visitor) override;

	std::string get_name() const { return _name; }

private:
	std::string _name;
};

class Assign : public Node
{
public:
	Assign(std::string&& var, Node* expr, bool declaration = false)
		:_variable(std::move(var))
		,_expression(expr)
		,_declaration(declaration)
	{}

	void accept(NodeVisitor& visitor) override;

	std::string get_var_name() const { return _variable; }

	Node* get_expression() const { return _expression; }

	bool is_declaration() const { return _declaration; }
private:
	std::string _variable;
	Node* _expression;
	bool _declaration;
};

class StringLiteral : public Node
{
public:
	StringLiteral(ObjectPtr val)
		:_value(std::move(val))
	{}

	void accept(NodeVisitor& visitor) override;

	ObjectPtr get_string()
	{
		return _value;
	}

private:
	ObjectPtr _value;
};

class Function : public Node
{
public:
	void accept(NodeVisitor& visitor) override;

	const std::string& get_name() const
	{
		return _name;
	}

private:
	std::string _name;
};

class Call : public Node
{
public:
	Call(std::string&& name)
		:_name(std::move(name))
	{}

	void accept(NodeVisitor& visitor) override;

	const std::string& get_name() const
	{
		return _name;
	}

private:
	std::string _name;
};

class NodeVisitor
{
public:
	virtual ~NodeVisitor() = default;

	virtual void visit(Scope* node) = 0;
	virtual void visit(BinaryOperation* node) = 0;
	virtual void visit(NumberLiteral* node) = 0;
	virtual void visit(Assign* node) = 0;
	virtual void visit(Variable* node) = 0;
	virtual void visit(StringLiteral* node) = 0;
	virtual void visit(Function* node) = 0;
	virtual void visit(Call* node) = 0;
};




