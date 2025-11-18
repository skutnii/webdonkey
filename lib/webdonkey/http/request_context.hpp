/*
 * request_context.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_REQUEST_CONTEXT_HPP_
#define LIB_WEBDONKEY_HTTP_REQUEST_CONTEXT_HPP_

#include <asm-generic/socket.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/parser_fwd.hpp>
#include <boost/beast/http/write.hpp>
#include <string_view>
#include <webdonkey/defs.hpp>

namespace webdonkey {

namespace http {

using request_buffer = beast::multi_buffer;
using request_parser = beast::http::request_parser<beast::http::buffer_body>;
using request = request_parser::value_type;

template <class socket_stream> class request_context {
public:
	request_context(socket_stream &s) :
		_stream{s} {};

	request_context(const request_context<socket_stream> &) = delete;
	request_context(request_context<socket_stream> &&) = delete;

	request_context<socket_stream> &
	operator=(const request_context<socket_stream> &) = delete;

	request_context<socket_stream> &
	operator=(request_context<socket_stream> &&) = delete;

	request_buffer &buffer() { return _buffer; }

	request_parser &parser() { return _parser; }

	socket_stream &stream() { return _stream; }

	asio::awaitable<std::size_t> read_header() {
		return beast::http::async_read_header(_stream, _buffer, _parser,
											  asio::use_awaitable);
	}

	template <class body>
	asio::awaitable<std::size_t> write(beast::http::response<body> &response) {
		return beast::http::async_write(_stream, response, asio::use_awaitable);
	}

	void force_keep_alive(bool flag) { _force_keep_alive = flag; }

	bool keep_alive() const {
		if (_force_keep_alive.has_value())
			return _force_keep_alive.value();

		return _parser.get().keep_alive();
	}

	http::request &request() { return _parser.get(); }

	std::string_view target() const { return _parser.get().base().target(); }

private:
	std::optional<bool> _force_keep_alive;
	socket_stream &_stream;
	request_buffer _buffer;
	request_parser _parser;
};

} // namespace http

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_REQUEST_CONTEXT_HPP_ */
