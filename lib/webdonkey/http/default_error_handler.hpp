/*
 * default_error_responder.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_DEFAULT_ERROR_HANDLER_HPP_
#define LIB_WEBDONKEY_HTTP_DEFAULT_ERROR_HANDLER_HPP_

#include "webdonkey/http/request_context.hpp"
#include <functional>
#include <unordered_map>
#include <variant>
#include <webdonkey/http/error_handler.hpp>

namespace webdonkey {
namespace http {

class default_error_handler : public error_handler_base {
public:
	using response_ptr = error_handler_base::response_ptr;

	static int default_response_code() { return -1; }

	response_ptr default_response(const request &rq, int code) {
		if (!_handlers.contains(default_response_code()))
			return nullptr;

		return get_response(_handlers[default_response_code()], rq, code);
	}

	response_ptr response_for(const request &rq, int code) override {
		if (!_handlers.contains(code))
			return default_response(rq, code);

		return get_response(_handlers[code], rq, code);
	}

	void set(int code, response_ptr rsp) {
		if (nullptr == rsp)
			_handlers.erase(code);

		_handlers[code] = rsp;
	}

	template <typename constructor> void set(int code, constructor c) {
		_handlers[code] = generator{c};
	}

	void set_default(response_ptr rsp) { set(default_response_code(), rsp); }

	template <typename constructor> void set_default(constructor c) {
		set(default_response_code(), c);
	}

	virtual ~default_error_handler() = default;

private:
	using generator = std::function<response_ptr(const request &, int)>;
	using handler_store = std::variant<response_ptr, generator>;

	static response_ptr get_response(handler_store &store, const request &rq,
									 int code) {
		if (std::holds_alternative<response_ptr>(store))
			return std::get<response_ptr>(store);

		return std::get<generator>(store)(rq, code);
	}

	std::unordered_map<int, handler_store> _handlers;
};

} /* namespace http */
} /* namespace webdonkey */

#endif /* LIB_WEBDONKEY_HTTP_DEFAULT_ERROR_HANDLER_HPP_ */
