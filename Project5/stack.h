#ifndef STACK_H
#define STACK_H

#include <stack>
#include <map>
#include <memory>
#include <set>

namespace {
template<typename K, typename V>
class Storage {
private:
	const K key;
	size_t first_referenced = 0;
	std::stack<std::pair<V, size_t>> values;

public:
	Storage(const K &_key) : key(_key) {}

	Storage(const std::shared_ptr<Storage<K, V>> &pointer) 
		: key(pointer->key)
		, first_referenced(0)
		, values(pointer->values) {}

	void push(const V &value, size_t height) {
		values.push(std::pair(value, height));

		if (first_referenced > 0)
			first_referenced++;
	}

	void pop() {
		values.pop();

		if (first_referenced > 0)
			first_referenced--;
	}

	V & top() {
		V &top = values.top().first;
		
		if (first_referenced == 0)
			first_referenced = 1;

		return top;
	}

	V const & top() const {
		V const &top = values.top().first;
		return top;
	}

	K const & get_key() const {
		return key;
	}

	size_t size() const {
		return values.size();
	}

	bool empty() const {
		return size() == 0;
	}

	bool is_referenced() const {
		return first_referenced > 0;
	}

	size_t height() const {
		if (empty())
			return 0;
		return values.top().second;
	}
};

template<typename K, typename V>
class Inner_stack {
private:
	std::map<K, std::shared_ptr<Storage<K, V>>> storage_map;


	std::set<std::pair<size_t, std::shared_ptr<Storage<K, V>>>> storage_set;

	size_t referenced_storages = 0;
	size_t height = 0;

public:
	Inner_stack() = default;

	Inner_stack(Inner_stack const &inner_stack) {
		for (const auto &[key, value]: inner_stack.storage_map) {
			std::shared_ptr<Storage<K, V>> pointer = std::make_shared<Storage<K, V>>(value);
			storage_map[key] = pointer;
			storage_set.insert(std::pair(pointer->height(), pointer));
		}

		referenced_storages = 0;
		height = inner_stack.height;
	}


	void push(K const &key, V const &value) {
		height++;

		if (not storage_map.contains(key))
			storage_map[key] = std::make_shared<Storage<K, V>>(key);

		std::shared_ptr<Storage<K, V>> &pointer = storage_map[key];
		storage_set.erase(std::pair(pointer->height(), pointer));

		pointer->push(value, height);
		storage_set.insert(std::pair(pointer->height(), pointer));
	}

	void pop() {
		height--;

		const std::shared_ptr<Storage<K, V>> &pointer = prev(storage_set.end())->second;
		bool was_referenced = pointer->is_referenced();
		storage_set.erase(std::pair(pointer->height(), pointer));

		pointer->pop();
		bool is_referenced = pointer->is_referenced();
		storage_set.insert(std::pair(pointer->height(), pointer));

		if (was_referenced and (not is_referenced))
			referenced_storages--;
	}

	void pop(K const &key) {
		height--;

		const std::shared_ptr<Storage<K, V>> &pointer = storage_map[key];
		bool was_referenced = pointer->is_referenced();
		storage_set.erase(std::pair(pointer->height(), pointer));

		pointer->pop();
		bool is_referenced = pointer->is_referenced();
		storage_set.insert(std::pair(pointer->height(), pointer));

		if (was_referenced and (not is_referenced))
			referenced_storages--;
	}

	std::pair<K const &, V &> front() {
		const std::shared_ptr<Storage<K, V>> &pointer = prev(storage_set.end())->second;
		bool was_referenced = pointer->is_referenced();

		std::pair<K const &, V &> result = {pointer->get_key(), pointer->top()};

		if (not was_referenced)
			referenced_storages++;

		return result;
	}

	std::pair<K const &, V const &> front() const {

		const std::shared_ptr<Storage<K, V>> &pointer = prev(storage_set.end())->second;
		
		std::pair<K const &, V const &> result = {pointer->get_key(), pointer->top()};

		return result;
	}

	V & front(K const &key) {
		V &result;

		std::shared_ptr<Storage<K, V>> &pointer = storage_map[key];
		bool was_referenced = pointer->is_referenced();

		result = pointer->top();

		if (not was_referenced)
			referenced_storages++;

		return result;
	}

	V const & front(K const &key) const {
		V const &result;

		std::shared_ptr<Storage<K, V>> &pointer = storage_map[key];

		result = pointer->top();

		return result;
	}

	size_t size() const {
		return height;
	}

	size_t count(K const &key) const {
		auto it = storage_map.find(key);
		if (it == storage_map.end())
			return 0;
		return it->second->size();
	}

	void clear() {
		storage_map.clear();
		storage_set.clear();
		height = 0;
		referenced_storages = 0;
	}

	bool is_referenced() const {
		return referenced_storages > 0;
	}
};
}

namespace cxx {
template<typename K, typename V>
class stack {
private:
	std::shared_ptr<Inner_stack<K, V>> stack_pointer;

	void detach() {
		stack_pointer = std::make_shared<Inner_stack<K, V>>(*stack_pointer);
	}

public:
	stack() {
		stack_pointer = std::make_shared<Inner_stack<K, V>>();
	}

	stack(stack const &_stack) {
		stack_pointer = _stack.stack_pointer;
		if (_stack.stack_pointer->is_referenced())
			detach();
	}

	stack & operator=(stack const &_stack) {
		stack_pointer = _stack.stack_pointer;
		if (_stack.stack_pointer->is_referenced())
			detach();
		return *this;
	}

	void push(K const &key, V const &value) {
		if (not stack_pointer.unique())
			detach();

		stack_pointer->push(key, value);
	}
	
	void pop() {
		if (not stack_pointer.unique())
			detach();

		stack_pointer->pop();
	}

	void pop(K const &key) {
		if (not stack_pointer.unique())
			detach();

		stack_pointer->pop(key);
	}

	std::pair<K const &, V &> front() {
		if (not stack_pointer.unique())
			detach();

		return stack_pointer->front();
	}

	std::pair<K const &, V const &> front() const {
		return stack_pointer->front();
	}

	V & front(K const &key) {
		if (not stack_pointer.unique())
			detach();

		return stack_pointer->front(key);
	}

	V const & front(K const &key) const {
		return stack_pointer->front(key);
	}

	size_t size() const {
		return stack_pointer->size();
	}

	size_t count(K const &key) const {
		return stack_pointer->count(key);
	}

	void clear() {
		if (not stack_pointer.unique())
			detach();

		stack_pointer->clear();
	}
};
}

#endif
