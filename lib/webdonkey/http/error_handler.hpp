/*
 * error_handler.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_ERROR_HANDLER_HPP_
#define LIB_WEBDONKEY_HTTP_ERROR_HANDLER_HPP_

#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <concepts>
#include <webdonkey/defs.hpp>
#include <webdonkey/http/request_context.hpp>

namespace webdonkey {

namespace http {

class error_handler_base {
public:
	using error_response = beast::http::response<beast::http::string_body>;
	using response_ptr = std::shared_ptr<error_response>;

	virtual response_ptr response_for(const request &rq, int code) = 0;
};

template <class concrete_handler>
concept error_handler =
	std::convertible_to<concrete_handler &, error_handler_base &> &&
	std::convertible_to<concrete_handler *, error_handler_base *>;

} // namespace http

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_ERROR_HANDLER_HPP_ */
