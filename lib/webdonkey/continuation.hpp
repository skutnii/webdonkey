/*
 * continuation.hpp
 *
 *  Created on: Nov 23, 2025
 *      Author: Sergii Kutnii
 */

#ifndef CONTINUATION_HPP_
#define CONTINUATION_HPP_

#include <condition_variable>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>

namespace webdonkey {

namespace coroutine {

enum class continuation_flavor { pointer, copy, blocking };

template <typename value_type>
inline static constexpr continuation_flavor continuation_storage_type() {
	return continuation_flavor::pointer;
}

template <>
inline constexpr continuation_flavor continuation_storage_type<int>() {
	return continuation_flavor::copy;
}

template <>
inline constexpr continuation_flavor continuation_storage_type<bool>() {
	return continuation_flavor::copy;
}

template <typename value_type, continuation_flavor storage_strategy =
								   continuation_storage_type<value_type>()>
class continuation;

template <typename value_type>
class continuation<value_type, continuation_flavor::pointer> {
public:
	bool await_ready() {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		return ((_state->_short_lived_value != nullptr) ||
				_state->_long_lived_value || _state->_exception);
	}

	template <typename caller_promise>
	void await_suspend(std::coroutine_handle<caller_promise> h) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		void *address = h.address();
		_state->_resume = [address]() {
			std::coroutine_handle<caller_promise>::from_address(address)
				.resume();
		};

		if (_state->_suspend)
			_state->_suspend();
	}

	value_type await_resume() {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		if (_state->_exception) {
			std::exception_ptr ex;
			std::swap(_state->_exception, ex);
			std::rethrow_exception(ex);
		}

		if (_state->_short_lived_value) {
			const value_type *val = _state->_short_lived_value;
			_state->_short_lived_value = nullptr;
			return *val;
		}

		std::unique_ptr<value_type> tmp;
		std::swap(tmp, _state->_long_lived_value);
		return *tmp;
	}

	template <typename functor> void on_suspend(functor suspend) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_suspend = suspend;
	}

	void operator()(const value_type &val) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		if (_state->resumable()) {
			_state->_short_lived_value = &val;
			pop_resume();
		} else {
			_state->_long_lived_value =
				std::make_unique<value_type>(std::move(val));
		}
	}

	void operator()(std::exception_ptr exception) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_exception = exception;
		if (_state->resumable())
			pop_resume();
	}

private:
	void pop_resume() {
		std::function<void()> resume = _state->_resume;
		_state->_resume = nullptr;
		resume();
	}

	struct state {
		const value_type *_short_lived_value = nullptr;
		std::unique_ptr<value_type> _long_lived_value;
		std::exception_ptr _exception;
		std::function<void()> _resume;
		std::function<void()> _suspend;
		std::recursive_mutex _access_mutex;

		bool resumable() const { return (_resume != nullptr); }
	};

	std::shared_ptr<state> _state{std::make_shared<state>()};
};

template <typename value_type>
class continuation<value_type, continuation_flavor::copy> {
public:
	bool await_ready() {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		return _state->_ready || _state->_exception;
	}

	template <typename caller_promise>
	void await_suspend(std::coroutine_handle<caller_promise> h) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		void *address = h.address();
		_state->_resume = [address]() {
			std::coroutine_handle<caller_promise>::from_address(address)
				.resume();
		};

		if (_state->_suspend)
			_state->_suspend();
	}

	value_type await_resume() {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		if (_state->_exception) {
			std::exception_ptr ex;
			std::swap(_state->_exception, ex);
			std::rethrow_exception(ex);
		}

		_state->_ready = false;
		return _state->_value;
	}

	template <typename functor> void on_suspend(functor suspend) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_suspend = suspend;
	}

	void operator()(const value_type &val) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_value = val;
		if (_state->resumable())
			pop_resume();
	}

	void operator()(std::exception_ptr exception) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_exception = exception;
		if (_state->resumable())
			pop_resume();
	}

	std::exception_ptr exception() { return _state->_exception; }

private:
	void pop_resume() {
		std::function<void()> resume = _state->_resume;
		_state->_resume = nullptr;
		resume();
	}

	struct state {
		bool _ready = false;
		value_type _value;
		std::exception_ptr _exception;
		std::function<void()> _resume;
		std::function<void()> _suspend;
		std::recursive_mutex _access_mutex;

		bool resumable() const { return (_resume != nullptr); }
	};

	std::shared_ptr<state> _state{std::make_shared<state>()};
};

template <> class continuation<void> {
public:
	bool await_ready() {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		return _state->_ready || _state->_exception;
	}

	template <typename caller_promise>
	void await_suspend(std::coroutine_handle<caller_promise> h) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		void *address = h.address();
		_state->_resume = [address]() {
			std::coroutine_handle<caller_promise>::from_address(address)
				.resume();
		};

		if (_state->_suspend)
			_state->_suspend();
	}

	void await_resume() {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		if (_state->_exception) {
			std::exception_ptr ex;
			std::swap(_state->_exception, ex);
			std::rethrow_exception(ex);
		}

		_state->_ready = false;
	}

	template <typename functor> void on_suspend(functor suspend) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_suspend = suspend;
	}

	void operator()() {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		if (_state->resumable())
			pop_resume();
	}

	void operator()(std::exception_ptr exception) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_exception = exception;
		if (_state->resumable())
			pop_resume();
	}

	std::exception_ptr exception() { return _state->_exception; }

private:
	void pop_resume() {
		std::function<void()> resume = _state->_resume;
		_state->_resume = nullptr;
		resume();
	}

	struct state {
		bool _ready = false;
		std::exception_ptr _exception;
		std::function<void()> _resume;
		std::function<void()> _suspend;
		std::recursive_mutex _access_mutex;

		bool resumable() const { return (_resume != nullptr); }
	};

	std::shared_ptr<state> _state = std::make_shared<state>();
};

template <typename value_type>
class continuation<value_type, continuation_flavor::blocking> {
public:
	bool await_ready() { return false; }

	template <typename caller_promise>
	void await_suspend(std::coroutine_handle<caller_promise> h) {
		{
			std::lock_guard<std::recursive_mutex> access_lock(
				_state->_access_mutex);
			void *address = h.address();
			_state->_resume = [address]() {
				std::coroutine_handle<caller_promise>::from_address(address)
					.resume();
			};

			if (_state->_suspend)
				_state->_suspend();
		}

		_state->_await_guard.notify_all();
	}

	value_type await_resume() {
		if (_state->_exception) {
			std::exception_ptr ex;
			std::swap(_state->_exception, ex);
			std::rethrow_exception(ex);
		}

		const value_type *val = _state->_value;
		_state->_value = nullptr;
		return *val;
	}

	template <typename functor> void on_suspend(functor suspend) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_suspend = suspend;
	}

	void operator()(const value_type &val) {
		std::unique_lock<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_await_guard.wait(access_lock,
								  [this]() { _state->resumable(); });
		_state->_value = &val;
		pop_resume();
	}

	void operator()(std::exception_ptr exception) {
		std::unique_lock<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_await_guard.wait(access_lock,
								  [this]() { _state->resumable(); });
		_state->_exception = exception;
		pop_resume();
	}

private:
	void pop_resume() {
		std::function<void()> resume = _state->_resume;
		_state->_resume = nullptr;
		resume();
	}

	struct state {
		const value_type *_value = nullptr;
		std::condition_variable _await_guard;
		std::exception_ptr _exception;
		std::function<void()> _resume;
		std::function<void()> _suspend;
		std::recursive_mutex _access_mutex;

		bool resumable() const { return (_resume != nullptr); }
	};

	std::shared_ptr<state> _state{std::make_shared<state>()};
};

template <> class continuation<void, continuation_flavor::blocking> {
public:
	bool await_ready() { return false; }

	template <typename caller_promise>
	void await_suspend(std::coroutine_handle<caller_promise> h) {
		{
			std::lock_guard<std::mutex> access_lock(_state->_access_mutex);
			void *address = h.address();
			_state->_resume = [address]() {
				std::coroutine_handle<caller_promise>::from_address(address)
					.resume();
			};

			if (_state->_suspend)
				_state->_suspend();
		}

		_state->_await_guard.notify_all();
	}

	void await_resume() {
		if (_state->_exception) {
			std::exception_ptr ex;
			std::swap(_state->_exception, ex);
			std::rethrow_exception(ex);
		}
	}

	template <typename functor> void on_suspend(functor suspend) {
		std::lock_guard<std::mutex> access_lock(_state->_access_mutex);
		_state->_suspend = suspend;
	}

	void operator()() {
		std::unique_lock<std::mutex> access_lock(_state->_access_mutex);
		_state->_await_guard.wait(access_lock,
								  [this]() { return _state->resumable(); });
		pop_resume();
	}

	void operator()(std::exception_ptr exception) {
		std::unique_lock<std::mutex> access_lock(_state->_access_mutex);
		_state->_await_guard.wait(access_lock,
								  [this]() { return _state->resumable(); });
		_state->_exception = exception;
		pop_resume();
	}

private:
	void pop_resume() {
		std::function<void()> resume = _state->_resume;
		_state->_resume = nullptr;
		resume();
	}

	struct state {
		std::condition_variable _await_guard;
		std::exception_ptr _exception;
		std::function<void()> _resume;
		std::function<void()> _suspend;
		std::mutex _access_mutex;

		bool resumable() const { return (_resume != nullptr); }
	};

	std::shared_ptr<state> _state{std::make_shared<state>()};
};

} // namespace coroutine

} // namespace webdonkey

#endif /* CONTINUATION_HPP_ */
