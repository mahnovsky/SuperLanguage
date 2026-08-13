#pragma once

#include "hive.h"

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
		hive::Id data_id;

		Node(Index _index)
			:index(_index)
		{
		}

		static constexpr std::uint32_t get_nodes_count()
		{
			return static_cast<std::uint32_t>(Index::Size);
		}

		static constexpr std::uint32_t index_as_int(Index index)
		{
			return static_cast<std::uint32_t>(index);
		}
	};

	class NodeDataHolder
	{
	public:

		NodeDataHolder() = default;
		NodeDataHolder(const NodeDataHolder&) = delete;
		NodeDataHolder(NodeDataHolder&&) = default;

		~NodeDataHolder()
		{
			if (_destroy_fn && _hive_ptr) {
				_destroy_fn(_hive_ptr);
			}
		}

		bool is_initialized() const
		{
			return _hive_ptr != nullptr;
		}

		template <typename T>
		void init()
		{
			_hive_ptr = new hive::Hive<T>{};
			_destroy_fn = [](void* ptr) {
				delete static_cast<hive::Hive<T>*>(ptr);
				};
		}

		template <typename T>
		hive::Hive<T>* get_hive()
		{
			return static_cast<hive::Hive<T>*>(_hive_ptr);
		}

	private:
		void* _hive_ptr = nullptr;
		std::function<void(void*)> _destroy_fn = {};
	};
	template <typename ... Args>
	class NodeDataStorage
	{
	public:
		NodeDataStorage() = default;

		template <class T>
		hive::Id add_data(T&& data)
		{
			auto& h = std::get<hive::Hive<T>>(_hives);
			return h.emplace(data);
		}

		template <class T>
		const T* get_data(const hive::Id& id) const
		{
			auto& h = std::get<hive::Hive<T>>(_hives);
			return h.at(id);
		}

		template <class T>
		T* get_data_mut(const hive::Id& id)
		{
			auto& h = std::get<hive::Hive<T>>(_hives);
			return h.at(id);
		}
	private:
		NodeDataHolder _data_table[Node::get_nodes_count()];
		std::tuple<hive::Hive<Args> ...> _hives;
	};
}