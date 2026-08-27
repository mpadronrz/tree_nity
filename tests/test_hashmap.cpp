#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

#include "data_structures/HashMap.hpp"



// ============================================================================
// Suite 1: Constructors & Lifecycle (Rule of 5)
// ============================================================================


TEST(HashMapConstructorsTest, DefaultConstructor) {
	HashMap<std::string, int> map;

	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0u);
	EXPECT_EQ(map.bucket_count(), 16u);
	EXPECT_FLOAT_EQ(map.load_factor(), 0.0f);
	EXPECT_EQ(map.begin(), map.end());
}

TEST(HashMapConstructorsTest, SizedAndCustomLoadFactorConstructor) {
	HashMap<std::string, int> map(64, 0.5f);

	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0u);
	EXPECT_EQ(map.bucket_count(), 64u);
	EXPECT_FLOAT_EQ(map.load_factor(), 0.0f);

	HashMap<std::string, int> zero_cap_map(0, 0.75f);
	EXPECT_EQ(zero_cap_map.bucket_count(), 16u);

	HashMap<std::string, int> zero_load_map(16, 0.0f);
	EXPECT_GT(zero_cap_map.max_load_factor(), 0.1f);
}

TEST(HashMapConstructorsTest, InitializerListConstructor) {
	HashMap<std::string, int> map = {
		{"client0", 100},
		{"client1", 200},
		{"client2", 300}
	};

	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.size(), 3u);
	EXPECT_TRUE(map.contains("client0"));
	EXPECT_TRUE(map.contains("client1"));
	EXPECT_TRUE(map.contains("client2"));
	EXPECT_EQ(*map.find("client0"), 100);
	EXPECT_EQ(*map.find("client1"), 200);
	EXPECT_EQ(*map.find("client2"), 300);
}

TEST(HashMapConstructorsTest, CopyConstructorOperator) {
	HashMap<std::string, int> original;
	original.insert_or_assign("keyA", 42);
	original.insert_or_assign("keyB", 84);

	HashMap<std::string, int> copy(original);

	EXPECT_EQ(copy.size(), original.size());
	EXPECT_TRUE(copy.contains("keyA"));
	EXPECT_TRUE(copy.contains("keyB"));
	EXPECT_EQ(*copy.find("keyA"), 42);

	original.insert_or_assign("keyC", 126);
	*original.find("keyA") = 666;

	EXPECT_EQ(original.size(), 3u);
	EXPECT_EQ(*original.find("keyA"), 666);
	EXPECT_EQ(*original.find("keyC"), 126);
	EXPECT_EQ(copy.size(), 2u);
	EXPECT_FALSE(copy.contains("keyC"));
	EXPECT_EQ(*copy.find("keyA"), 42);
}

TEST(HashMapConstructorsTest, CopyAssignmentOperator) {
	HashMap<std::string, int> map1;
	map1.insert_or_assign("alpha", 1);
	map1.insert_or_assign("beta", 2);

	HashMap<std::string, int> map2;
	map2.insert_or_assign("eliminated", 99);

	map2 = map1;

	EXPECT_EQ(map2.size(), 2u);
	EXPECT_FALSE(map2.contains("eliminated"));
	EXPECT_TRUE(map2.contains("alpha"));
	EXPECT_TRUE(map2.contains("beta"));
	EXPECT_EQ(*map2.find("alpha"), 1);

	map1.erase("alpha");
	EXPECT_FALSE(map1.contains("alpha"));
	EXPECT_TRUE(map2.contains("alpha"));
}

TEST(HashMapConstructorsTest, MoveConstructorOperator) {
	HashMap<std::string, std::string> source;
	source.insert_or_assign("clientA", "ipc_endpoint_A");
	source.insert_or_assign("clientB", "ipc_endpoint_B");

	HashMap<std::string, std::string> destination(std::move(source));

	EXPECT_EQ(destination.size(), 2u);
	EXPECT_TRUE(destination.contains("clientA"));
	EXPECT_TRUE(destination.contains("clientB"));
	EXPECT_EQ(*destination.find("clientA"), "ipc_endpoint_A");

	EXPECT_EQ(source.size(), 0u);
	EXPECT_TRUE(source.empty());
}

TEST(HashMapConstructorsTest, MoveAssignmentOperator) {
	HashMap<std::string, int> source;
	source.insert_or_assign("sensor_1", 55);
	source.insert_or_assign("sensor_2", 89);

	HashMap<std::string, int> destination;
	destination.insert_or_assign("discarded_key", 1);

	destination = std::move(source);

	EXPECT_EQ(destination.size(), 2u);
	EXPECT_FALSE(destination.contains("discarded_key"));
	EXPECT_TRUE(destination.contains("sensor_1"));
	EXPECT_TRUE(destination.contains("sensor_2"));
	EXPECT_EQ(*destination.find("sensor_1"), 55);

	EXPECT_EQ(source.size(), 0u);
	EXPECT_TRUE(source.empty());
}

TEST(HashMapConstructorsTest, ReservePreAllocation) {
	HashMap<int, int> map(4, 0.75f);
	EXPECT_EQ(map.size(), 0u);
	EXPECT_EQ(map.bucket_count(), 4u);

	map.reserve(100);// Verify all keys remain accessible

	EXPECT_EQ(map.bucket_count(), 134u);

	size_t buckets_after_reserve = map.bucket_count();
	for (int i = 0; i < 50; ++i) {
		map[i] = i * 2;
	}

	EXPECT_EQ(map.size(), 50u);
	EXPECT_EQ(map.bucket_count(), buckets_after_reserve);

	for (int i = 0; i < 50; ++i) {
		EXPECT_EQ(map.at(i), i * 2);
	}
}



// ============================================================================
// Suite 2: Modifiers (Insert, Update, Erase, Clear, Rehashing)
// ============================================================================


TEST(HashMapModifiersTest, InsertOnly) {
	HashMap<std::string, int> map;

	EXPECT_TRUE(map.insert("keyA", 10));
	EXPECT_TRUE(map.insert("keyB", 20));
	EXPECT_TRUE(map.insert("keyC", 30));
	EXPECT_EQ(*map.find("keyA"), 10);
	EXPECT_EQ(*map.find("keyB"), 20);
	EXPECT_EQ(*map.find("keyC"), 30);
	EXPECT_EQ(map.size(), 3u);

	EXPECT_FALSE(map.insert("keyA", 40));
	EXPECT_EQ(map.size(), 3u);
	EXPECT_EQ(*map.find("keyA"), 10);
}

TEST(HashMapModifiersTest, ReplaceOnly) {
	HashMap<std::string, int> map;

	EXPECT_TRUE(map.insert("keyA", 10));
	EXPECT_TRUE(map.insert("keyB", 20));
	EXPECT_EQ(*map.find("keyA"), 10);
	EXPECT_EQ(*map.find("keyB"), 20);
	EXPECT_EQ(map.size(), 2u);

	EXPECT_TRUE(map.replace("keyA", 30));
	EXPECT_TRUE(map.replace("keyB", 40));
	EXPECT_FALSE(map.replace("keyC", 50));

	EXPECT_EQ(map.size(), 2u);
	EXPECT_EQ(*map.find("keyA"), 30);
	EXPECT_EQ(*map.find("keyB"), 40);
	EXPECT_EQ(map.find("keyC"), nullptr);
}

TEST(HashMapModifiersTest, InsertNewKeys) {
	HashMap<std::string, int> map;

	EXPECT_TRUE(map.insert_or_assign("keyA", 10));
	EXPECT_TRUE(map.insert_or_assign("keyB", 20));
	EXPECT_TRUE(map.insert_or_assign("keyC", 30));

	EXPECT_EQ(map.size(), 3u);
	EXPECT_FALSE(map.empty());
}

TEST(HashMapModifiersTest, UpdateExistingKeys) {
	HashMap<std::string, int> map;

	EXPECT_TRUE(map.insert_or_assign("keyA", 100));
	EXPECT_EQ(map.size(), 1u);

	EXPECT_FALSE(map.insert_or_assign("keyA", 999));
	EXPECT_EQ(map.size(), 1u);

	int* val = map.find("keyA");
	ASSERT_NE(val, nullptr);
	EXPECT_EQ(*val, 999);
}

TEST(HashMapModifiersTest, EraseExistingAndNonExistent) {
	HashMap<std::string, int> map;
	map.insert_or_assign("k1", 1);
	map.insert_or_assign("k2", 2);
	map.insert_or_assign("k3", 3);

	EXPECT_EQ(map.size(), 3u);

	EXPECT_TRUE(map.erase("k2"));
	EXPECT_EQ(map.size(), 2u);
	EXPECT_EQ(map.find("k2"), nullptr);

	EXPECT_FALSE(map.erase("k2"));
	EXPECT_FALSE(map.erase("non_existent"));
	EXPECT_EQ(map.size(), 2u);

	EXPECT_TRUE(map.erase("k1"));
	EXPECT_TRUE(map.erase("k3"));
	EXPECT_EQ(map.size(), 0u);
	EXPECT_TRUE(map.empty());
}

TEST(HashMapModifiersTest, ClearOperation) {
	HashMap<std::string, int> map;
	for (int i = 0; i < 50; ++i) {
		map.insert_or_assign("id_" + std::to_string(i), i);
	}
	EXPECT_EQ(map.size(), 50u);
	EXPECT_NE(map.find("id_0"), nullptr);
	EXPECT_NE(map.find("id_49"), nullptr);

	map.clear();

	EXPECT_EQ(map.size(), 0u);
	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.find("id_0"), nullptr);
	EXPECT_EQ(map.find("id_49"), nullptr);

	EXPECT_TRUE(map.insert_or_assign("fresh_key", 777));
	EXPECT_EQ(map.size(), 1u);
	ASSERT_NE(map.find("fresh_key"), nullptr);
	EXPECT_EQ(*map.find("fresh_key"), 777);
}

TEST(HashMapModifiersTest, CollisionChaining) {
	struct ForceCollisionHasher {
		size_t operator()(const std::string&) const { return 0; }
	};

	HashMap<std::string, int, ForceCollisionHasher> map(4);

	EXPECT_TRUE(map.insert_or_assign("col_1", 100));
	EXPECT_TRUE(map.insert_or_assign("col_2", 200));
	EXPECT_TRUE(map.insert_or_assign("col_3", 300));

	EXPECT_EQ(map.size(), 3u);

	EXPECT_TRUE(map.erase("col_2"));
	EXPECT_EQ(map.size(), 2u);
	EXPECT_EQ(map.find("col_2"), nullptr);

	ASSERT_NE(map.find("col_1"), nullptr);
	ASSERT_NE(map.find("col_3"), nullptr);
	EXPECT_EQ(*map.find("col_1"), 100);
	EXPECT_EQ(*map.find("col_3"), 300);
}

TEST(HashMapModifiersTest, DynamicRehashingGrowth) {
	HashMap<int, int> map(4, 0.5f);
	size_t initial_buckets = map.bucket_count();
	EXPECT_EQ(initial_buckets, 4u);

	for (int i = 0; i < 20; ++i) {
		map.insert_or_assign(i, i * 10);
	}

	EXPECT_EQ(map.size(), 20u);
	EXPECT_GT(map.bucket_count(), initial_buckets);
	EXPECT_EQ(map.bucket_count(), 64u);

	for (int i = 0; i < 20; ++i) {
		int* val = map.find(i);
		ASSERT_NE(val, nullptr);
		EXPECT_EQ(*val, i * 10);
	}
}



// ============================================================================
// Suite 3: Lookups (find, contains, const-correctness, collision retrieval)
// ============================================================================


TEST(HashMapLookupTest, FindExistingElements) {
	HashMap<std::string, int> map;
	map.insert_or_assign("key0", 100);
	map.insert_or_assign("key1", 200);

	int* val0 = map.find("key0");
	int* val1 = map.find("key1");

	ASSERT_NE(val0, nullptr);
	ASSERT_NE(val1, nullptr);
	EXPECT_EQ(*val0, 100);
	EXPECT_EQ(*val1, 200);
}

TEST(HashMapLookupTest, FindNonExistentReturnsNull) {
	HashMap<std::string, int> map;
	map.insert_or_assign("key", 42);

	EXPECT_EQ(map.find("missing_key"), nullptr);
	EXPECT_EQ(map.find(""), nullptr);

	map.erase("key");
	EXPECT_EQ(map.find("key"), nullptr);
}

TEST(HashMapLookupTest, InPlaceMutationViaFindPointer) {
	HashMap<std::string, std::string> map;
	map.insert_or_assign("key1", "value1");

	std::string* path = map.find("key1");
	ASSERT_NE(path, nullptr);
	EXPECT_EQ(*path, "value1");

	*path = "value2";

	EXPECT_EQ(*map.find("key1"), "value2");
	EXPECT_EQ(map.size(), 1u);
}

TEST(HashMapLookupTest, ConstPointerFind) {
	HashMap<std::string, int> map;
	map.insert_or_assign("alpha", 75);

	const auto& const_map = map;

	const int* val = const_map.find("alpha");
	ASSERT_NE(val, nullptr);
	EXPECT_EQ(*val, 75);

	EXPECT_EQ(const_map.find("beta"), nullptr);
}

TEST(HashMapLookupTest, ContainsValidation) {
	HashMap<std::string, int> map;
	map.insert_or_assign("topicA", 1);
	map.insert_or_assign("topicB", 2);

	EXPECT_TRUE(map.contains("topicA"));
	EXPECT_TRUE(map.contains("topicB"));
	EXPECT_FALSE(map.contains("topicC"));
	EXPECT_FALSE(map.contains(""));

	map.erase("topicA");
	EXPECT_FALSE(map.contains("topicA"));
	EXPECT_TRUE(map.contains("topicB"));
}

TEST(HashMapLookupTest, LookupAcrossCollisionChain) {
	struct SingleBucketHasher {
		size_t operator()(const std::string&) const { return 0; }
	};

	HashMap<std::string, int, SingleBucketHasher> map(4);
	map.insert_or_assign("first", 10);
	map.insert_or_assign("second", 20);
	map.insert_or_assign("third", 30);

	EXPECT_TRUE(map.contains("first"));
	EXPECT_TRUE(map.contains("second"));
	EXPECT_TRUE(map.contains("third"));

	ASSERT_NE(map.find("first"), nullptr);
	ASSERT_NE(map.find("second"), nullptr);
	ASSERT_NE(map.find("third"), nullptr);

	EXPECT_EQ(*map.find("first"), 10);
	EXPECT_EQ(*map.find("second"), 20);
	EXPECT_EQ(*map.find("third"), 30);

	EXPECT_FALSE(map.contains("fourth"));
	EXPECT_EQ(map.find("fourth"), nullptr);
}

TEST(HashMapLookupTest, AtExistingElement) {
	HashMap<std::string, int> map;
	map.insert_or_assign("key", 100);

	EXPECT_EQ(map.at("key"), 100);
	map.at("key") = 250;
	EXPECT_EQ(map.at("key"), 250);

	const auto& const_map = map;
	EXPECT_EQ(const_map.at("key"), 250);
}

TEST(HashMapLookupTest, AtThrowsOutOfRangeOnMissingKey) {
	HashMap<std::string, int> map;
	map.insert_or_assign("key", 100);

	EXPECT_THROW(map.at("non_existent"), std::out_of_range);

	const auto& const_map = map;
	EXPECT_THROW(const_map.at("missing_const_key"), std::out_of_range);
}

TEST(HashMapModifiersTest, SubscriptOperatorAccessAndInsertion) {
	HashMap<std::string, int> map;

	map["key0"] = 42;
	EXPECT_EQ(map.size(), 1u);
	EXPECT_EQ(map["key0"], 42);

	map["key0"] = 99;
	EXPECT_EQ(map.size(), 1u);
	EXPECT_EQ(map["key0"], 99);

	EXPECT_EQ(map["key1"], 0);
	EXPECT_EQ(map.size(), 2u);
	EXPECT_TRUE(map.contains("key1"));
}



// ============================================================================
// Suite 4: Iterators (Traversal, Constness, Conversions, STL Integration)
// ============================================================================


TEST(HashMapIteratorsTest, EmptyMapIteration) {
	HashMap<std::string, int> map;

	EXPECT_EQ(map.begin(), map.end());
	EXPECT_EQ(map.cbegin(), map.cend());

	int iteration_count = 0;
	for (const auto& [key, value] : map) {
		(void)key;
		(void)value;
		++iteration_count;
	}
	EXPECT_EQ(iteration_count, 0);
}

TEST(HashMapIteratorsTest, MutableIterationAndInPlaceUpdate) {
	HashMap<std::string, int> map;
	map.insert_or_assign("keyA", 10);
	map.insert_or_assign("keyB", 20);
	map.insert_or_assign("keyC", 30);

	for (auto& [key, value] : map) {
		value += 5;
	}

	EXPECT_EQ(*map.find("keyA"), 15);
	EXPECT_EQ(*map.find("keyB"), 25);
	EXPECT_EQ(*map.find("keyC"), 35);
}

TEST(HashMapIteratorsTest, MultiBucketHopping) {
	HashMap<int, int> map(16);
	map.insert_or_assign(1, 100);
	map.insert_or_assign(17, 200);
	map.insert_or_assign(5, 500);

	std::vector<int> traversed_keys;
	for (auto it = map.begin(); it != map.end(); ++it) {
		traversed_keys.push_back(it->first);
	}

	EXPECT_EQ(traversed_keys.size(), 3u);
	EXPECT_NE(std::find(traversed_keys.begin(), traversed_keys.end(), 1), traversed_keys.end());
	EXPECT_NE(std::find(traversed_keys.begin(), traversed_keys.end(), 17), traversed_keys.end());
	EXPECT_NE(std::find(traversed_keys.begin(), traversed_keys.end(), 5), traversed_keys.end());
}

TEST(HashMapIteratorsTest, ConstTraversalAndExplicitCBegin) {
	HashMap<std::string, int> map;
	map.insert_or_assign("x", 100);
	map.insert_or_assign("y", 200);

	const auto& const_map = map;
	int sum = 0;

	for (auto it = const_map.begin(); it != const_map.end(); ++it) {
		sum += it->second;
	}
	EXPECT_EQ(sum, 300);

	auto cit = map.cbegin();
	ASSERT_NE(cit, map.cend());
	EXPECT_TRUE(cit != map.cend());
}

TEST(HashMapIteratorsTest, ImplicitConversionAndCrossComparisons) {
	HashMap<std::string, int> map;
	map.insert_or_assign("target", 42);

	HashMap<std::string, int>::Iterator it = map.begin();
	HashMap<std::string, int>::ConstIterator cit = it;

	EXPECT_TRUE(it == cit);
	EXPECT_FALSE(it != cit);
	EXPECT_TRUE(cit == it);
	EXPECT_FALSE(cit != it);

	EXPECT_TRUE(it != map.cend());
	EXPECT_TRUE(cit != map.end());

	++it;
	++cit;
	EXPECT_EQ(it, map.end());
	EXPECT_EQ(cit, map.cend());
	EXPECT_TRUE(it == map.cend());
	EXPECT_TRUE(cit == map.end());
}

TEST(HashMapIteratorsTest, STLAlgorithmCompatibility) {
	HashMap<std::string, int> map;
	map.insert_or_assign("sub1", 10);
	map.insert_or_assign("sub2", 50);
	map.insert_or_assign("sub3", 100);

	EXPECT_EQ(std::distance(map.begin(), map.end()), 3);
	EXPECT_EQ(std::distance(map.cbegin(), map.cend()), 3);

	auto count_above_20 = std::count_if(map.begin(), map.end(), [](const auto& pair) {
		return pair.second > 20;
	});
	EXPECT_EQ(count_above_20, 2);

	auto it = map.begin();
	EXPECT_EQ((*it).first, it->first);
	EXPECT_EQ((*it).second, it->second);
}
