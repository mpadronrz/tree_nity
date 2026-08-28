#include <gtest/gtest.h>
#include <algorithm>

#include "data_structures/PrefixTree.hpp"



// ============================================================================
// Suite 1: Constructors & Move Semantics (PrefixTreeConstructorsTest)
// ============================================================================


TEST(PrefixTreeConstructorsTest, DefaultConstructorInitialState) {
	PrefixTree tree;

	EXPECT_TRUE(tree.empty());
	EXPECT_EQ(tree.subscriber_count(), 0u);
	EXPECT_TRUE(tree.match("user.create").empty());
	EXPECT_TRUE(tree.match("").empty());
}

TEST(PrefixTreeConstructorsTest, DisallowCopySemanticsAtCompileTime) {
	EXPECT_FALSE(std::is_copy_constructible_v<PrefixTree>);
	EXPECT_FALSE(std::is_copy_assignable_v<PrefixTree>);

	EXPECT_TRUE(std::is_nothrow_move_constructible_v<PrefixTree>);
	EXPECT_TRUE(std::is_nothrow_move_assignable_v<PrefixTree>);
}

TEST(PrefixTreeConstructorsTest, MoveConstructorResourceTheft) {
	PrefixTree source;
	source.insert("auth.login", "client_auth");
	source.insert("order.create", "client_order");
	source.insert("", "wildcard_client");

	EXPECT_EQ(source.subscriber_count(), 3u);
	EXPECT_FALSE(source.empty());

	PrefixTree destination(std::move(source));

	EXPECT_EQ(destination.subscriber_count(), 3u);
	EXPECT_FALSE(destination.empty());

	auto auth_matches = destination.match("auth.login");
	ASSERT_EQ(auth_matches.size(), 2u);
	EXPECT_EQ(auth_matches[0], "wildcard_client");
	EXPECT_EQ(auth_matches[1], "client_auth");

	EXPECT_EQ(source.subscriber_count(), 0u);
	EXPECT_TRUE(source.empty());
	EXPECT_TRUE(source.match("auth.login").empty());
	source.insert("auth.login", "client_auth");
	EXPECT_EQ(source.subscriber_count(), 1u);
	EXPECT_FALSE(source.match("auth.login").empty());
}

TEST(PrefixTreeConstructorsTest, MoveAssignmentOperator) {
	PrefixTree source;
	source.insert("metrics.cpu", "monitor_cpu");

	PrefixTree destination;
	destination.insert("old.topic", "discarded_client");
	destination.insert("another.topic", "discarded_client_2");

	EXPECT_EQ(destination.subscriber_count(), 2u);

	destination = std::move(source);

	EXPECT_EQ(destination.subscriber_count(), 1u);
	EXPECT_FALSE(destination.empty());
	EXPECT_TRUE(destination.match("old.topic").empty());

	auto metrics_matches = destination.match("metrics.cpu");
	ASSERT_EQ(metrics_matches.size(), 1u);
	EXPECT_EQ(metrics_matches[0], "monitor_cpu");

	EXPECT_EQ(source.subscriber_count(), 0u);
	EXPECT_TRUE(source.empty());
	EXPECT_TRUE(source.match("metrics.cpu").empty());
	source.insert("auth.login", "client_auth");
	EXPECT_EQ(source.subscriber_count(), 1u);
	EXPECT_FALSE(source.match("auth.login").empty());
}



// ============================================================================
// Suite 2: Modifiers & Tree Pruning (PrefixTreeModifiersTest)
// ============================================================================


TEST(PrefixTreeModifiersTest, InsertMultipleSubscribersInSameNode) {
	PrefixTree tree;

	tree.insert("user.create", "client_0");
	tree.insert("user.create", "client_1");

	EXPECT_EQ(tree.subscriber_count(), 2u);
	EXPECT_FALSE(tree.empty());

	auto matches = tree.match("user.create");
	ASSERT_EQ(matches.size(), 2u);
	EXPECT_EQ(matches[0], "client_0");
	EXPECT_EQ(matches[1], "client_1");
}

TEST(PrefixTreeModifiersTest, InsertEmptyPrefixWildcard) {
	PrefixTree tree;

	tree.insert("", "wildcard_client");
	EXPECT_EQ(tree.subscriber_count(), 1u);
	EXPECT_FALSE(tree.empty());

	auto matches = tree.match("any.arbitrary.key");
	ASSERT_EQ(matches.size(), 1u);
	EXPECT_EQ(matches[0], "wildcard_client");
}

TEST(PrefixTreeModifiersTest, RemoveSubscribersAndPruneDeadNodes) {
	PrefixTree tree;

	tree.insert("auth.login.web", "client_web");
	tree.insert("auth.login.mobile", "client_mobile");

	EXPECT_EQ(tree.subscriber_count(), 2u);

	EXPECT_FALSE(tree.remove("auth.login.web", "unknown_client"));
	EXPECT_EQ(tree.subscriber_count(), 2u);

	EXPECT_FALSE(tree.remove("auth.non.existent.path", "client_web"));
	EXPECT_EQ(tree.subscriber_count(), 2u);

	EXPECT_TRUE(tree.remove("auth.login.web", "client_web"));
	EXPECT_EQ(tree.subscriber_count(), 1u);

	auto web_matches = tree.match("auth.login.web");
	EXPECT_TRUE(web_matches.empty());

	auto mobile_matches = tree.match("auth.login.mobile");
	ASSERT_EQ(mobile_matches.size(), 1u);
	EXPECT_EQ(mobile_matches[0], "client_mobile");

	EXPECT_TRUE(tree.remove("auth.login.mobile", "client_mobile"));
	EXPECT_EQ(tree.subscriber_count(), 0u);
	EXPECT_TRUE(tree.empty());
	EXPECT_TRUE(tree.match("auth.login.mobile").empty());
}

TEST(PrefixTreeModifiersTest, RemoveWildcardSubscriber) {
	PrefixTree tree;

	tree.insert("", "wildcard_client");
	EXPECT_EQ(tree.subscriber_count(), 1u);

	EXPECT_TRUE(tree.remove("", "wildcard_client"));
	EXPECT_EQ(tree.subscriber_count(), 0u);
	EXPECT_TRUE(tree.empty());
	EXPECT_TRUE(tree.match("random.event").empty());
}

TEST(PrefixTreeModifiersTest, ClearOperation) {
	PrefixTree tree;

	tree.insert("orders.created", "order_service");
	tree.insert("orders.cancelled", "audit_service");
	tree.insert("", "global_logger");

	EXPECT_EQ(tree.subscriber_count(), 3u);
	EXPECT_FALSE(tree.empty());

	tree.clear();

	EXPECT_EQ(tree.subscriber_count(), 0u);
	EXPECT_TRUE(tree.empty());
	EXPECT_TRUE(tree.match("orders.created").empty());
	EXPECT_TRUE(tree.match("orders.cancelled").empty());
	EXPECT_TRUE(tree.match("").empty());

	tree.insert("fresh.start", "new_consumer");
	EXPECT_EQ(tree.subscriber_count(), 1u);
	EXPECT_FALSE(tree.empty());

	auto matches = tree.match("fresh.start");
	ASSERT_EQ(matches.size(), 1u);
	EXPECT_EQ(matches[0], "new_consumer");
}



// ============================================================================
// Suite 3: Prefix Matching & Lookup Operations (PrefixTreeLookupTest)
// ============================================================================


TEST(PrefixTreeLookupTest, ExactMatch) {
	PrefixTree tree;
	tree.insert("user", "client_exact");

	auto matches = tree.match("user");
	ASSERT_EQ(matches.size(), 1u);
	EXPECT_EQ(matches[0], "client_exact");
}

TEST(PrefixTreeLookupTest, HierarchicalPrefixMatch) {
	PrefixTree tree;
	tree.insert("user", "client_root_user");

	auto matches_login = tree.match("user.login");
	ASSERT_EQ(matches_login.size(), 1u);
	EXPECT_EQ(matches_login[0], "client_root_user");

	auto matches_create = tree.match("user.create");
	ASSERT_EQ(matches_create.size(), 1u);
	EXPECT_EQ(matches_create[0], "client_root_user");
}

TEST(PrefixTreeLookupTest, MultiLevelPrefixAccumulation) {
	PrefixTree tree;
	tree.insert("order", "order_service");
	tree.insert("order.eu", "eu_service");
	tree.insert("order.eu.checkout", "checkout_service");

	auto matches = tree.match("order.eu.checkout");
	ASSERT_EQ(matches.size(), 3u);

	EXPECT_TRUE(std::find(matches.begin(), matches.end(), "order_service") != matches.end());
	EXPECT_TRUE(std::find(matches.begin(), matches.end(), "eu_service") != matches.end());
	EXPECT_TRUE(std::find(matches.begin(), matches.end(), "checkout_service") != matches.end());

	auto eu_matches = tree.match("order.eu.refund");
	ASSERT_EQ(eu_matches.size(), 2u);
	EXPECT_TRUE(std::find(eu_matches.begin(), eu_matches.end(), "order_service") != eu_matches.end());
	EXPECT_TRUE(std::find(eu_matches.begin(), eu_matches.end(), "eu_service") != eu_matches.end());
}

TEST(PrefixTreeLookupTest, NonMatchingKeysDoNotDispatch) {
	PrefixTree tree;
	tree.insert("user", "client_user");

	EXPECT_TRUE(tree.match("admin").empty());
	EXPECT_TRUE(tree.match("use").empty());
	EXPECT_TRUE(tree.match("orders").empty());
}

TEST(PrefixTreeLookupTest, RootWildcardMatchesAllKeys) {
	PrefixTree tree;
	tree.insert("", "wildcard_subscriber");
	tree.insert("auth.login", "auth_subscriber");

	auto matches_unrelated = tree.match("system.alert");
	ASSERT_EQ(matches_unrelated.size(), 1u);
	EXPECT_EQ(matches_unrelated[0], "wildcard_subscriber");

	auto matches_auth = tree.match("auth.login");
	ASSERT_EQ(matches_auth.size(), 2u);
	EXPECT_TRUE(std::find(matches_auth.begin(), matches_auth.end(), "wildcard_subscriber") != matches_auth.end());
	EXPECT_TRUE(std::find(matches_auth.begin(), matches_auth.end(), "auth_subscriber") != matches_auth.end());
}

TEST(PrefixTreeLookupTest, ConstCorrectnessMatch) {
	PrefixTree tree;
	tree.insert("sensor.temp", "temp_monitor");

	const PrefixTree& const_tree = tree;

	auto matches = const_tree.match("sensor.temp");
	ASSERT_EQ(matches.size(), 1u);
	EXPECT_EQ(matches[0], "temp_monitor");
	EXPECT_TRUE(const_tree.match("sensor.pressure").empty());
}
