#pragma once

#include <string>

#include "object.hpp"
#include "new_ast.h"

class Interpreter;

namespace data
{
	using NodeType = new_ast::Node::Index;
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

	

	struct BinaryOperation
	{
		static inline constexpr NodeType _node_type = NodeType::BinaryOp;
		new_ast::Node _left;
		new_ast::Node _right;
		Operation _operation;

		BinaryOperation(new_ast::Node left, new_ast::Node right, Operation op)
			:_left(left)
			,_right(right)
			,_operation(op)
		{}
	};

	struct Scope 
	{
		static inline constexpr NodeType _node_type = NodeType::Scope;
		size_t _base_index = 0;
		size_t _variable_count = 0;
		std::vector<new_ast::Node> _nodes;

		Scope(std::vector<new_ast::Node> nodes)
			:_nodes(std::move(nodes))
		{}

		std::vector<size_t> get_variables() const
		{
			std::vector<size_t> res;
			auto from = _base_index;
			const auto end = _base_index + _variable_count;
			while (from < end)
			{
				res.push_back(from++);
			}
			return res;
		}
	};

	struct Variable
	{
		static inline constexpr NodeType _node_type = NodeType::Variable;
		std::string _name;
		size_t _index = 0;

		Variable(std::string name, size_t index)
			:_name(name)
			,_index(index)
		{}
	};

	struct Assign
	{
		static inline constexpr NodeType _node_type = NodeType::Assign;
		size_t _var_index = 0;
		new_ast::Node _expression;
		bool _declaration;

		Assign(size_t var_index, new_ast::Node exp, bool is_decl)
			:_var_index(var_index)
			,_expression(exp)
			,_declaration(is_decl)
		{}
	};

	struct StackValue
	{
		static inline constexpr NodeType _node_type = NodeType::StackValue;
		ObjectPtr _value;

		StackValue(ObjectPtr obj)
			:_value(obj)
		{}
	};

	struct Function
	{
		static inline constexpr NodeType _node_type = NodeType::Function;
		Scope* _scope;
		std::string _name;
		int _param_count;

		Function(Scope* s, std::string name, int params)
			:_scope(s)
			,_name(name)
			,_param_count(params)
		{}
	};

	struct InternalFunction
	{
		static inline constexpr NodeType _node_type = NodeType::InternalFunction;
		using Func = std::function<void(Interpreter*, Scope*)>;
		std::string _name;
		int _param_count;
		Func _func;

		InternalFunction(const std::string& name, Func fn)
			:_name(name)
			,_func(fn)
		{}
	};

	struct Call
	{
		static inline constexpr NodeType _node_type = NodeType::Call;
		std::vector<new_ast::Node> _args;
		std::string _function_name;
		size_t _var_index = 0;
	};


	struct Return
	{
		static inline constexpr NodeType _node_type = NodeType::Return;
		new_ast::Node _expression;
	};

	struct BranchIfElse
	{
		static inline constexpr NodeType _node_type = NodeType::BranchIfElse;
		new_ast::Node _expression;
		Scope* _scope;
		Scope* _else_scope;

		BranchIfElse(new_ast::Node expr, Scope* s, Scope* els)
			:_expression(expr)
			,_scope(s)
			,_else_scope(els)
		{}
	};

	struct Loop
	{
		static inline constexpr NodeType _node_type = NodeType::Loop;
		new_ast::Node _expression;
		Scope* _scope;

		Loop(new_ast::Node expr, Scope* s)
			:_expression(expr)
			,_scope(s)
		{}
	};

	struct Array
	{
		static inline constexpr NodeType _node_type = NodeType::Array;
		std::vector<new_ast::Node> _array_nodes;

		Array(std::vector<new_ast::Node> nodes)
			:_array_nodes(std::move(nodes))
		{}
	};

	using Storage = new_ast::NodeDataStorage<
		BinaryOperation,
		Scope,
		Variable,
		Assign,
		StackValue,
		Function,
		InternalFunction,
		Call,
		Return,
		BranchIfElse,
		Loop,
		Array
	>;

	static inline data::Storage storage;
}
