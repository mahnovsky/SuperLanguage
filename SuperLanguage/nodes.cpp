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
	:_parent(nullptr)
	,_nodes(std::move(nodes))
{
	link_scopes();
}

Scope::~Scope()
{
	for (const auto node : _nodes)
	{
		delete node;
	}
}

void Scope::link_scopes()
{
	for(auto node : _nodes)
	{
		if(auto child = dynamic_cast<Scope*>(node))
		{
			child->set_parent(this);
		}
	}
}

void Scope::set_parent(Scope* parent)
{
	_parent = parent;
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

void StringLiteral::accept(NodeVisitor& visitor)
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

void Function::run(NodeVisitor* interp, size_t stack_base)
{
	if(_scope)
	{
		_scope->set_stack_base(stack_base);
		interp->visit(_scope);
	}
}
