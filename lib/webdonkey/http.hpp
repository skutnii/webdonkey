/*
 * http.hpp
 *
 *  Created on: Nov 20, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_HPP_
#define LIB_WEBDONKEY_HTTP_HPP_

#include "webdonkey/continuation.hpp"
#include "webdonkey/defs.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/system/detail/error_code.hpp>
#include <coroutine>
#include <expected>
#include <functional>
#include <regex>
#include <webdonkey/coroutines.hpp>
#include <webdonkey/utils.hpp>

namespace webdonkey {

using response_generator = beast::http::message_generator;
using request_buffer = beast::multi_buffer;
using request_parser = beast::http::request_parser<beast::http::buffer_body>;
using request = request_parser::value_type;
using response_ptr = std::shared_ptr<response_generator>;
using io_result = std::expected<size_t, boost::system::error_code>;

/**
 * HTTP request wrapper
 */
template <class socket_stream> class request_context {
public:
	using stream_ptr = std::shared_ptr<socket_stream>;

	request_context(socket_stream &s) :
		_stream{s} {};

	request_context(const request_context<socket_stream> &) = delete;
	request_context(request_context<socket_stream> &&) = delete;

	request_context<socket_stream> &
	operator=(const request_context<socket_stream> &) = delete;

	request_context<socket_stream> &
	operator=(request_context<socket_stream> &&) = delete;

	/**
	 * I/O buffer accessor
	 */
	request_buffer &buffer() { return _buffer; }

	/**
	 * Request parser accessor
	 */
	request_parser &parser() { return _parser; }

	/**
	 * Underlying stream accessor
	 */
	socket_stream &stream() { return _stream; }

	/**
	 * Const request accessor
	 */
	const webdonkey::request &request() const { return _parser.get(); }

	/**
	 * Non-const request accessor
	 */
	webdonkey::request &request() { return _parser.get(); }

	/**
	 * Requested resource path
	 */
	std::string_view target() const { return _parser.get().base().target(); }

	/**
	 * Request HTTP method as a string
	 */
	std::string method_string() const {
		return beast::http::to_string(request().method());
	}

	/**
	 * Reads the request header
	 */
	coroutine::continuation<io_result, coroutine::continuation_flavor::copy>
	read_header() {
		using continuation =
			coroutine::continuation<io_result,
									coroutine::continuation_flavor::copy>;
		continuation then;
		beast::http::async_read_header(
			_stream, _buffer, _parser,
			[then](
				boost::system::error_code const &error, // result of operation
				std::size_t bytes_transferred) {
				if (error)
					const_cast<continuation &>(then)(std::unexpected{error});
				else
					const_cast<continuation &>(then)(bytes_transferred);
			});
		return then;
	}

	/**
	 * Write a type-erased HTTP response.
	 */
	coroutine::continuation<io_result, coroutine::continuation_flavor::copy>
	write(response_generator &gen) {
		using continuation =
			coroutine::continuation<io_result,
									coroutine::continuation_flavor::copy>;
		continuation then;
		beast::async_write(
			_stream, std::move(gen),
			[then](
				boost::system::error_code const &error, // result of operation
				std::size_t bytes_transferred) {
				if (error)
					const_cast<continuation &>(then)(std::unexpected{error});
				else
					const_cast<continuation &>(then)(bytes_transferred);
			});
		return then;
	}

	/**
	 * Write a typed response
	 */
	template <class body>
	coroutine::continuation<io_result, coroutine::continuation_flavor::copy>
	write(beast::http::response<body> &res) {
		using continuation =
			coroutine::continuation<io_result,
									coroutine::continuation_flavor::copy>;
		continuation then;
		beast::http::async_write(
			_stream, res,
			[then](
				boost::system::error_code const &error, // result of operation
				std::size_t bytes_transferred) {
				if (error)
					const_cast<continuation &>(then)(std::unexpected{error});
				else
					const_cast<continuation &>(then)(bytes_transferred);
			});
		return then;
	}

	/**
	 * Force override the keep-alive value defined by the request.
	 */
	void force_keep_alive(bool flag) { _force_keep_alive = flag; }

	/**
	 * Effective keep-alive flag
	 */
	bool keep_alive() const {
		if (_force_keep_alive.has_value())
			return _force_keep_alive.value();

		return _parser.get().keep_alive();
	}

private:
	std::optional<bool> _force_keep_alive;
	socket_stream &_stream;
	request_buffer _buffer;
	request_parser _parser;
};

//==============================================================================

using http_context = request_context<tcp_stream>;
using https_context = request_context<ssl_stream>;

template <class socket_stream>
using context_wrapper = std::reference_wrapper<request_context<socket_stream>>;

template <class socket_stream>
using expected_request = std::expected<context_wrapper<socket_stream>,
									   boost::system::error_code>;

//==============================================================================

/**
 * Accept requests from a socket stream
 */
template <class socket_stream>
coroutine::yielding<expected_request<socket_stream>, 
										std::suspend_always, 
										coroutine::continuation_flavor::copy>
accept_requests(socket_stream &stream) {
	for (;;) {
		using context = request_context<socket_stream>;
		context ctx{stream};
		io_result status = co_await ctx.read_header();
		if (!status.has_value()) {
			if ((status.error() != beast::http::error::end_of_stream) &&
				(status.error() != beast::http::error::partial_message))
				co_yield std::unexpected{status.error()};

			break;
		}

		co_yield std::ref(ctx);

		if (!ctx.keep_alive())
			break;
	}
}

//==============================================================================

/**
 * HTTP over a TCP socket.
 */
inline static coroutine::yielding<expected_request<tcp_stream>,
								  std::suspend_always, 
									coroutine::continuation_flavor::copy>
http(tcp::socket socket) {
	tcp_stream stream{std::move(socket)};
	auto next_request = accept_requests(stream);

	while (auto request_or = co_await next_request)
		co_yield request_or.value();
}

//==============================================================================

/**
 * HTTPS over a TCP socket.
 */
inline static coroutine::yielding<expected_request<ssl_stream>,
								  								std::suspend_always, 
																	coroutine::continuation_flavor::copy>
https(tcp::socket socket, ssl::context &ssl_ctx) {
	ssl_stream stream{std::move(socket), ssl_ctx};
	stream.handshake(ssl::stream_base::server);
	defer shutdown{[&stream]() { stream.shutdown(); }};

	auto next_request = accept_requests(stream);

	while (auto request_or = co_await next_request)
		co_yield request_or.value();
}

//==============================================================================

/**
 * HTTP protocol error
 */
struct protocol_error {
	beast::http::status status;
	std::string message;

	/**
	 * If true, indicates that a recover attempt should be made by trying
	 * another responder.
	 */
	bool recoverable = true;
};

//==============================================================================

using expected_response = std::expected<response_generator, protocol_error>;

//==============================================================================

template <typename server_type, class socket_stream>
concept responder = requires {
	std::is_invocable_r_v<expected_response, server_type,
						  request_context<socket_stream>, std::string_view>;
};

//==============================================================================

template <class socket_stream, responder<socket_stream> upstream_responder>
std::function<expected_response(request_context<socket_stream> &,
								std::string_view)>
route(const std::regex &route_regex, upstream_responder upstream) {
	return [prefix_regex = route_regex,
			upstream](request_context<socket_stream> &ctx,
					  std::string_view target) -> expected_response {
		std::string_view prefix = prefix_matching(target, prefix_regex);
		if (prefix.data() == nullptr)
			return std::unexpected{
				protocol_error{beast::http::status::not_found, ""}};

		std::string_view resource = target.substr(prefix.length());
		return upstream(ctx, resource);
	};
}

//==============================================================================

template <class socket_stream, responder<socket_stream> first_responder,
		  responder<socket_stream> next_responder>
std::function<expected_response(request_context<socket_stream> &,
								std::string_view)>
operator|(first_responder first, next_responder next) {
	return [first, next](request_context<socket_stream> &ctx,
						 std::string_view target) -> expected_response {
		expected_response first_response = first(ctx, target);
		if (first_response.has_value())
			return first_response;

		if (first.response.error().recoverable)
			return next_responder(ctx, target);

		return std::unexpected{first_response.error()};
	};
}

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_HPP_ */
