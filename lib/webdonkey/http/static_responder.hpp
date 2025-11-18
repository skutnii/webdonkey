/*
 * static_responder.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_STATIC_RESPONDER_HPP_
#define LIB_WEBDONKEY_HTTP_STATIC_RESPONDER_HPP_

#include <boost/asio/awaitable.hpp>
#include <filesystem>
#include <webdonkey/contextual.hpp>
#include <webdonkey/http/common_responder.hpp>
#include <webdonkey/http/error_handler.hpp>

namespace webdonkey {

namespace http {

template <class context, error_handler error_responder>
class static_responder : public common_responder<context, error_responder> {
public:
	static_responder(std::filesystem::path root) :
		_root{root} {}

	template <class socket_stream>
	asio::awaitable<bool> respond(request_context<socket_stream> &r_context,
								  std::string_view target);

private:
	std::filesystem::path _root;
};

template <class context, error_handler error_responder>
template <class socket_stream>
asio::awaitable<bool> static_responder<context, error_responder>::respond(
	request_context<socket_stream> &r_context, std::string_view target) {

	// Request path must be absolute and not contain "..".
	if (target.empty() || (target[0] != '/') ||
		(target.find("..") != std::string_view::npos))
		co_return co_await this->error(r_context, 400);

	std::filesystem::path file_path = _root / target.substr(1);
	if (!file_path.has_filename())
		file_path /= "index.html";

	request &req = r_context.request();

	// Make sure we can handle the method
	if (req.method() != beast::http::verb::get &&
		req.method() != beast::http::verb::head)
		co_return co_await this->error(r_context, 400);

	// Attempt to open the file
	beast::error_code ec;
	beast::http::file_body::value_type body;
	body.open(file_path.c_str(), beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if (ec == beast::errc::no_such_file_or_directory)
		co_return co_await this->error(r_context, 404);

	// Handle an unknown error
	if (ec)
		co_return co_await this->error(r_context, 500);

	// Cache the size since we need it after the move
	const std::size_t size = body.size();

	// Respond to HEAD request
	if (req.method() == beast::http::verb::head) {
		beast::http::response<beast::http::empty_body> res{
			beast::http::status::ok, req.version()};
		res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(beast::http::field::content_type, this->mime_type(file_path));
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		co_await r_context.write(res);
	} else {
		// Respond to GET request
		beast::http::response<beast::http::file_body> res{
			std::piecewise_construct, std::make_tuple(std::move(body)),
			std::make_tuple(beast::http::status::ok, req.version())};
		res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(beast::http::field::content_type, this->mime_type(file_path));
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		co_await r_context.write(res);
	}

	co_return true;
}

} // namespace http

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_STATIC_RESPONDER_HPP_ */
