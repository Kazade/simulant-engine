// StE
// © Shlomi Steinberg, 2015-2016

#pragma once

#include <atomic>
#include <memory>

namespace StE {

/*
*	From Anthony William's "C++ Concurrency in Action"
*/

template <typename T>
class concurrent_queue {
private:
	struct node;
	struct counted_node_ptr;
	struct node_counter;

	struct counted_node_ptr {
		int ref_count;
		node *ptr;
	};

	struct node_counter {
		unsigned internal_count : 30;
		unsigned external_counters : 2;
	};

	struct node {
		std::atomic<node_counter> count;
		std::atomic<counted_node_ptr> next;

		std::atomic<T*> data;

		node() : data(nullptr) {
			node_counter new_counter;
			new_counter.internal_count = 0;
			new_counter.external_counters = 2;
			count.store(new_counter);

			counted_node_ptr new_counted_node_ptr{ 0 };
			next.store(new_counted_node_ptr);
		}

		void release_ref() {
			node_counter old_counter = count.load(std::memory_order_relaxed);
			node_counter new_counter;
			do {
				new_counter = old_counter;
				--new_counter.internal_count;
			} while (!count.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

			if (!new_counter.internal_count && !new_counter.external_counters)
				delete this;
		}
	};

protected:
	std::atomic<counted_node_ptr> head, tail;

	static void increase_external_count(std::atomic<counted_node_ptr> &counter, counted_node_ptr &old_counter) {
		counted_node_ptr new_counter;
		do {
			new_counter = old_counter;
			++new_counter.ref_count;
		} while (!counter.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

		old_counter.ref_count = new_counter.ref_count;
	}

	static void free_external_counter(counted_node_ptr &old_node_ptr) {
		node * const ptr = old_node_ptr.ptr;
		int count_increase = old_node_ptr.ref_count - 2;

		node_counter old_counter = ptr->count.load(std::memory_order_relaxed);
		node_counter new_counter;
		do {
			new_counter = old_counter;
			--new_counter.external_counters;
			new_counter.internal_count += count_increase;
		} while (!ptr->count.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

		if (!new_counter.internal_count && !new_counter.external_counters)
			delete ptr;
	}

	void set_new_tail(counted_node_ptr &old_tail, counted_node_ptr const &new_tail) {
		node * const current_tail_ptr = old_tail.ptr;
		while (!tail.compare_exchange_weak(old_tail, new_tail) && old_tail.ptr == current_tail_ptr);

		old_tail.ptr == current_tail_ptr ? 
			free_external_counter(old_tail) : 
			current_tail_ptr->release_ref();
	}

public:
	concurrent_queue() {
		counted_node_ptr new_counted_node;
		new_counted_node.ref_count = 1;
		new_counted_node.ptr = new node;

		head.store(new_counted_node);
		tail.store(new_counted_node);
		
		assert(head.is_lock_free() && "head/tail not lock free!");
		assert(new_counted_node.ptr->count.is_lock_free() && "count not lock free!");
		assert(new_counted_node.ptr->next.is_lock_free() && "next not lock free!");
	}
	~concurrent_queue() {
		while (pop() != nullptr);
		delete head.load().ptr;
	}

	concurrent_queue(const concurrent_queue &q) = delete;
	concurrent_queue& operator=(const concurrent_queue &q) = delete;

	std::unique_ptr<T> pop() {
		counted_node_ptr old_head = head.load(std::memory_order_relaxed);
		for (;;) {
			increase_external_count(head, old_head);
			node * const ptr = old_head.ptr;
			if (ptr == tail.load().ptr)
				return nullptr;

			counted_node_ptr next = ptr->next.load();
			if (head.compare_exchange_strong(old_head, next)) {
				T * const res = ptr->data.exchange(nullptr);
				free_external_counter(old_head);
				return std::unique_ptr<T>(res);
			}

			ptr->release_ref();
		}
	}

	void push(T &&new_value) {
		std::unique_ptr<T> new_data(new T(std::move(new_value)));
		counted_node_ptr new_next;
		new_next.ptr = new node;
		new_next.ref_count = 1;
		counted_node_ptr old_tail = tail.load();
		for (;;) {
			increase_external_count(tail, old_tail);
			T* old_data = nullptr;
			if (old_tail.ptr->data.compare_exchange_strong(old_data, new_data.get())) {
				counted_node_ptr old_next = { 0 };
				auto oldnexta = old_tail.ptr->next.load();
				if (!old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
					delete new_next.ptr;
					new_next = old_next;
				}
				set_new_tail(old_tail, new_next);
				new_data.release();
				break;
			}
			else {
				counted_node_ptr old_next = { 0 };
				if (old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
					old_next = new_next;
					new_next.ptr = new node;
				}
				set_new_tail(old_tail, old_next);
			}
		}
	}
};

}
