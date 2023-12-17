#ifndef STACK_H
#define STACK_H

#include <stack>
#include <stdexcept>
#include <map>
#include <memory>
#include <set>
#include <iostream>

template<typename K, typename V>
class Element {
private:
	std::shared_ptr<K> key_ptr;
	std::shared_ptr<V> value_ptr;
	size_t height = 0;

public:
	Element (std::shared_ptr<K> _key_ptr, 
			std::shared_ptr<V> _value_ptr, size_t _height)
		: key_ptr(_key_ptr), value_ptr(_value_ptr), height(_height) {}

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
	bool operator()(const std::shared_ptr<K> &a, 
					const std::shared_ptr<K> &b) const {

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
		Rollback_helper rollback(*this);

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

		rollback.set_commit();
	}

	void pop() {
		Rollback_helper rollback(*this);

		Element<K, V> element = *element_set.begin();
		element_set.erase(element);
		
		std::shared_ptr<K> key_pointer = element.key_pointer();
		element_map[key_pointer].pop();

		if (element_map[key_pointer].empty()) {
			keys.erase(key_pointer);
			element_map.erase(key_pointer);
		}

		height--;
		was_referenced = true;

		rollback.set_commit();
	}

	void pop(K const &key) {
		Rollback_helper rollback(*this);

		std::shared_ptr<K> key_pointer = std::make_shared<K>(key);
		Element<K, V> element = element_map[key_pointer].top();

		element_map[key_pointer].pop();
		element_set.erase(element);

		if (element_map[key_pointer].empty()) {
			keys.erase(key_pointer);
			element_map.erase(key_pointer);
		}

		height--;
		was_referenced = true;

		rollback.set_commit();
	}

	std::pair<K const &, V &> front() {
		Element<K, V> element = *element_set.begin();

		std::pair<K const &, V &> result = {element.get_key(), element.get_value()};
		was_referenced = true;

		return result;
	}

	std::pair<K const &, V const &> front() const {
		const Element<K, V> element = *element_set.begin();

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

	const std::set<std::shared_ptr<K>, key_comparator>& get_keys() const {
		return keys;
	}

private:
class Rollback_helper {
	private:
		Inner_stack<K, V>& stack;
		bool commit = false;

		std::set<std::shared_ptr<K>, key_comparator> old_keys;
		std::map<std::shared_ptr<K>, std::stack<Element<K, V>>, key_comparator> old_element_map;
		std::set<Element<K, V>, element_height_comparator> old_element_set;
		size_t old_height;

	public:
		Rollback_helper(Inner_stack<K, V>& stack) : stack(stack) {
			old_keys = stack.keys;
			old_element_map = stack.element_map;
			old_element_set = stack.element_set;
			old_height = stack.height;
	
		}

		~Rollback_helper() noexcept {
			if (!commit) {
				stack.keys = std::move(old_keys);
				stack.element_map = std::move(old_element_map);
				stack.element_set = std::move(old_element_set);
				stack.height = old_height;
			}
		}

		void set_commit() noexcept {
			commit = true;
		}
	};
};

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
		Detach_rollback_helper detach_helper(stack_pointer);

		if (not stack_pointer.unique())
			detach_helper.detach();

		try {
			stack_pointer->push(key, value);
			detach_helper.set_commit();
		} catch (...) {
			throw;
		}
	}
	
	void pop() {
		if (stack_pointer->size() == 0)
			throw std::invalid_argument("Stack is empty.");

		Detach_rollback_helper detach_helper(stack_pointer);

		if (not stack_pointer.unique())
			detach_helper.detach();

		try {
			stack_pointer->pop();
			detach_helper.set_commit();
		} catch (...) {
			throw;
		}

	}

	void pop(K const &key) {
		if (stack_pointer->count(key) == 0)
			throw std::invalid_argument("Key not found in stack.");

		Detach_rollback_helper detach_helper(stack_pointer);

		if (not stack_pointer.unique())
			detach_helper.detach();
		
		try {
			stack_pointer->pop(key);
			detach_helper.set_commit();
		} catch (...) {
			throw;
		}
	}

	std::pair<K const &, V &> front() {
		if (stack_pointer->size() == 0)
			throw std::invalid_argument("Stack is empty.");

		if (not stack_pointer.unique())
			detach();

		return stack_pointer->front();
	}

	std::pair<K const &, V const &> front() const {
		return stack_pointer->front();
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
	class const_iterator {
	private:
		using set_it = 
			typename std::set<std::shared_ptr<K>, key_comparator>
			::const_iterator;

		set_it set_iterator;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = const K;
		using difference_type = ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		const_iterator(set_it it)
			: set_iterator(it) {}

		reference operator*() const {
			return **set_iterator;
		}

		pointer operator->() const {
			return set_iterator->get;
		}

		const_iterator& operator++() {
			set_iterator++;
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator iterator = *this;
			++(*this);
			return iterator;
		}

		bool operator==(const const_iterator &other) const {
			return set_iterator == other.set_iterator;
		}

		bool operator!=(const const_iterator &other) const {
			return set_iterator != other.set_iterator;
		}
	};

	const_iterator cbegin() const {
		return const_iterator((stack_pointer->get_keys()).cbegin());
	}

	const_iterator cend() const {
		return const_iterator((stack_pointer->get_keys()).cend());
	}

private:
	class Detach_rollback_helper {
	private:
		std::shared_ptr<Inner_stack<K, V>>& st_pointer;
		std::shared_ptr<Inner_stack<K, V>> original_stack;
		bool commit = false;

	public:
		Detach_rollback_helper(std::shared_ptr<Inner_stack<K, V>>& sptr) 
			: st_pointer(sptr), original_stack(nullptr) {}

		void detach() {
			original_stack = st_pointer;
			st_pointer = std::make_shared<Inner_stack<K, V>>(*original_stack);
	
			
		}

		void set_commit() {
			commit = true;
		}

		~Detach_rollback_helper() noexcept {
			if (!commit) {
				st_pointer = original_stack;
			}
		}
	};
};
}

#endif