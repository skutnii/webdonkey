/*
 * continuation.hpp
 *
 *  Created on: Nov 23, 2025
 *      Author: Sergii Kutnii
 */

#ifndef CONTINUATION_HPP_
#define CONTINUATION_HPP_

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>

namespace webdonkey {

namespace coroutine {

enum class value_storage { pointer, copy };

template <typename value_type>
inline static constexpr value_storage continuation_storage_type() {
	return value_storage::pointer;
}

template <> inline constexpr value_storage continuation_storage_type<int>() {
	return value_storage::copy;
}

template <> inline constexpr value_storage continuation_storage_type<bool>() {
	return value_storage::copy;
}

template <typename value_type, value_storage storage_strategy =
								   continuation_storage_type<value_type>()>
class continuation;

template <typename value_type>
class continuation<value_type, value_storage::pointer> {
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
		_state->_resume = [h]() { h.resume(); };
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
		if (_state->_resume) {
			_state->_short_lived_value = &val;
			swap_resume();
		} else {
			_state->_long_lived_value =
				std::make_unique<value_type>(std::move(val));
		}
	}

	void operator()(std::exception_ptr exception) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_exception = exception;
		if (_state->_resume)
			swap_resume();
	}

private:
	void swap_resume() {
		std::function<void()> resume;
		std::swap(resume, _state->_resume);
		resume();
	}

	struct state {
		const value_type *_short_lived_value = nullptr;
		std::unique_ptr<value_type> _long_lived_value;
		std::exception_ptr _exception;
		std::function<void()> _resume;
		std::function<void()> _suspend;
		std::recursive_mutex _access_mutex;
	};

	std::shared_ptr<state> _state = std::make_shared<state>();
};

template <typename value_type>
class continuation<value_type, value_storage::copy> {
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
		_state->_resume = [h]() { h.resume(); };
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
		if (_state->_resume)
			swap_resume();
	}

	void operator()(std::exception_ptr exception) {
		std::lock_guard<std::recursive_mutex> access_lock(
			_state->_access_mutex);
		_state->_exception = exception;
		if (_state->_resume)
			swap_resume();
	}

	std::exception_ptr exception() { return _state->_exception; }

private:
	void swap_resume() {
		std::function<void()> resume;
		std::swap(resume, _state->_resume);
		resume();
	}

	struct state {
		value_type _value;
		bool _ready = false;
		std::exception_ptr _exception;
		std::function<void()> _resume;
		std::function<void()> _suspend;
		std::recursive_mutex _access_mutex;
	};

	std::shared_ptr<state> _state = std::make_shared<state>();
};

template <> class continuation<void> {
public:
	bool await_ready() {
		std::lock_guard<std::recursive_mutex> access_lock(_access_mutex);
		return _ready || _exception;
	}

	template <typename caller_promise>
	void await_suspend(std::coroutine_handle<caller_promise> h) {
		std::lock_guard<std::recursive_mutex> access_lock(_access_mutex);
		_resume = [h]() { h.resume(); };
		if (_suspend)
			_suspend();
	}

	void await_resume() {
		std::lock_guard<std::recursive_mutex> access_lock(_access_mutex);
		if (_exception) {
			std::exception_ptr ex;
			std::swap(_exception, ex);
			std::rethrow_exception(ex);
		}

		_ready = false;
	}

	template <typename functor> void on_suspend(functor suspend) {
		std::lock_guard<std::recursive_mutex> access_lock(_access_mutex);
		_suspend = suspend;
	}

	void operator()() {
		std::lock_guard<std::recursive_mutex> access_lock(_access_mutex);
		if (_resume)
			swap_resume();
	}

	void operator()(std::exception_ptr exception) {
		std::lock_guard<std::recursive_mutex> access_lock(_access_mutex);
		_exception = exception;
		if (_resume)
			swap_resume();
	}

	std::exception_ptr exception() { return _exception; }

private:
	void swap_resume() {
		std::function<void()> resume;
		std::swap(resume, _resume);
		resume();
	}

	bool _ready = false;
	std::exception_ptr _exception;
	std::function<void()> _resume;
	std::function<void()> _suspend;
	std::recursive_mutex _access_mutex;
};

} // namespace coroutine

} // namespace webdonkey

#endif /* CONTINUATION_HPP_ */
