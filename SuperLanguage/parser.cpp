#include "parser.hpp"

#include <cassert>
#include <format>

using namespace data;

Parser::Parser(std::vector<Token>&& tokens)
	:_tokens{std::move(tokens)}
	,_current(_tokens.begin())
{
}

new_ast::Node Parser::add_tokens(const std::vector<Token>& tokens)
{
	const auto size = _tokens.size();
	_tokens.insert(_tokens.end(), tokens.begin(), tokens.end());
	_current = _tokens.begin() + size;

	return statement();
}

new_ast::Node Parser::parse()
{
	auto nodes = statement_list();

	
	//return { new_ast::Node::Index::Scope, storage.create_node<Scope>(std::move(nodes)) };
	return storage.create_node<Scope>(std::move(nodes));
}

void Parser::eat(TokType tok_type)
{
	if (_current != _tokens.end() && _current->type == tok_type)
	{
		++_current;
	}
	else
	{
		puts("Parse error: expected ");
		assert(false);
	}
}

std::vector<new_ast::Node> Parser::statement_list()
{
	std::vector<new_ast::Node> nodes;

	while(_current != _tokens.end())
	{
		if(_current->type == TT_ScopeEnd)
		{
			_skip_semicolon = false;
			break;
		}

		nodes.push_back(statement());
		if (!_skip_semicolon)
		{
			eat(TT_Semicolon);
		}
		else
		{
			_skip_semicolon = false;
		}
	}

	return nodes;
}

new_ast::Node Parser::statement()
{
	if(_current == _tokens.end())
	{
		return {};
	}

	if(_current->type == TT_Let)
	{
		auto var_node = create_variable();
		eat(TT_Assign);

		new_ast::Node obj = _current->type == TT_Fn ?
			statement() : expression();

		auto* var = storage.get_data<Variable>(var_node);
		//_variables.emplace(var->_name, _current_context);

		return storage.create_node<Assign>(var->_index, obj, true);
	}

	if(_current->type == TT_Id)
	{
		const auto node = resolve_id();

		if(node.index == NodeType::Call)
		{
			return node;
		}

		const auto var = storage.get_data<Variable>(node);
		
		eat(TT_Assign);

		return storage.create_node<Assign>(var->_index, expression(), false);
	
		//new Assign(var->get_stack_index(), expression());
	}

	if(_current->type == TT_ScopeBegin)
	{
		const auto base_index = _index_counter;
		++_scope_level;
		eat(TT_ScopeBegin);
		auto nodes = statement_list();
		eat(TT_ScopeEnd);
		--_scope_level;
		_skip_semicolon = true;
		_index_counter = base_index;

		return storage.create_node<Scope>(std::move(nodes));
		// new Scope(std::move(nodes));
	}

	if(_current->type == TT_Fn)
	{
		eat(TT_Fn);
		if(_current->type == TT_Id)
		{
			_current_func = _current->name;
			eat(TT_Id);
		}

		eat(TT_LParen);
		const auto prev_counter = _index_counter;
		_index_counter = 0;
		int param_index = 0;
		while (_current->type == TT_Id)
		{
			const std::string param_name = std::format("param_{}_{}", _current_func, _current->name);
			const auto var_node = storage.create_node<Variable>( param_name, static_cast<size_t>(param_index));
			//auto* var = new Variable(std::string{param_name}, param_index);
			_variables.emplace(param_name, VariableInfo{ TypeContext::None, var_node });
			++_index_counter;
			eat(TT_Id);
			if (_current->type != TT_RParen)
			{
				eat(TT_Coma);
			}

			++param_index;
		}
		eat(TT_RParen);
		
		const auto scope = statement();
		auto s = storage.get_data_mut<Scope>(scope);
		_index_counter = prev_counter;
		return storage.create_node<Function>(s, std::move(_current_func), param_index );//new Function(scope, std::move(_current_func), param_index);
	}

	if(_current->type == TT_Ret)
	{
		eat(TT_Ret);

		//new Return(expression());
		return storage.create_node<Return>( expression() );
	}

	if(_current->type == TT_If)
	{
		eat(TT_If);
		eat(TT_LParen);
		new_ast::Node expr = expression();
		eat(TT_RParen);
		auto s = statement();
		Scope* scope = storage.get_data_mut<Scope>(s);
		Scope* else_branch = nullptr;
		if(_current != _tokens.end() && _current->type == TT_Else)
		{
			eat(TT_Else);
			_skip_semicolon = false;
			s = statement();
			else_branch = storage.get_data_mut<Scope>(s);
		}
		return  storage.create_node<BranchIfElse>( expression(), scope, else_branch );
		//return new BranchIfElse(expr, scope, else_branch);
	}

	if(_current->type == TT_Loop)
	{
		eat(TT_Loop);
		eat(TT_LParen);
		new_ast::Node expr = expression();
		eat(TT_RParen);
		//const auto scope = dynamic_cast<Scope*>(statement());
		auto s = statement();
		Scope* scope = storage.get_data_mut<Scope>(s);
		return storage.create_node<Loop>(expr, scope);
		//return new Loop(expr, scope);
	}

	return {};
}

new_ast::Node Parser::expression()
{
	_current_context = get_expression_context();
	
	if(_current_context == TypeContext::String)
	{
		return string_expression();
	}
	if(_current_context == TypeContext::Number)
	{
		return number_expression();
	}
	if(_current_context == TypeContext::Bool)
	{
		return bool_expression();
	}
	if(_current_context == TypeContext::Array)
	{
		return array_expression();
	}
	if(_current->type == TT_Id)
	{
		return resolve_id();
	}
	assert(false);

	return {};
}

new_ast::Node Parser::bool_expression()
{
	auto node = bool_term();

	while (_current->type == TT_And || _current->type == TT_Or)
	{
		const auto op = _current->type == TT_Plus ? Operation::Plus : Operation::Minus;
		eat(_current->type);
		node = storage.create_node<BinaryOperation>(node, term(), op );
		//node = new BinaryOperation(node, term(), op);
	}

	return node;
}

new_ast::Node Parser::number_expression()
{
	auto node = term();

	while (_current->type == TT_Plus || _current->type == TT_Minus)
	{
		const auto op = _current->type == TT_Plus ? Operation::Plus : Operation::Minus;
		eat(_current->type);
		//node = new BinaryOperation(node, term(), op);
		node = storage.create_node<BinaryOperation>( node, term(), op );
	}

	return node;
}

new_ast::Node Parser::string_expression()
{
	new_ast::Node node = string_factor();
	while (_current->type == TT_Plus)
	{
		eat(TT_Plus);
		//node = new BinaryOperation(node, string_factor(), Operation::Plus);
		node = storage.create_node<BinaryOperation>( node, string_factor(), Operation::Plus );
		//node = new BinaryOperation(node, term(), op);
	}

	return node;
}

new_ast::Node Parser::array_expression()
{
	if (_current->type == TT_ArrayBegin)
	{
		eat(TT_ArrayBegin);

		std::vector<new_ast::Node> nodes;
		new_ast::Node element;
		do
		{
			element = array_element();

			nodes.emplace_back(element);

			if (_current->type == TT_Coma)
			{
				eat(TT_Coma);
			}
		} while (_current->type != TT_ArrayEnd);

		eat(TT_ArrayEnd);

		//return new ArrayNode(std::move(nodes));
		return storage.create_node<Array>(std::move(nodes));
	}

	if(_current->type == TT_Id)
	{
		return resolve_id();
	}

	return {};
}

new_ast::Node Parser::array_element()
{
	if (_current->type == TT_Id)
	{
		return resolve_id();
	}
	if((_current->type & (TT_StringLiteral | TT_BoolLiteral | TT_NumberLiteral)) > 0)
	{
		const ObjectPtr f = _current->object;
		eat(_current->type);
		//return new StackValue(f);
		return storage.create_node<StackValue>( f );
	}

	return {};
}

new_ast::Node Parser::string_factor()
{
	if (_current->type == TT_Id)
	{
		return resolve_id();
	}
	if (_current->type == TT_StringLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TT_StringLiteral);
		//return new StackValue(f);
		return storage.create_node<StackValue>( f );
	}

	return {};
}

new_ast::Node Parser::factor()
{
	if(_current->type == TT_BoolLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TT_BoolLiteral);
		//return new StackValue(f);
		return storage.create_node<StackValue>( f );
	}
	if(_current->type == TT_LParen)
	{
		eat(TT_LParen);
		new_ast::Node expr = expression();
		eat(TT_RParen);
		return expr;
	}
	if(_current->type == TT_Id)
	{
		return resolve_id();
	}
	if(_current->type == TT_NumberLiteral)
	{
		ObjectPtr f = _current->object;
		eat(TT_NumberLiteral);
		//return new StackValue(f);
		return  storage.create_node<StackValue>( f );
	}
	
	return {};
}

new_ast::Node Parser::term()
{
	auto node = factor();

	while(_current->type == TT_Mul || _current->type == TT_Div)
	{
		const auto op = _current->type == TT_Mul ? Operation::Mul : Operation::Div;
		eat(_current->type);
		//node = new BinaryOperation(node, factor(), op);
		node = storage.create_node<BinaryOperation>(node, factor(), op);
	}

	return node;
}

new_ast::Node Parser::bool_term()
{
	auto node = factor();

	if (_current->type == TT_Equal || _current->type == TT_Greater || _current->type == TT_Less)
	{
		const auto prev_type = _current->type;
		eat(_current->type);

		Operation op = Operation::Equal;
		if(prev_type == TT_Greater)
		{
			op = Operation::Greater;
		}
		else if (prev_type == TT_Less)
		{
			op = Operation::Less;
		}

		if(_current->type == TT_Equal && prev_type == TT_Greater)
		{
			op = Operation::EqualGreater;
			eat(_current->type);
		}
		if (_current->type == TT_Equal && prev_type == TT_Less)
		{
			op = Operation::EqualLess;
			eat(_current->type);
		}

		//node = new BinaryOperation(node, factor(), op);
		node = storage.create_node<BinaryOperation>( node, factor(), op );
	}

	return node;
}

new_ast::Node Parser::create_variable()
{
	eat(TT_Let);

	std::string name = std::format("{}_{}_{}", _scope_level, _current->name, _current_func);
	eat(TT_Id);
	const size_t var_offset = _index_counter++;
	//auto* var = new Variable(std::move(name), var_offset);
	const auto var_node = storage.create_node<Variable>( name, var_offset );

	_variables.emplace(name, VariableInfo{ get_expression_context(), var_node });

	return var_node;
}

std::optional<new_ast::Node> Parser::get_variable()
{
	std::string name = _current->name;
	eat(TT_Id);

	auto find_var = [this](const std::string& name) -> std::optional<new_ast::Node>
	{
		if (const auto it = _variables.find(name); it != _variables.end())
		{
			return it->second.var_node;
		}
		return std::nullopt;
	};

	if(!_current_func.empty())
	{
		const auto param_name = std::format("param_{}_{}", _current_func, name);

		if(const auto var = find_var(param_name))
		{
			return var;
		}
	}

	auto scope = _scope_level;

	while (scope >= 0)
	{
		const auto var_name = std::format("{}_{}_{}", scope, name, _current_func);
		if (const auto var = find_var(var_name))
		{
			return var;
		}
		--scope;
	}

	return std::nullopt;
}

Parser::TypeContext Parser::get_variable_context(const std::string& name) const
{
	if(_variables.empty())
	{
		return TypeContext::None;
	}

	auto find_var = [this](const std::string& name)
	{
		if (const auto it = _variables.find(name); it != _variables.end())
		{
			return it->second.context;
		}
		return TypeContext::None;
	};

	if (!_current_func.empty())
	{
		const auto param_name = std::format("param_{}_{}", _current_func, name);

		if (const auto context = find_var(param_name); context != TypeContext::None)
		{
			return context;
		}
	}
	else
	{
		auto scope = _scope_level;

		while (scope >= 0)
		{
			auto scope_name = std::format("{}_{}_{}", scope, name, _current_func);

			if (const auto context = find_var(scope_name); context != TypeContext::None)
			{
				return context;
			}
			--scope;
		}
	}
	return TypeContext::None;
}

new_ast::Node Parser::resolve_id()
{
	std::string name = _current->name;
	auto var_node = get_variable();

	if (!var_node && _current->type == TT_LParen)
	{
		eat(TT_LParen);
		std::vector<new_ast::Node> args;
		while (_current->type != TT_RParen)
		{
			args.push_back(expression());
			if (_current->type != TT_RParen)
			{
				eat(TT_Coma);
			}
		}
		eat(TT_RParen);
		//return new Call(std::move(args), std::move(name));

		return storage.create_node<Call>(std::move(args), std::move(name));
	}

	return var_node.value();
}

Parser::TypeContext Parser::get_expression_context() const
{
	auto it = _current;
	TypeContext context = TypeContext::None;
	while (it != _tokens.end() && it->type != TT_Semicolon && it->type != TT_Coma)
	{
		if(it->type == TT_ScopeBegin)
		{
			return context;
		}

		if(it->type == TT_Equal || it->type == TT_Greater || it->type == TT_Less)
		{
			return TypeContext::Bool;
		}

		if(it->type == TT_Mul || it->type == TT_Div || it->type == TT_Minus)
		{
			return TypeContext::Number;
		}

		if(it->type == TT_ArrayBegin || it->type == TT_ArrayEnd)
		{
			return TypeContext::Array;
		}

		if(it->type == TT_Plus)
		{
			if(context == TypeContext::Number)
			{
				return TypeContext::Number;
			}

			if(context == TypeContext::String)
			{
				return TypeContext::String;
			}
		}

		if(it->type == TT_NumberLiteral)
		{
			context = TypeContext::Number;
		}

		if (it->type == TT_StringLiteral)
		{
			context = TypeContext::String;
		}

		if(it->type == TT_BoolLiteral)
		{
			context = TypeContext::Bool;
		}

		if (it->type == TT_Id)
		{
			const auto res = get_variable_context(it->name);
			if (res != TypeContext::None)
			{
				context = res;
			}
		}
		++it;
	}
	return context;
}
