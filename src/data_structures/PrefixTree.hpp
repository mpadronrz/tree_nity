#pragma once

#include "data_structures/HashMap.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <memory>

class	PrefixTree {
	public:
		PrefixTree() = default;
		~PrefixTree() = default;

		PrefixTree(const PrefixTree&) = delete;
		PrefixTree& operator=(const PrefixTree&) = delete;
		PrefixTree(PrefixTree&&) noexcept;
		PrefixTree& operator=(PrefixTree&&) noexcept;

		/**
		 * @brief Registers a consumer client under a specified key prefix.
		 *
		 * Traverses or builds the trie branch corresponding to @p prefix character-by-character
		 * and appends @p client_id to the subscriber list at the terminal node.
		 * If @p prefix is empty (""), the subscriber is registered at the root as a wildcard
		 * consumer matching all messages.
		 *
		 * @param prefix The key prefix string to subscribe to (e.g., "user" or "" for wildcard).
		 * @param client_id The unique identifier string of the subscribing consumer.
		 *
		 * @note Time complexity is O(|prefix|). Multiple subscribers may share the exact same prefix.
		 */
		void	insert(std::string_view prefix, const std::string& client_id);

		/**
		 * @brief Deregisters a consumer client from a specified key prefix.
		 *
		 * Locates the node corresponding to @p prefix, searches its subscriber list for
		 * @p client_id, and removes it. Prunes and deallocates any empty intermediate
		 * child nodes along the branch if they no longer hold subscribers or children.
		 *
		 * @param prefix The exact key prefix string under which the client was registered.
		 * @param client_id The unique identifier of the subscriber to remove.
		 * @return true if the subscriber was found and removed; false if the prefix branch
		 *         or client ID did not exist.
		 *
		 * @note Time complexity is O(|prefix|).
		 */
		bool	remove(std::string_view prefix, const std::string& client_id);

		/**
		 * @brief Retrieves all active subscribers whose registered prefixes match an incoming message key.
		 *
		 * Collects all wildcard consumers registered at the root node, then walks down
		 * the tree along each character of @p message_key, collecting subscribers stored
		 * at every matched ancestor node along the path. Traversal terminates early if a
		 * character branch does not exist.
		 *
		 * @param message_key The routing key of the published message (e.g., "user.login.success").
		 * @return std::vector<std::string> A list containing all matching subscriber client IDs.
		 *
		 * @note Time complexity is O(|message_key| + M), where |message_key| is the length of the
		 *       key and M is the number of matching subscribers. Traversal cost is independent
		 *       of total non-matching subscribers N.
		 */
		[[nodiscard]] std::vector<std::string>	match(std::string_view message_key) const;

		/**
		 * @brief Resets the prefix tree, removing all subscriber entries and child branches.
		 *
		 * Clears the root node's subscriber collection and destroys all child nodes
		 * recursively via smart pointer deallocation. Leaves the tree in an empty, ready state.
		 *
		 * @note This operation guarantees the no-throw exception guarantee (noexcept).
		 */
		void	clear() noexcept;

	[[nodiscard]] size_t subscriber_count() const noexcept { return subscriber_count_; }
	[[nodiscard]] bool empty() const noexcept { return subscriber_count_ == 0; }

	private:
		struct Node {
			std::vector<std::string>	subscribers;
			HashMap<char, std::unique_ptr<Node>>	children;
		};

		Node	root_;
		size_t	subscriber_count_{0};
};
