#ifndef STACK_H
#define STACK_H

#include <stack>
#include <stdexcept>
#include <map>
#include <memory>
#include <set>
#include <iostream>

namespace {
template<typename K, typename V>
class Element {
private:
	std::shared_ptr<K> key_ptr;
	std::shared_ptr<V> value_ptr;
	size_t height = 0;

public:
	Element (std::shared_ptr<K> _key_ptr, std::shared_ptr<V> _value_ptr, size_t _height)
		:key_ptr(_key_ptr), value_ptr(_value_ptr), height(_height) {}

	std::shared_ptr<K> key_pointer() const {
		return key_ptr;
	}

	K const & get_key() const {
		return *key_ptr;
	}

	V const & get_value() const {
		return *value_ptr;
	}
	
	V & get_value() {
		return *value_ptr;
	}
	
	std::shared_ptr<V> value_pointer() const {
		return value_ptr;
	}

	size_t get_height() const {
		return height;
	}
};

struct key_comparator {
	template<typename K>
	bool operator()(const std::shared_ptr<K> &a, const std::shared_ptr<K> &b) const {
		return *a < *b;
	}
};

struct element_height_comparator {
	template<typename K, typename V>
	bool operator()(const Element<K, V> &a, const Element<K, V> &b) const {
		return a.get_height() > b.get_height();
	}
};

template<typename K, typename V>
class Inner_stack {
private:
	std::set<std::shared_ptr<K>, key_comparator> keys;
	std::map<std::shared_ptr<K>, std::stack<Element<K, V>>, key_comparator> element_map;
	std::set<Element<K, V>, element_height_comparator> element_set;

	bool was_referenced = false;
	size_t height = 0;

public:
	Inner_stack() = default;

	Inner_stack(Inner_stack const &inner_stack) {
		clear();

		for (auto it = inner_stack.element_set.rbegin(); it != inner_stack.element_set.rend(); ++it)
			push(it->get_key(), it->get_value());

		was_referenced = false;
	}


	void push(K const &key, V const &value) {
		std::shared_ptr<K> key_pointer = std::make_shared<K>(key);
		if (keys.contains(key_pointer))
			key_pointer = *keys.find(key_pointer);
		else
			keys.insert(key_pointer);

		std::shared_ptr<V> value_pointer = std::make_shared<V>(value);

		Element<K, V> element(key_pointer, value_pointer, height);

		if (not element_map.contains(key_pointer))
			element_map[key_pointer] = std::stack<Element<K, V>>();

		element_map[key_pointer].push(element);
		element_set.insert(element);

		height++;
		was_referenced = true;
	}

	void pop() {
		Element<K, V> element = *element_set.begin();
		element_set.erase(element);
		
		std::shared_ptr<K> key_pointer = element.key_pointer();
		element_map[key_pointer].pop();

		height--;
		was_referenced = true;
	}

	void pop(K const &key) {
		std::shared_ptr<K> key_pointer = std::make_shared<K>(key);
		Element<K, V> element = element_map[key_pointer].top();

		element_map[key_pointer].pop();
		element_set.erase(element);

		height--;
		was_referenced = true;
	}

	std::pair<K const &, V &> front() {
		Element<K, V> element = *element_set.begin();

		std::pair<K const &, V &> result = {element.get_key(), element.get_value()};
		was_referenced = true;

		return result;
	}

	std::pair<K const &, V const &> front(const char *xd) const {
		Element<K, V> element = *element_set.begin();

		std::pair<K const &, V const &> result = {element.get_key(), element.get_value()};

		return result;
	}

	V & front(K const &key) {
		std::shared_ptr key_pointer = std::make_shared<K>(key);
		Element<K, V> element = element_map[key_pointer].top();

		was_referenced = true;

		return element.get_value();
	}

	V const & front(K const &key) const {
		std::shared_ptr key_pointer = std::make_shared<K>(key);
		Element<K, V> element = element_map[key].top();

		return element.get_value();
	}

	size_t size() const {
		return height;
	}

	size_t count(K const &key) const {
		std::shared_ptr key_pointer = std::make_shared<K>(key);
		auto it = element_map.find(key_pointer);

		if (it == element_map.end())
			return 0;
		return (it->second).size();
	}

	void clear() {
		keys.clear();
		element_map.clear();
		element_set.clear();
		was_referenced = false;
		height = 0;
	}

	bool is_referenced() const {
		return was_referenced;
	}

	const std::map<std::shared_ptr<K>, Element<K, V>, key_comparator> & get_element_map() const {
		return &element_map;
	}
};

template<typename K, typename V>
class const_stack_iterator {
private:
	using map_it = 
		typename std::map<std::shared_ptr<K>, Element<K, V>, key_comparator>::const_iterator;

	map_it map_iterator;

public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = const K;
	using pointer = const K*;
	using reference = const K&;

	const_stack_iterator(map_it it)
		: map_iterator(it) {}

	reference operator*() const {
		return *(map_iterator->first);
	}

	pointer operator->() const {
		return &(*(map_iterator->first));
	}

	const_stack_iterator& operator++() {
		map_iterator++;
		return *this;
	}

	const_stack_iterator operator++(int) {
		const_stack_iterator iterator = *this;
		++(*this);
		return iterator;
	}

	bool operator==(const const_stack_iterator &other) const {
		return map_iterator == other.map_iterator;
	}

	bool operator!=(const const_stack_iterator &other) const {
		return map_iterator != other.map_iterator;
	}
};
}

namespace cxx {
template<typename K, typename V>
class stack {
private:
	std::shared_ptr<Inner_stack<K, V>> stack_pointer;

	void detach() {
		std::cerr << "lolololo\n";
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

	stack(stack &&_stack) noexcept : 
	 	stack_pointer(std::move(_stack.stack_pointer)) {}

	~stack() noexcept = default;

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
		if (stack_pointer->size() == 0)
			throw std::invalid_argument("Stack is empty.");

		if (not stack_pointer.unique())
			detach();

		stack_pointer->pop();
	}

	void pop(K const &key) {
		if (stack_pointer->count(key) == 0)
			throw std::invalid_argument("Key not found in stack.");

		if (not stack_pointer.unique())
			detach();

		stack_pointer->pop(key);
	}

	std::pair<K const &, V &> front() {
		if (stack_pointer->size() == 0)
			throw std::invalid_argument("Stack is empty.");

		if (not stack_pointer.unique())
			detach();

		return stack_pointer->front();
	}

	std::pair<K const &, V const &> front() const {
		return static_cast<const std::shared_ptr<Inner_stack<K, V>>>(stack_pointer)->front("FIXME XD");
	}

	V & front(K const &key) {
		if (stack_pointer->count(key) == 0)
			throw std::invalid_argument("Key not found in stack.");

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

public:
	using const_iterator = const_stack_iterator<K, V>;

	const_iterator cbegin() const {
		return const_iterator((stack_pointer->get_element_map()).cbegin());
	}

	const_iterator cend() const {
		return const_iterator((stack_pointer->get_element_map()).cend());
	}
};
}

#endif
