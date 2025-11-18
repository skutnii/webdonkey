/*
 * http_listener.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_LISTENER_HPP_
#define LIB_WEBDONKEY_HTTP_LISTENER_HPP_

#include "webdonkey/http/server.hpp"
#include <webdonkey/tcp_listener.hpp>

namespace webdonkey {

template <class context, class main_responder,
		  http::error_handler error_responder, class executor>
class http_listener
	: public tcp_listener<
		  context,
		  http_listener<context, main_responder, error_responder, executor>,
		  executor>,
	  public http::server<context, main_responder, error_responder> {
public:
	using tcp_base = tcp_listener<
		context,
		http_listener<context, main_responder, error_responder, executor>,
		executor>;
	using executor_ptr = tcp_base::executor_ptr;

	using http_base = http::server<context, main_responder, error_responder>;
	using responder_ptr = http_base::responder_ptr;

	asio::awaitable<void> handle(tcp::socket &&socket);
};

//==============================================================================

template <class context, class responder, http::error_handler error_responder,
		  class executor>
asio::awaitable<void>
http_listener<context, responder, error_responder, executor>::handle(
	tcp::socket &&socket) {
	tcp_stream stream{std::forward<decltype(socket)>(socket)};
	try {
		co_await static_cast<
			http::server<context, responder, error_responder> *>(this)
			->serve(stream);
	} catch (std::exception &e) {
		this->fail(&e);
	} catch (...) {
		this->fail(nullptr);
	}
}

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_LISTENER_HPP_ */
