/*
 * static_responder.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_STATIC_RESPONDER_HPP_
#define LIB_WEBDONKEY_HTTP_STATIC_RESPONDER_HPP_

#include <boost/asio/awaitable.hpp>
#include <boost/beast/http/status.hpp>
#include <expected>
#include <filesystem>
#include <string>
#include <webdonkey/contextual.hpp>
#include <webdonkey/http.hpp>
#include <webdonkey/utils.hpp>

namespace webdonkey {

class static_responder {
public:
	static_responder(const std::filesystem::path &root,
					 const std::string &index, const std::string &version) :
		_root{root}, _index{index}, _version{version} {}

	static_responder(const static_responder &) = default;
	static_responder(static_responder &&) = default;

	template <class socket_stream>
	expected_response operator()(request_context<socket_stream> &r_context,
								 std::string_view target);

private:
	std::filesystem::path _root;
	std::string _index;
	std::string _version;
};

template <class socket_stream>
expected_response
static_responder::operator()(request_context<socket_stream> &r_context,
							 std::string_view target) {
	// Request path must be absolute and not contain "..".
	if (target.find("..") != std::string_view::npos)
		return std::unexpected{
			protocol_error{beast::http::status::bad_request, "Bad request"}};

	std::string_view resource =
		(!target.empty() && target[0] == '/') ? target.substr(1) : target;
	std::filesystem::path file_path = _root / resource;
	if (!file_path.has_filename())
		file_path /= _index;

	request &req = r_context.request();

	// Make sure we can handle the method
	if (req.method() != beast::http::verb::get &&
		req.method() != beast::http::verb::head)
		return std::unexpected{protocol_error{
			beast::http::status::method_not_allowed,
			r_context.method_string() + " " + std::string{r_context.target()}}};

	// Attempt to open the file
	beast::error_code ec;
	beast::http::file_body::value_type body;
	body.open(file_path.c_str(), beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if (ec == beast::errc::no_such_file_or_directory)
		return std::unexpected{protocol_error{beast::http::status::not_found,
											  std::string{target}}};

	// Handle an unknown error
	if (ec)
		return std::unexpected{
			protocol_error{beast::http::status::bad_request, "Unknown error"}};

	// Cache the size since we need it after the move
	const std::size_t size = body.size();

	// Respond to HEAD request
	if (req.method() == beast::http::verb::head) {
		beast::http::response<beast::http::empty_body> res{
			beast::http::status::ok, req.version()};
		res.set(beast::http::field::server, _version);
		res.set(beast::http::field::content_type, mime_type(file_path));
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		return std::make_shared<response_generator>(std::move(res));
	} else {
		// Respond to GET request
		beast::http::response<beast::http::file_body> res{
			std::piecewise_construct, std::make_tuple(std::move(body)),
			std::make_tuple(beast::http::status::ok, req.version())};
		res.set(beast::http::field::server, _version);
		res.set(beast::http::field::content_type, mime_type(file_path));
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		return std::make_shared<response_generator>(std::move(res));
	}
}

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_STATIC_RESPONDER_HPP_ */
