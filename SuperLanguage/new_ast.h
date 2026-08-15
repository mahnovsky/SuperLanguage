#pragma once

#include "arena.h"

#include <cstdint>
#include <functional>


namespace new_ast
{
	struct Node
	{
		enum class Index : std::uint8_t
		{
			Id = 0,
			Assign,
			BinaryOp,
			Scope,
			Variable,
			StackValue,
			Function,
			InternalFunction,
			Call,
			Return,
			BranchIfElse,
			Loop,
			Array,

			Size
		};

		Index index;
		arena::Id data_id;

		Node()
			:index(Index::Size)
		{}

		static constexpr std::uint32_t get_nodes_count()
		{
			return static_cast<std::uint32_t>(Index::Size);
		}

		static constexpr std::uint32_t index_as_int(Index index)
		{
			return static_cast<std::uint32_t>(index);
		}

		Node(Index _index, arena::Id id)
			:index{ _index }
			,data_id{id}
		{
		}
	};

	
	template <typename ... Args>
	class NodeDataStorage
	{
	public:
		template <typename  T>
		using Container = arena::Arena<T>;
		using Id = arena::Id;

		NodeDataStorage() = default;

		template <typename T, typename ... InArgs>
		Node create_node(InArgs&& ... args)
		{
			auto& h = std::get<Container<T>>(_hives);

			const auto id = h.emplace(std::forward<InArgs>(args) ...);

			return { T::_node_type, id };
		}

		template <class T>
		const T* get_data(const Node& n) const
		{
			assert(T::_node_type == n.index);
			auto& h = std::get<Container<T>>(_hives);
			return h.at(n.data_id);
		}

		template <class T>
		T* get_data_mut(const Node& n)
		{
			assert(T::_node_type == n.index);
			auto& h = std::get<Container<T>>(_hives);
			return h.at(n.data_id);
		}
	private:
		std::tuple<arena::Arena<Args> ...> _hives;
	};
}
