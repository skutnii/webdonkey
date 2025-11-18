/*
 * contextual.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_CONTEXTUAL_HPP_
#define LIB_WEBDONKEY_CONTEXTUAL_HPP_

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <variant>

namespace webdonkey {

template <class derived> class context_base {
public:
	static derived &shared_ctx() {
		static derived instance{};
		return instance;
	}

	template <typename instance_type> static std::type_index instance_id() {
		return std::type_index{typeid(instance_type)};
	}

	template <typename instance_type> void assert_free_slot() {
		std::type_index id = instance_id<instance_type>();
		if (_getters.contains(id))
			throw std::runtime_error{"Instance getter already defined"};
	}

	template <typename instance_type> struct shared_instance {
		shared_instance(std::shared_ptr<instance_type> instance) {
			context_base<derived> &ctx =
				static_cast<context_base<derived> &>(shared_ctx());
			std::lock_guard<std::mutex> lock{ctx._getters_mutex};
			ctx.assert_free_slot<instance_type>();
			ctx._getters[instance_id<instance_type>()] = any_getter{
				std::static_pointer_cast<void, instance_type>(instance)};
		}
	};

	template <typename instance_type> struct instance_constructor {
		template <typename constructor> instance_constructor(constructor c) {
			context_base<derived> &ctx =
				static_cast<context_base<derived> &>(shared_ctx());
			std::lock_guard<std::mutex> lock{ctx._getters_mutex};
			ctx.assert_free_slot<instance_type>();
			ctx._getters[instance_id<instance_type>()] =
				any_getter{any_constructor{c}};
		}
	};

	template <typename instance_type> class managed_ptr {
	public:
		managed_ptr() {
			context_base<derived> &ctx =
				static_cast<context_base<derived> &>(shared_ctx());
			std::lock_guard<std::mutex> lock{ctx._getters_mutex};
			any_getter &getter = ctx.getter<instance_type>();
			if (std::holds_alternative<any_constructor>(getter))
				_local = std::static_pointer_cast<instance_type, void>(
					std::get<any_constructor>(getter)());
		}

		instance_type *get() const {
			if (_local)
				return _local.get();

			context_base<derived> &ctx =
				static_cast<context_base<derived> &>(shared_ctx());
			std::lock_guard<std::mutex> lock{ctx._getters_mutex};
			any_getter &getter = ctx.getter<instance_type>();
			if (std::holds_alternative<any_ptr>(getter))
				return static_cast<instance_type *>(
					std::get<any_ptr>(getter).get());

			return nullptr;
		}

		instance_type &operator*() const { return *get(); }

		instance_type *operator->() const { return get(); }

	private:
		std::shared_ptr<instance_type> _local;
	};

private:
	using any_ptr = std::shared_ptr<void>;
	using any_constructor = std::function<any_ptr()>;
	using any_getter = std::variant<any_ptr, any_constructor>;

	template <typename instance_type> any_getter &getter() {
		std::type_index id = instance_id<instance_type>();
		if (!_getters.contains(id))
			throw std::runtime_error{"Undefined instance getter"};

		return _getters[id];
	}

	std::unordered_map<std::type_index, any_getter> _getters;
	std::mutex _getters_mutex;
};

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_CONTEXTUAL_HPP_ */
