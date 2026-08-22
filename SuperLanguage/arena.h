#pragma once

#include <cstdint>
#include <vector>
#include <limits>
#include <cassert>
#include <memory>
#include <new>
#include <utility>
#include <iterator>
#include <type_traits>

namespace arena {
	static constexpr std::uint16_t kInvalidBucket = std::numeric_limits<std::uint16_t>::max();
	struct Id
	{
		std::uint16_t bucket = kInvalidBucket;
		std::uint16_t index = 0;
	};

	template <typename T, size_t BucketSize = 1024>
	class Arena final
	{
	private:
		struct Slot
		{
			std::uint16_t index = 0;
			std::uint16_t bucket = 0;
		};

		struct Bucket {
			std::uint16_t watermark = 0;
			alignas(T) std::byte data[sizeof(T) * BucketSize];

			Bucket() noexcept = default;

			T* get_element(size_t index)
			{
				return std::launder(reinterpret_cast<T*>(&data[index * sizeof(T)]));
			}

			const T* get_element(size_t index) const
			{
				return std::launder(reinterpret_cast<const T*>(&data[index * sizeof(T)]));
			}

			void* construct_address(size_t index)
			{
				return &data[index * sizeof(T)];
			}
		};
		using BucketPtr = std::unique_ptr<Bucket>;

		static_assert(BucketSize < std::numeric_limits<std::uint16_t>::max());

		template <bool IsConst>
		class Iterator
		{
			friend Arena;
			using ArenaPtr = std::conditional_t<IsConst, const Arena*, Arena*>;

			ArenaPtr     _hive = nullptr;
			std::size_t _bucket = 0;
			std::size_t _index = 0;

			Iterator(ArenaPtr h, std::size_t bucket, std::size_t index) noexcept
				: _hive(h), _bucket(bucket), _index(index) {}

			// встать на текущий живой слот или на end()
			void seek_forward() noexcept
			{
				const auto& bs = _hive->_buckets;
				for (; _bucket < bs.size(); ++_bucket, _index = 0) {
					const Bucket& b = *bs[_bucket];
					for (; _index < b.watermark; ++_index) {
						
					}
				}
				_index = 0;                       // end() == { size(), 0 }
			}

			void seek_backward() noexcept
			{
				const auto& bs = _hive->_buckets;
				std::size_t b = _bucket, i = _index;
				while (true) {
					if (i == 0) {
						if (b == 0) return;       // --begin(): UB, просто не двигаемся
						i = bs[--b]->watermark;
						if (i == 0) continue;     // пустой бакет — левее
					}
				}
			}

		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using iterator_concept = std::bidirectional_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = std::conditional_t<IsConst, const T*, T*>;
			using reference = std::conditional_t<IsConst, const T&, T&>;

			Iterator() noexcept = default;

			reference operator*()  const noexcept { return *operator->(); }
			pointer   operator->() const noexcept { return _hive->_buckets[_bucket]->get_element(_index); }

			Iterator& operator++()    noexcept { ++_index; seek_forward();  return *this; }
			Iterator& operator--()    noexcept { seek_backward();           return *this; }
			Iterator  operator++(int) noexcept { auto t = *this; ++*this; return t; }
			Iterator  operator--(int) noexcept { auto t = *this; --*this; return t; }

			friend bool operator==(const Iterator&, const Iterator&) noexcept = default;

			operator Iterator<true>() const noexcept requires (!IsConst)
			{
				return { _hive, _bucket, _index };
			}

			Id id() const noexcept
			{
				const Bucket& b = *_hive->_buckets[_bucket];
				return { .bucket = static_cast<std::uint16_t>(_bucket),
						 .index = static_cast<std::uint16_t>(_index) };
			}
		};


		void grow()
		{
			assert(_buckets.size() < std::numeric_limits<std::uint16_t>::max());
			if (_buckets.empty() || _buckets.back()->watermark >= BucketSize) {
				_buckets.emplace_back(std::make_unique<Bucket>());
			}
		}

		void destroy_all() noexcept
		{
			for (auto& b : _buckets) {
				for (size_t i = 0; i < b->watermark; ++i) {
					b->get_element(i)->~T();
				}
			}
		}

		const T* find(const Id& id) const noexcept
		{
			if (id.bucket < _buckets.size()) {
				Bucket& b = *_buckets[id.bucket];
				if (id.index < b.watermark ) {
					return b.get_element(id.index);
				}
			}
			return nullptr;
		}

	public:
		using iterator = Iterator<false>;
		using const_iterator = Iterator<true>;

		iterator begin() noexcept { iterator it{ this, 0, 0 }; it.seek_forward(); return it; }
		iterator end()   noexcept { return { this, _buckets.size(), 0 }; }

		const_iterator begin()  const noexcept { const_iterator it{ this, 0, 0 }; it.seek_forward(); return it; }
		const_iterator end()    const noexcept { return { this, _buckets.size(), 0 }; }
		const_iterator cbegin() const noexcept { return begin(); }
		const_iterator cend()   const noexcept { return end(); }

		Arena() = default;
		Arena(const Arena&) = delete;
		Arena(Arena&& other) noexcept
			:_buckets(std::move(other._buckets))
			, _size(std::exchange(other._size, 0))
		{
		}
		~Arena() noexcept
		{
			destroy_all();
		}

		Arena& operator=(const Arena&) = delete;
		Arena& operator=(Arena&& h) noexcept
		{
			if (this != &h) {
				destroy_all();
				_buckets = std::move(h._buckets);
				_size = h._size;
				h._size = 0;
			}
			return *this;
		}

		void clean() noexcept
		{
			destroy_all();
			_buckets.clear();
			_size = 0;
		}

		size_t size() const noexcept
		{
			return _size;
		}

		const T* at(const Id& id) const noexcept
		{
			return find(id);
		}

		T* at(const Id& id) noexcept
		{
			return const_cast<T*>(find(id));
		}

		template <typename ... Args>
		Id emplace(Args && ... args)
		{
			grow();

			Bucket& b = *_buckets.back();
			const auto index = b.watermark;
			auto ptr = b.construct_address(index);
			new (ptr) T{ std::forward<Args>(args) ... };
			b.watermark += 1;
			_size += 1;

			return { .bucket = static_cast<uint16_t>(_buckets.size() - 1), .index = static_cast<uint16_t>(index) };
		}

		bool contains(const Id& id) const noexcept
		{
			return find(id) != nullptr;
		}

	private:
		std::vector<BucketPtr> _buckets;
		size_t _size = 0;
	};

}
