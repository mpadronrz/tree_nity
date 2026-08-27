#pragma once
#include <functional>
#include <vector>
#include <list>
#include <cmath>
#include <stdexcept>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class	HashMap {
	public:
		using ValueType = std::pair<Key, Value>;
		using BucketVector = std::vector<std::list<ValueType>>;

		explicit HashMap(size_t initial_capacity = 16, float max_load_factor = 0.75f)
			: buckets_(initial_capacity > 0 ? initial_capacity : 16),
			  max_load_factor_(max_load_factor > 0.0f ? max_load_factor : 0.75f) {}

		HashMap(std::initializer_list<ValueType> init_list, float max_load_factor = 0.75f)
			: HashMap(calculate_initial_capacity(init_list.size(), max_load_factor), max_load_factor) {
				insert(init_list);
			}

		HashMap(const HashMap&) = default;
		HashMap& operator=(const HashMap&) = default;
		~HashMap() = default;
		HashMap(HashMap&& other) noexcept
			: buckets_(std::move(other.buckets_)),
			  num_elements_(other.num_elements_),
			  max_load_factor_(other.max_load_factor_),
			  hasher_(std::move(other.hasher_)) {
				other.num_elements_ = 0;
				other.buckets_.assign(1, {});
			  }
		HashMap& operator=(HashMap&& other) noexcept {
			if (this != &other) {
				buckets_ = std::move(other.buckets_);
				num_elements_ = other.num_elements_;
				hasher_ = std::move(other.hasher_);

				other.num_elements_ = 0;
				other.buckets_.assign(1, {});
			}
			return *this;
		}

		[[nodiscard]] size_t size() const noexcept { return num_elements_; }
		[[nodiscard]] bool empty() const noexcept { return num_elements_ == 0; }
		[[nodiscard]] size_t bucket_count() const noexcept { return buckets_.size(); }
		[[nodiscard]] float load_factor() const noexcept {
			return static_cast<float>(num_elements_) / static_cast<float>(buckets_.size());
		}
		[[nodiscard]] float max_load_factor() const noexcept { return max_load_factor_; }

		[[nodiscard]] bool contains(const Key& key) const noexcept { return find(key) != nullptr; }

		/**
		 * @brief Updates the value for an existing key.
		 *
		 * @param key Lookup key.
		 * @param value New value to assign.
		 * @return true if the value was updated, false if the key was not found.
		 */
		bool	replace(const Key& key, const Value& value) {
			auto*	entry = find_entry(key);
			if (entry == nullptr) {
				return false;
			}
			entry->second = value;
			return true;
		}

		/**
		 * @brief Inserts a key-value pair only if the key does not already exist.
		 *
		 * @param key Lookup key.
		 * @param value Value associated with key.
		 * @return true if the element was inserted, false if the key already exists.
		 */
		bool	insert(const Key& key, const Value& value) {
			if (find_entry(key) != nullptr) {
				return false;
			}
			safe_insert(key, value);
			return true;
		}

		/**
		 * @brief Inserts a range of key-value pairs into the map.
		 *
		 * Traverses elements in the range [first, last) and inserts each pair if the key
		 * does not already exist. If an iterator category is forward or stronger, the method
		 * pre-calculates the distance and pre-allocates bucket capacity via reserve()
		 * to avoid intermediate rehashing cycles.
		 *
		 * @tparam InputIt Iterator type satisfying the LegacyInputIterator concept and dereferencing
		 *                 to a pair-like object with .first (convertible to Key) and .second (convertible to Value).
		 * @param first Starting iterator of the range to insert (inclusive).
		 * @param last  Sentinel iterator marking the end of the range (exclusive).
		 *
		 * @note Follows standard associative container duplicate semantics: keys already present
		 *       in the table are preserved, and corresponding new values are ignored.
		 */
		template <typename InputIt>
		void	insert(InputIt first, InputIt last) {
			using Category = typename std::iterator_traits<InputIt>::iterator_category;
			if constexpr (std::is_base_of_v<std::forward_iterator_tag, Category>) {
				auto	count = static_cast<size_t>(std::distance(first, last));
				reserve(num_elements_ + count);
			}
			for (; first != last; ++first) {
				insert(first->first, first->second);
			}
		}

		/**
		 * @brief Inserts elements from an initializer list of key-value pairs.
		 *
		 * Pre-allocates bucket capacity for the additional elements before inserting
		 * each element sequentially. Existing keys are preserved and will not be overwritten.
		 *
		 * @param list An std::initializer_list of std::pair<Key, Value> elements to insert.
		 *
		 * @note Enables direct braced initialization and bulk insertion syntax:
		 *       map.insert({{"client1", meta1}, {"client2", meta2}}).
		 */
		void	insert(std::initializer_list<ValueType> list) {
			reserve(num_elements_ + list.size());
			for (const auto& entry: list) {
				insert(entry.first, entry.second);
			}
		}

		/**
		 * @brief Inserts a new key-value pair or updates the value if the key exists.
		 *
		 * @param key Lookup key.
		 * @param value Value to insert or assign.
		 * @return true if a brand-new element was inserted, false if an existing element was updated.
		 */
		bool	insert_or_assign(const Key& key, const Value& value) {
			if (replace(key, value)) {
				return false;
			}

			safe_insert(key, value);
			return true;
		}

		/**
		 * @brief Looks up a key and returns a pointer to its mutable value.
		 *
		 * @param key Lookup key.
		 * @return Pointer to the stored value, or nullptr if not found.
		 */
		Value*	find(const Key& key) {
			auto*	entry = find_entry(key);
			return entry ? &entry->second : nullptr;
		}

		/**
		 * @brief Read-only lookup of a key for const container instances.
		 *
		 * @param key Lookup key.
		 * @return Const pointer to the stored value, or nullptr if not found.
		 */
		const Value*	find(const Key& key) const {
			auto*	entry = find_entry(key);
			return entry ? &entry->second : nullptr;
		}

		/**
		 * @brief Accesses or inserts the element associated with the specified key.
		 *
		 * If @p key matches an existing entry, returns a reference to its mapped value.
		 * If @p key is not found, inserts a new entry with a copy of @p key and a
		 * default-constructed Value, returning a reference to the newly created value.
		 *
		 * @param key The key of the element to find or insert.
		 * @return Value& Reference to the mapped value (existing or newly inserted).
		 */
		Value& operator[](const Key& key) {
			size_t	idx = get_index(key);
			for (auto& entry : buckets_[idx]) {
				if (entry.first == key) {
					return entry.second;
				}
			}

			if (static_cast<float>(num_elements_ + 1) / static_cast<float>(buckets_.size()) > max_load_factor_) {
				rehash(buckets_.size() * 2);
				idx = get_index(key);
			}

			buckets_[idx].emplace_back(key, Value{});
			++num_elements_;
			return buckets_[idx].back().second;
		}

		/**
		 * @brief Accesses or inserts the element associated with the specified key using move semantics.
		 *
		 * If @p key matches an existing entry, returns a reference to its mapped value.
		 * If @p key is not found, inserts a new entry by moving @p key and binding a
		 * default-constructed Value, returning a reference to the newly created value.
		 *
		 * @param key The rvalue reference to the key of the element to find or insert.
		 * @return Value& Reference to the mapped value (existing or newly inserted).
		 */
		Value& operator[](Key&& key) {
			size_t	idx = get_index(key);
			for (auto& entry : buckets_[idx]) {
				if (entry.first == key) {
					return entry.second;
				}
			}

			if (static_cast<float>(num_elements_ + 1) / static_cast<float>(buckets_.size()) > max_load_factor_) {
				rehash(buckets_.size() * 2);
				idx = get_index(key);
			}

			buckets_[idx].emplace_back(std::move(key), Value{});
			++num_elements_;
			return buckets_[idx].back().second;
		}

		/**
		 * @brief Accesses the element with the specified key with bounds checking.
		 *
		 * @param key The key of the element to find.
		 * @return Value& Reference to the mapped value.
		 * @throws std::out_of_range If the specified key is not present in the container.
		 */
		Value&	at(const Key& key) {
			size_t	idx = get_index(key);
			for (auto& entry: buckets_[idx]) {
				if (entry.first == key) {
					return entry.second;
				}
			}
			throw std::out_of_range("HashMap::at: key not found");
		}

		/**
		 * @brief Accesses the element with the specified key with bounds checking (const overload).
		 *
		 * @param key The key of the element to find.
		 * @return const Value& Const reference to the mapped value.
		 * @throws std::out_of_range If the specified key is not present in the container.
		 */
		const Value& at(const Key& key) const {
			size_t	idx = get_index(key);
			for (const auto& entry: buckets_[idx]) {
				if (entry.first == key) {
					return entry.second;
				}
			}
			throw std::out_of_range("HashMap::at: key not found");
		}

		/**
		 * @brief Reserves minimum bucket capacity to accommodate a specified element count.
		 *
		 * Calculates the required bucket count to hold at least `count` elements without
		 * exceeding the configured `max_load_factor_`. If the calculated bucket count exceeds
		 * the current capacity, a single rehash() operation is triggered immediately.
		 *
		 * @param count The expected total number of elements the map should hold.
		 *
		 * @note Prevents multiple cascading rehash allocations when performing bulk insertions
		 *       or when the expected dataset size is known ahead of time.
		 */
		void	reserve(size_t count) {
			auto	required_buckets = static_cast<size_t>(std::ceil(static_cast<float>(count) / max_load_factor_));
			if (required_buckets > buckets_.size()) {
				rehash(required_buckets);
			}
		}

		/**
		 * @brief Removes a key and its associated value from the container.
		 *
		 * @param key Lookup key to delete.
		 * @return true if an entry was removed, false if the key was absent.
		 */
		bool	erase(const Key& key) {
			size_t	idx = get_index(key);
			auto&	bucket = buckets_[idx];

			for (auto it = bucket.begin(); it != bucket.end(); ++it) {
				if (it->first == key) {
					bucket.erase(it);
					--num_elements_;
					return true;
				}
			}
			return false;
		}

		/**
		 * @brief Removes all elements from the map without reducing bucket capacity.
		 */
		void	clear() noexcept {
			for (auto& bucket : buckets_) {
				bucket.clear();
			}
			num_elements_ = 0;
		}

		template <bool IsConst>
		class IteratorImpl {
			public:
				using iterator_category = std::forward_iterator_tag;
				using value_type = ValueType;
				using difference_type = std::ptrdiff_t;

				using pointer = std::conditional_t<IsConst, const ValueType*, ValueType*>;
				using reference = std::conditional_t<IsConst, const ValueType&, ValueType&>;

				using VectorPtr = std::conditional_t<IsConst, const BucketVector*, BucketVector*>;
				using ListIt = std::conditional_t<IsConst,
												  typename std::list<ValueType>::const_iterator,
												  typename std::list<ValueType>::iterator>;

				IteratorImpl() = default;

				template <bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
				IteratorImpl(const IteratorImpl<OtherConst>& other)
					: buckets_(other.buckets_), bucket_idx_(other.bucket_idx_), list_it_(other.list_it_) {}

				reference	operator*() const { return *list_it_; }
				pointer	operator->() const { return &(*list_it_); }

				IteratorImpl&	operator++() {
					++list_it_;
					if (list_it_ == (*buckets_)[bucket_idx_].end()) {
						++bucket_idx_;
						advance_to_next_valid_bucket();
					}
					return *this;
				}
				IteratorImpl	operator++(int) {
					IteratorImpl temp = *this;
					++(*this);
					return temp;
				}

				template<bool OtherConst>
				bool operator==(const IteratorImpl<OtherConst>& other) const {
					if (bucket_idx_ >= buckets_->size() && other.bucket_idx_ >= buckets_->size()) {
						return true;
					}
					return bucket_idx_ == other.bucket_idx_ && list_it_ == other.list_it_;
				}
				template<bool OtherConst>
				bool operator!=(const IteratorImpl<OtherConst>& other) const {
					return !(*this == other);
				}

			private:
				friend class HashMap;
				template <bool> friend class IteratorImpl;

				VectorPtr	buckets_{nullptr};
				size_t	bucket_idx_{0};
				ListIt	list_it_{};

				IteratorImpl(VectorPtr buckets, size_t bucket_idx)
					: buckets_(buckets), bucket_idx_(bucket_idx) {
						advance_to_next_valid_bucket();
					}

				void	advance_to_next_valid_bucket() {
					while (bucket_idx_ < buckets_->size() && (*buckets_)[bucket_idx_].empty()) {
						++bucket_idx_;
					}
					if (bucket_idx_ < buckets_->size()) {
						list_it_ = (*buckets_)[bucket_idx_].begin();
					}
				}
		};

		using Iterator = IteratorImpl<false>;
		using ConstIterator = IteratorImpl<true>;

		Iterator begin() noexcept { return Iterator(&buckets_, 0); }
		Iterator end() noexcept { return Iterator(&buckets_, buckets_.size()); }

		ConstIterator begin() const noexcept { return ConstIterator(&buckets_, 0); }
		ConstIterator end() const noexcept { return ConstIterator(&buckets_, buckets_.size()); }

		ConstIterator cbegin() const noexcept { return begin(); }
		ConstIterator cend() const noexcept { return end(); }

	private:
		BucketVector	buckets_;
		size_t	num_elements_{0};
		float	max_load_factor_{0.75f};
		Hash	hasher_{};

		static size_t	calculate_initial_capacity(size_t elements, float load_factor) {
			float	factor = load_factor > 0.0f ? load_factor : 0.75f;
			size_t	capacity = static_cast<size_t>(std::ceil(static_cast<float>(elements) / factor));
			return std::max(size_t{16}, capacity);
		}
		size_t	get_index(const Key& key) const { return hasher_(key) % buckets_.size(); }

		ValueType*	find_entry(const Key& key) {
			size_t	idx = get_index(key);
			for (auto& entry : buckets_[idx]) {
				if (entry.first == key) {
					return &entry;
				}
			}
			return nullptr;
		}

		const ValueType*	find_entry(const Key& key) const {
			size_t	idx = get_index(key);
			for (auto& entry : buckets_[idx]) {
				if (entry.first == key) {
					return &entry;
				}
			}
			return nullptr;
		}

		void safe_insert(const Key& key, const Value& value) {
			if (static_cast<float>(num_elements_ + 1) / static_cast<float>(buckets_.size()) > max_load_factor_) {
				rehash(buckets_.size() * 2);
			}

			size_t idx = get_index(key);
			buckets_[idx].emplace_back(key, value);
			++num_elements_;
		}

		void	rehash(size_t	new_capacity) {
			BucketVector	new_buckets(new_capacity);

			for (auto& old_bucket: buckets_) {
				for (auto& entry : old_bucket) {
					size_t	new_idx = hasher_(entry.first) % new_capacity;
					new_buckets[new_idx].push_back(std::move(entry));
				}
			}
			buckets_ = std::move(new_buckets);
		}
};
