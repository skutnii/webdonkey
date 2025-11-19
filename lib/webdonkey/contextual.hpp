/*
 * contextual.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_CONTEXTUAL_HPP_
#define LIB_WEBDONKEY_CONTEXTUAL_HPP_

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

namespace webdonkey {

template <class context, typename instance_type> class managed_ptr;
template <class context, typename instance_type> class shared_object;
template <class context, typename instance_type> class shared_factory;

template <class context, typename instance_type>
using managed_getter = std::function<managed_ptr<context, instance_type>()>;

template <class context> class shared_registry {
public:
	shared_registry(const shared_registry<context> &) = delete;
	shared_registry(shared_registry<context> &&) = delete;
	shared_registry<context> &
	operator=(const shared_registry<context> &) = delete;
	shared_registry<context> &operator=(shared_registry<context> &&) = delete;

	static shared_registry<context> &shared() {
		static shared_registry<context> obj;
		return obj;
	}

	template <typename instance_type> static std::type_index getter_id() {
		return std::type_index{typeid(instance_type)};
	}

	template <typename instance_type>
	void register_getter(const managed_getter<context, instance_type> &getter);

	template <typename instance_type>
	managed_ptr<context, instance_type> instance();

private:
	shared_registry() = default;

	std::unordered_map<std::type_index, std::any> _getters;
	std::mutex _getters_mutex;
};

template <class context, typename instance_type> class managed_ptr {
public:
	explicit managed_ptr(std::nullptr_t) {}
	explicit managed_ptr(std::shared_ptr<instance_type> local) :
		_getter{&managed_ptr<context, instance_type>::get_local},
		_local{local} {}

	explicit managed_ptr(std::weak_ptr<instance_type> shared) :
		_getter{&managed_ptr<context, instance_type>::get_shared},
		_shared{shared} {}

	managed_ptr(const managed_ptr<context, instance_type> &) = default;
	managed_ptr(managed_ptr<context, instance_type> &&) = default;

	managed_ptr<context, instance_type> &
	operator=(const managed_ptr<context, instance_type> &) = default;

	managed_ptr<context, instance_type> &
	operator=(managed_ptr<context, instance_type> &&) = default;

	managed_ptr() = default;

	instance_type *get() const { return (this->*_getter)(); }

	instance_type &operator*() const { return *get(); }

	instance_type *operator->() const { return get(); }

	operator bool() const { return (_local || _shared.lock()); }

private:
	instance_type *get_local() const { return _local.get(); }
	instance_type *get_shared() const { return _shared.lock().get(); }

	instance_type *get_lazy() const;

	instance_type *(managed_ptr<context, instance_type>::*_getter)() const =
		&managed_ptr<context, instance_type>::get_lazy;
	std::shared_ptr<instance_type> _local;
	std::weak_ptr<instance_type> _shared;
};

template <class context, typename instance_type> class shared_object {
public:
	using instance_ptr = std::shared_ptr<instance_type>;

	managed_getter<context, instance_type> getter() {
		std::weak_ptr<instance_type> weak_instance{_instance};
		return [weak_instance]() -> managed_ptr<context, instance_type> {
			return managed_ptr<context, instance_type>{weak_instance};
		};
	}

	explicit shared_object(instance_ptr instance) :
		_instance{instance} {
		shared_registry<context>::shared().register_getter(getter());
	}

private:
	std::shared_ptr<instance_type> _instance;
};

template <class context, typename instance_type> class shared_factory {
public:
	using instance_ptr = std::shared_ptr<instance_type>;

	managed_getter<context, instance_type> &getter() { return _getter; }

	template <typename factory>
	explicit shared_factory(factory f) :
		_getter{[f]() -> managed_ptr<context, instance_type> {
			instance_ptr instance = f();
			return managed_ptr<context, instance_type>{instance};
		}} {
		shared_registry<context>::shared().register_getter(getter());
	}

private:
	managed_getter<context, instance_type> _getter;
};

class duplicate_getter : public std::runtime_error {
public:
	duplicate_getter() :
		std::runtime_error{"Getter already registered."} {}

	duplicate_getter(const duplicate_getter &) = default;
	duplicate_getter(duplicate_getter &&) = default;
	duplicate_getter &operator=(const duplicate_getter &) = default;
	duplicate_getter &operator=(duplicate_getter &&) = default;

	virtual ~duplicate_getter() = default;
};

class missing_getter : public std::runtime_error {
public:
	missing_getter() :
		std::runtime_error{"Missing getter."} {}

	missing_getter(const missing_getter &) = default;
	missing_getter(missing_getter &&) = default;
	missing_getter &operator=(const missing_getter &) = default;
	missing_getter &operator=(missing_getter &&) = default;

	virtual ~missing_getter() = default;
};

class lazy_resolution_failure : public std::runtime_error {
public:
	lazy_resolution_failure() :
		std::runtime_error{"Lazy managed pointer resolution failed."} {}

	lazy_resolution_failure(const lazy_resolution_failure &) = default;
	lazy_resolution_failure(lazy_resolution_failure &&) = default;
	lazy_resolution_failure &
	operator=(const lazy_resolution_failure &) = default;
	lazy_resolution_failure &operator=(lazy_resolution_failure &&) = default;

	virtual ~lazy_resolution_failure() = default;
};

template <class context>
template <typename instance_type>
void shared_registry<context>::register_getter(
	const managed_getter<context, instance_type> &getter) {
	std::lock_guard<std::mutex> getters_lock{_getters_mutex};
	std::type_index id = getter_id<instance_type>();
	if (_getters.contains(id))
		throw duplicate_getter{};
	_getters[id] = getter;
}

template <class context>
template <typename instance_type>
managed_ptr<context, instance_type> shared_registry<context>::instance() {
	std::lock_guard<std::mutex> getters_lock{_getters_mutex};
	std::type_index id = getter_id<instance_type>();
	if (!_getters.contains(id))
		throw missing_getter{};
	return (*std::any_cast<managed_getter<context, instance_type>>(
		&_getters[id]))();
}

template <class context, typename instance_type>
instance_type *managed_ptr<context, instance_type>::get_lazy() const {
	const_cast<managed_ptr<context, instance_type> &>(*this) =
		shared_registry<context>::shared().template instance<instance_type>();

	// Prevent infinite recursion
	if (_getter == &managed_ptr<context, instance_type>::get_lazy)
		throw lazy_resolution_failure{};

	return (this->*_getter)();
}

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_CONTEXTUAL_HPP_ */
