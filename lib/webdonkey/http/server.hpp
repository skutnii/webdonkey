/*
 * http_server.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_SERVER_HPP_
#define LIB_WEBDONKEY_HTTP_SERVER_HPP_

#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <stdexcept>
#include <webdonkey/defs.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http.hpp>
#include <webdonkey/contextual.hpp>
#include <webdonkey/http/error_handler.hpp>
#include <webdonkey/http/request_context.hpp>

namespace webdonkey {

namespace http {

template <class context, class responder, error_handler error_responder>
class server {
public:
	using responder_ptr = managed_ptr<context, responder>;
	using error_handler_ptr = managed_ptr<context, error_responder>;

	using error_response = error_handler_base::error_response;
	using error_response_ptr = error_handler_base::response_ptr;

	template <class io_stream> asio::awaitable<void> serve(io_stream &stream);

private:
	responder_ptr _responder;
	error_handler_ptr _error_handler;
};

//==============================================================================

template <class context, class responder, error_handler error_responder>
template <class socket_stream>
asio::awaitable<void>
server<context, responder, error_responder>::serve(socket_stream &stream) {
	for (;;) {
		try {
			request_context<socket_stream> r_context{stream};
			size_t header_length = co_await r_context.read_header();
			bool responded =
				co_await _responder->respond(r_context, r_context.target());

			if (!responded) {
				error_response_ptr not_found =
					_error_handler->response_for(r_context.request(), 404);
				if (nullptr == not_found)
					throw std::runtime_error{"Missing 404 response for " +
											 std::string{r_context.target()}};
				co_await r_context.write(*not_found);
			}

			if (!r_context.keep_alive())
				break;
		} catch (boost::system::system_error &err) {
			if (err.code() == beast::http::error::end_of_stream)
				break;
			else
				throw;
		}
	}
}

} // namespace http

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_SERVER_HPP_ */
