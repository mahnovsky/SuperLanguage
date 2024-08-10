#include "nodes.hpp"

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

ObjectPtr Scope::get_variable(const std::string& name, Scope** scope)
{
	const auto it = _variables.find(name);
	if (it != _variables.end())
	{
		if(scope)
		{
			(*scope) = this;
		}
		
		return it->second;
	}
	if(_parent)
	{
		return _parent->get_variable(name, scope);
	}

	return {};
}

void Scope::set_variable(const std::string& name, ObjectPtr var, bool declaration)
{
	if(declaration)
	{
		_variables[name] = std::move(var);
		return;
	}

	auto it = _variables.find(name);
	if(it != _variables.end())
	{
		_variables[name] = std::move(var);
	}
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
