// StE
// © Shlomi Steinberg, 2015-2016

#pragma once

#include <atomic>
#include <iostream>

#include "concurrent_pointer_recycler.hpp"

namespace StE {

template <typename DataType, bool recycle_pointers>
class shared_double_reference_guard {
private:
	class data {
	public:
		std::atomic<int> internal_counter;
		DataType object;

		template <typename ... Ts>
		data(Ts&&... args) : internal_counter(0), object(std::forward<Ts>(args)...) {}
		data& operator=(data &&d) {
			internal_counter.store(0);
			object = std::move(d.object);
			return *this;
		}

		void release_ref() {
			if (internal_counter.fetch_add(1, std::memory_order_acquire) == -1) {
				destroy();
			}
		}

	private:
		template <bool b, typename = void>
		class data_factory {
		public:
			void release(data *ptr) { delete ptr; }
			template <typename ... Ts>
			data* claim(Ts&&... args) { return new data(std::forward<Ts>(args)...); }
		};
		template <bool b>
		class data_factory<b, std::enable_if_t<b>> : public concurrent_pointer_recycler<data> {};

	public:
		static data_factory<std::is_move_assignable<DataType>::value && recycle_pointers> recycler;

		void destroy() {
			recycler.release(this);
		}
	};

	struct data_ptr {
		int external_counter;
		data *ptr;
	};

public:
	class data_guard {
		friend class shared_double_reference_guard<DataType, recycle_pointers>;

	private:
		data *ptr;

	public:
		data_guard(data *ptr) : ptr(ptr) { }
		data_guard(const data_guard &d) = delete;
		data_guard &operator=(const data_guard &d) = delete;
		data_guard(data_guard &&d) {
			ptr = d.ptr;
			d.ptr = 0; 
		}
		data_guard &operator=(data_guard &&d) {
			if (ptr) ptr->release_ref();
			ptr = d.ptr;
			d.ptr = 0; 
			return *this;
		}

		~data_guard() { if (ptr) ptr->release_ref(); }

		bool is_valid() const { return !!ptr; }

		DataType* operator->() { return &ptr->object; }
		DataType& operator*() { return ptr->object; }
		const DataType* operator->() const { return &ptr->object; }
		const DataType& operator*() const { return ptr->object; }
	};

private:
	std::atomic<data_ptr> guard;

	void release(data_ptr &old_data_ptr) {
		if (!old_data_ptr.ptr)
			return;
		auto external = old_data_ptr.external_counter;
		if (old_data_ptr.ptr->internal_counter.fetch_sub(external, std::memory_order_release) == external - 1)
			old_data_ptr.ptr->destroy();
		else
			old_data_ptr.ptr->release_ref();
	}

public:
	shared_double_reference_guard() {
		data_ptr new_data_ptr{ 0 };
		guard.store(new_data_ptr);
	}

	template <typename ... Ts>
	shared_double_reference_guard(Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		guard.store(new_data_ptr);

		assert(guard.is_lock_free() && "guard not lock free");
	}

	~shared_double_reference_guard() {
		data_ptr old_data_ptr = guard.load();
		release(old_data_ptr);
	}

	data_guard acquire(std::memory_order order = std::memory_order_acquire) {
		data_ptr new_data_ptr;
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		do {
			new_data_ptr = old_data_ptr;
			++new_data_ptr.external_counter;
		} while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed));

		return data_guard(new_data_ptr.ptr);
	}

	template <typename ... Ts>
	data_guard emplace_and_acquire(std::memory_order order, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 2, new_data };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed));

		release(old_data_ptr);

		return data_guard(new_data_ptr.ptr);
	}

	template <typename ... Ts>
	void emplace(std::memory_order order, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed));

		release(old_data_ptr);
	}

	template <typename ... Ts>
	bool try_emplace(std::memory_order order1, std::memory_order order2, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		data_ptr old_data_ptr = guard.load(order2);
		if (guard.compare_exchange_strong(old_data_ptr, new_data_ptr, order1, std::memory_order_relaxed)) {
			release(old_data_ptr);
			return true;
		}
		data::recycler.release(new_data);
		return false;
	}

	template <typename ... Ts>
	bool try_compare_emplace(std::memory_order order, data_guard &old_data, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);

		bool success = false;
		while (old_data_ptr.ptr == old_data.ptr && !(success = guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed)));
		if (success)
			release(old_data_ptr);
		else
			delete new_data;

		return success;
	}

	bool is_valid_hint(std::memory_order order = std::memory_order_relaxed) const {
		return !!guard.load(order).ptr;
	}

	void drop() {
		data_ptr new_data_ptr{ 0 };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, std::memory_order_acq_rel, std::memory_order_relaxed));

		release(old_data_ptr);
	}
};

template <typename DataType, bool recycle_pointers>
shared_double_reference_guard<DataType, recycle_pointers>::data::data_factory<std::is_move_assignable<DataType>::value && recycle_pointers> shared_double_reference_guard<DataType, recycle_pointers>::data::recycler;

}
