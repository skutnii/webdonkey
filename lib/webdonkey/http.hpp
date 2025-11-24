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
#include <boost/system/detail/error_code.hpp>
#include <coroutine>
#include <expected>
#include <regex>
#include <webdonkey/coroutines.hpp>
#include <webdonkey/utils.hpp>

namespace webdonkey {

using response_generator = beast::http::message_generator;
using request_buffer = beast::multi_buffer;
using request_parser = beast::http::request_parser<beast::http::buffer_body>;
using request = request_parser::value_type;
using response_ptr = std::shared_ptr<response_generator>;

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

	using io_result = std::expected<size_t, boost::system::error_code>;

	coroutine::continuation<io_result, coroutine::value_storage::copy>
	co_read_header() {
		using continuation =
			coroutine::continuation<io_result, coroutine::value_storage::copy>;
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

	awaitable<std::size_t> read_header() {
		return beast::http::async_read_header(_stream, _buffer, _parser,
											  asio::use_awaitable);
	}

	template <class body>
	asio::awaitable<std::size_t> write(beast::http::response<body> &response) {
		return beast::http::async_write(_stream, response, asio::use_awaitable);
	}

	awaitable<std::size_t> write(response_generator &gen) {
		return beast::async_write(_stream, std::move(gen), asio::use_awaitable);
	}

	coroutine::continuation<io_result, coroutine::value_storage::copy>
	co_write(response_generator &gen) {
		using continuation =
			coroutine::continuation<io_result, coroutine::value_storage::copy>;
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

	void force_keep_alive(bool flag) { _force_keep_alive = flag; }

	bool keep_alive() const {
		if (_force_keep_alive.has_value())
			return _force_keep_alive.value();

		return _parser.get().keep_alive();
	}

	const webdonkey::request &request() const { return _parser.get(); }

	webdonkey::request &request() { return _parser.get(); }

	std::string_view target() const { return _parser.get().base().target(); }

	std::string method_string() const {
		return beast::http::to_string(request().method());
	}

	response_ptr response;

private:
	std::optional<bool> _force_keep_alive;
	socket_stream &_stream;
	request_buffer _buffer;
	request_parser _parser;
};

template <typename responder_type, class socket_stream>
awaitable<void> serve(socket_stream &stream, responder_type respond) {
	request_context<socket_stream> ctx{std::forward<decltype(stream)>(stream)};
	for (;;) {
		try {
			request_context<socket_stream> ctx{
				std::forward<decltype(stream)>(stream)};
			co_await ctx.read_header();
			response_ptr response = co_await respond(ctx);

			/*
			 * Implementations may choose to write responses to the stream
			 * directly instead of returning them.
			 */
			if (response)
				co_await ctx.write(*response);

			if (!ctx.keep_alive())
				break;
		} catch (boost::system::system_error &err) {
			// Client hangup
			if (err.code() == beast::http::error::end_of_stream)
				break;
			else
				throw;
		}
	}
}

template <class socket_stream>
using request_context_ptr = std::shared_ptr<request_context<socket_stream>>;

template <class socket_stream>
using expected_request = std::expected<request_context_ptr<socket_stream>,
									   boost::system::error_code>;

template <class socket_stream>
coroutine::yielding<expected_request<socket_stream>, std::suspend_always,
					coroutine::value_storage::copy>
next_request(std::shared_ptr<socket_stream> stream) {
	for (;;) {
		using context = request_context<socket_stream>;
		request_context_ptr<socket_stream> ctx =
			std::make_shared<context>(*stream);
		typename context::io_result status = co_await ctx->co_read_header();
		if (!status.has_value()) {
			if ((status.error() != beast::http::error::end_of_stream) &&
				(status.error() != beast::http::error::partial_message))
				co_yield std::unexpected{status.error()};

			break;
		}

		co_yield ctx;

		if (!ctx->keep_alive())
			break;
	}
}

inline static coroutine::yielding<expected_request<tcp_stream>,
								  std::suspend_always,
								  coroutine::value_storage::copy>
http_request(tcp::socket &socket) {
	return next_request(std::make_shared<tcp_stream>(std::move(socket)));
}

inline static coroutine::yielding<expected_request<ssl_stream>,
								  std::suspend_always,
								  coroutine::value_storage::copy>
https_request(tcp::socket &socket, ssl::context &ssl_ctx) {
	std::shared_ptr<ssl_stream> stream =
		std::make_shared<ssl_stream>(std::move(socket), ssl_ctx);
	stream->handshake(ssl::stream_base::server);
	coroutine::yielding<expected_request<ssl_stream>, std::suspend_always,
						coroutine::value_storage::copy>
		next = next_request(stream);

	while (auto maybe_request = co_await next)
		co_yield maybe_request.value();

	stream->shutdown();
}

template <typename server_type>
awaitable<void> http(tcp::socket &socket, server_type server) {
	tcp_stream stream{std::move(socket)};
	co_await serve(stream, server);
}

template <typename server_type>
awaitable<void> https(tcp::socket &socket, ssl::context &ssl_ctx,
					  server_type server) {
	ssl_stream stream{std::move(socket), ssl_ctx};
	stream.handshake(ssl::stream_base::server);
	co_await serve(stream, server);
	stream.shutdown();
}

struct protocol_error {
	beast::http::status status;
	std::string message;

	/**
	 * If true, indicates that a recover attempt should be made by trying
	 * another responder.
	 */
	bool recoverable = true;
};

using expected_response = std::expected<response_ptr, protocol_error>;

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
