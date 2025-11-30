/*
 * producer.hpp
 *
 *  Created on: Nov 22, 2025
 *      Author: Sergii Kutnii
 */

#ifndef COROUTINES_HPP_
#define COROUTINES_HPP_

#include "continuation.hpp"
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace webdonkey {

namespace coroutine {

template <class suspend_type>
concept suspend = requires {
	std::is_same_v<suspend_type, std::suspend_always> ||
		std::is_same_v<suspend_type, std::suspend_never>;
};

/**
 * A coroutine that `co_yield`s a sequence of values
 * with void return type.
 */
template <typename yield_type, suspend init_suspend,
		  continuation_flavor flavor =
			  continuation_storage_type<std::optional<yield_type>>()>
class yielding {
public:
	using self = yielding<yield_type, init_suspend, flavor>;
	using result_type = std::optional<yield_type>;
	using yield_continuation = continuation<std::optional<yield_type>, flavor>;

	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type {
		yield_continuation _yield;
		bool _returned = false;
		std::recursive_mutex _mutex;

		self get_return_object() {
			return self{handle_type::from_promise(*this)};
		}

		init_suspend initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void unhandled_exception() {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			_yield(std::current_exception());
		} // saving
		  // exception

		std::suspend_always yield_value(yield_type &&from) {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			_yield(std::forward<yield_type>(from));
			return {};
		}

		std::suspend_always yield_value(const yield_type &from) {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			_yield(std::move(from));
			return {};
		}

		void return_void() {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			_returned = true;
			_yield(std::optional<yield_type>{});
		}
	};

	yielding(handle_type handle) :
		_handle{handle} {}

	promise_type &promise() { return _handle.promise(); }

	/**
	 * The return continuation's value type is std::optional<yield_type>.
	 * An empty optional indicates end of the sequence.
	 */
	yield_continuation &operator co_await() {
		std::unique_lock<std::recursive_mutex> state_lock{promise()._mutex};

		// Check if returned.
		if (!promise()._returned) {
			state_lock.unlock();
			_handle.resume();
		}

		return promise()._yield;
	}

private:
	handle_type _handle;
};

/**
 * A coroutine that returns a value with co_return but does not use co_yield.
 */
template <typename return_type, suspend init_suspend,
		  continuation_flavor flavor = continuation_storage_type<return_type>()>
class returning {
public:
	using self = returning<return_type, init_suspend, flavor>;
	using return_continuation = continuation<return_type, flavor>;

	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type // required
	{
		return_continuation _return;

		self get_return_object() {
			return self{handle_type::from_promise(*this)};
		}

		init_suspend initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void unhandled_exception() {
			_return(std::current_exception());
		} // saving
		  // exception

		void return_value(return_type &&from) {
			_return(std::forward<return_type>(from));
		}

		void return_value(const return_type &from) { _return(from); }
	};

	returning(handle_type handle) :
		_handle{handle} {}

	returning(const self &) = default;
	returning(self &&) = default;

	self &operator=(const self &) = default;
	self &operator=(self &&) = default;

	return_continuation &operator co_await() {
		_handle.resume();
		return _handle.promise()._return;
	}

	void operator()() { _handle.resume(); }

private:
	handle_type _handle;
};

/**
 * A void-returning coroutine..
 */
template <suspend init_suspend> class returning<void, init_suspend> {
public:
	using self = returning<void, init_suspend>;
	using return_continuation = continuation<void>;

	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type // required
	{
		return_continuation _return;

		self get_return_object() {
			return self{handle_type::from_promise(*this)};
		}

		init_suspend initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void unhandled_exception() {
			_return(std::current_exception());
		} // saving
		  // exception

		void return_void() { _return(); }
	};

	returning(handle_type handle) :
		_handle{handle} {}

	returning(const self &) = default;
	returning(self &&) = default;

	self &operator=(const self &) = default;
	self &operator=(self &&) = default;

	return_continuation &operator co_await() {
		_handle.resume();
		return _handle.promise()._return;
	}

	void operator()() { _handle.resume(); }

private:
	handle_type _handle;
};

class unhandled_yield : public std::runtime_error {
public:
	unhandled_yield() :
		std::runtime_error{
			"Unhandled yield while waiting for return from a coroutine."} {}
	virtual ~unhandled_yield() = default;
};

/**
 * A coroutine that can both yield and return.
 */
template <typename yield_type, typename return_type, suspend init_suspend,
		  continuation_flavor yield_flavor =
			  continuation_storage_type<std::optional<yield_type>>(),
		  continuation_flavor return_flavor =
			  continuation_storage_type<return_type>()>
class combined {
public:
	using self = combined<yield_type, return_type, init_suspend, yield_flavor,
						  return_flavor>;
	using yield_continuation =
		continuation<std::optional<yield_type>, yield_flavor>;
	using yield_result_type = std::optional<yield_type>;
	using return_continuation = continuation<return_type, return_flavor>;

	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type // required
	{
		yield_continuation _yield;
		return_continuation _return;
		bool _expects_return = false;
		bool _expects_yield = false;
		std::recursive_mutex _mutex;

		self get_return_object() {
			return self{handle_type::from_promise(*this)};
		}

		init_suspend initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }

		void unhandled_exception() {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			if (_expects_return)
				_return(std::current_exception());
			else
				_yield(std::current_exception());
		} // saving
		  // exception

		std::suspend_always yield_value(yield_result_type &&from) {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			if (_expects_return)
				_return(std::make_exception_ptr(unhandled_yield{}));

			_yield(std::forward<yield_result_type>(from));
			return {};
		}

		std::suspend_always yield_value(const yield_type &from) {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			if (_expects_return)
				_return(std::make_exception_ptr(unhandled_yield{}));

			_yield(std::move(from));
			return {};
		}

		void return_value(yield_type &&from) {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			if (_expects_yield)
				_yield(std::optional<yield_type>{});

			_return(std::forward<yield_type>(from));
		}

		void return_value(const return_type &from) {
			std::lock_guard<std::recursive_mutex> state_lock{_mutex};
			if (_expects_yield)
				_yield(std::optional<yield_type>{});

			_return(from);
		}
	};

	combined(handle_type handle) :
		_handle{handle} {}

	promise_type &promise() { return _handle.promise(); }

	yield_continuation &yield() {
		std::unique_lock<std::recursive_mutex> state_lock{promise().mutex};
		if (std::exception_ptr ex = promise()._return.exception())
			std::rethrow_exception(ex);

		if (!promise()._return.await_ready()) {
			promise()._expects_yield = true;
			state_lock.unlock();
			_handle.resume();
			state_lock.lock();
		}

		promise()._expects_yield = false;
		return promise()._yield;
	}

	return_continuation &operator co_await() {
		std::unique_lock<std::recursive_mutex> state_lock{promise().mutex};
		if (std::exception_ptr ex = promise()._yield.exception())
			std::rethrow_exception(ex);

		promise()._expects_return = true;
		state_lock.unlock();

		_handle.resume();

		state_lock.lock();
		promise()._expects_return = false;
		return promise()._return;
	}

private:
	handle_type _handle;
};

} // namespace coroutine

} // namespace webdonkey

#endif /* COROUTINES_HPP_ */
