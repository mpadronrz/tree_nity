#include "PrefixTree.hpp"
#include <algorithm>

PrefixTree::PrefixTree(PrefixTree&& other) noexcept
	: root_(std::move(other.root_)),
	  subscriber_count_(other.subscriber_count_) {
	other.subscriber_count_ = 0;
}

PrefixTree& PrefixTree::operator=(PrefixTree&& other) noexcept {
	if (this != &other) {
		root_ = std::move(other.root_);
		subscriber_count_ = other.subscriber_count_;
		other.subscriber_count_ = 0;
	}
	return *this;
}

void	PrefixTree::insert(std::string_view prefix, const std::string& client_id) {
	Node*	current_node = &root_;
	for (char ch : prefix) {
		std::unique_ptr<PrefixTree::Node>&	child_node = current_node->children[ch];
		if (!child_node) {
			child_node = std::make_unique<Node>();
		}
		current_node = child_node.get();
	}
	current_node->subscribers.push_back(client_id);
	++subscriber_count_;
}

bool	PrefixTree::remove(std::string_view prefix, const std::string& client_id) {
	std::vector<std::pair<Node*, char>>	branch;
	branch.reserve(prefix.size());

	Node*	current_node = &root_;
	for (char ch : prefix) {
		std::unique_ptr<Node>*	child_ptr = current_node->children.find(ch);
		if (!child_ptr) {
			return false;
		}
		branch.emplace_back(current_node, ch);
		current_node = child_ptr->get();
	}

	auto it = std::find(current_node->subscribers.begin(), current_node->subscribers.end(), client_id);
	if (it == current_node->subscribers.end()) {
		return false;
	}
	current_node->subscribers.erase(it);
	--subscriber_count_;

	while (!branch.empty()) {
		if (!(current_node->subscribers.empty() && current_node->children.empty())) {
			return true;
		}
		auto [parent, ch] = branch.back();
		branch.pop_back();
		parent->children.erase(ch);
		current_node = parent;
	}
	return true;
}

std::vector<std::string>	PrefixTree::match(std::string_view message_key) const {
	std::vector<std::string>	result;

	result.insert(result.end(), root_.subscribers.begin(), root_.subscribers.end());
	const Node* current_node = &root_;
	for (char ch : message_key) {
		const std::unique_ptr<Node>*	child_ptr = current_node->children.find(ch);
		if (!child_ptr) {
			return result;
		}
		current_node = child_ptr->get();
		result.insert(result.end(), current_node->subscribers.begin(), current_node->subscribers.end());
	}
	return result;
}

void	PrefixTree::clear() noexcept {
	root_.children.clear();
	root_.subscribers.clear();
	subscriber_count_ = 0;
}
