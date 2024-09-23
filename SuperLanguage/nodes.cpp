#include "nodes.hpp"

#include "interpreter.hpp"

void BinaryOperation::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

void NumberLiteral::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

Scope::Scope(std::vector<Node*>&& nodes)
	:_nodes(std::move(nodes))
{
}

Scope::~Scope()
{
	for (const auto node : _nodes)
	{
		delete node;
	}
}

void Scope::reset()
{
	_base_index = 0;
	_variable_count = 0;
}

void Scope::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

void Scope::add_variable()
{
	++_variable_count;
}

size_t Scope::get_variable_count() const
{
	return _variable_count;
}

size_t Scope::apply_index_offset(size_t index) const
{
	return _base_index + index;
}

void Scope::set_variable_count(size_t var_count)
{
	_variable_count = var_count;
}

std::vector<size_t> Scope::get_variables() const
{
	std::vector<size_t> res;
	auto from = _base_index;
	const auto end = _base_index + _variable_count;
	while(from < end)
	{
		res.push_back(from++);
	}
	return res;
}

void Scope::set_stack_base(size_t base)
{
	_base_index = base;
}

void Assign::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

void Variable::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

void Call::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

void Literal::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

Function::Function(Scope* scope, std::string&& name, int params)
	:_scope(scope)
	,_name(std::move(name))
	,_param_count(params)
{
	
}

void Function::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

void Function::run(Interpreter* interp, size_t stack_base)
{
	if(_scope)
	{
		_scope->set_stack_base(stack_base);
		NodeVisitor* visitor = interp;
		visitor->visit(_scope);
	}
}

InternalFunction::InternalFunction(std::string&& name, Func f)
	:Function(new Scope({}), std::move(name), 0)
	,_func(std::move(f))
{
}

void InternalFunction::run(Interpreter* interp, size_t stack_base)
{
	if(const auto scope = get_scope())
	{
		scope->set_stack_base(stack_base);
	}
	if(_func)
	{
		_func(interp, get_scope());
	}
}

void InternalFunction::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

Return::Return(Node* expression)
	:_expression(expression)
{
}

void Return::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

Node* Return::get_expression() const
{
	return _expression;
}

BranchIfElse::BranchIfElse(Node* expression, Scope* scope, Scope* else_scope)
	:_expression(expression)
	,_scope(scope)
	,_else_scope(else_scope)
{}

void BranchIfElse::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

Node* BranchIfElse::get_expression() const
{
	return _expression;
}

void BranchIfElse::execute(NodeVisitor& visitor, bool main_branch)
{
	if(main_branch)
	{
		visitor.visit(_scope);
	}
	else if(_else_scope)
	{
		visitor.visit(_else_scope);
	}
}

Loop::Loop(Node* expr, Scope* scope)
	:_expression(expr)
	, _scope(scope)
{}

void Loop::accept(NodeVisitor& visitor)
{
	visitor.visit(this);
}

Node* Loop::get_expression() const
{
	return _expression;
}