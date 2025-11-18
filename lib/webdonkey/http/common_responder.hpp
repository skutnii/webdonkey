/*
 * responder_mixin.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_COMMON_RESPONDER_HPP_
#define LIB_WEBDONKEY_HTTP_COMMON_RESPONDER_HPP_

#include <filesystem>
#include <webdonkey/contextual.hpp>
#include <webdonkey/http/error_handler.hpp>

namespace webdonkey {

namespace http {

template <class context, error_handler error_responder> class common_responder {
public:
	template <class socket_stream>
	asio::awaitable<bool> error(request_context<socket_stream> &r_context,
								int code);

	static std::string_view mime_type(const std::filesystem::path &file_path);

private:
	using error_handler_ptr =
		context_base<context>::template managed_ptr<error_responder>;

	error_handler_ptr _error_handler;
};

template <class context, error_handler error_responder>
std::string_view common_responder<context, error_responder>::mime_type(
	const std::filesystem::path &file_path) {
	using beast::iequals;
	std::filesystem::path ext = file_path.extension();
	if (iequals(ext.c_str(), ".htm"))
		return "text/html";
	if (iequals(ext.c_str(), ".html"))
		return "text/html";
	if (iequals(ext.c_str(), ".php"))
		return "text/html";
	if (iequals(ext.c_str(), ".css"))
		return "text/css";
	if (iequals(ext.c_str(), ".txt"))
		return "text/plain";
	if (iequals(ext.c_str(), ".js"))
		return "application/javascript";
	if (iequals(ext.c_str(), ".json"))
		return "application/json";
	if (iequals(ext.c_str(), ".xml"))
		return "application/xml";
	if (iequals(ext.c_str(), ".swf"))
		return "application/x-shockwave-flash";
	if (iequals(ext.c_str(), ".flv"))
		return "video/x-flv";
	if (iequals(ext.c_str(), ".png"))
		return "image/png";
	if (iequals(ext.c_str(), ".jpe"))
		return "image/jpeg";
	if (iequals(ext.c_str(), ".jpeg"))
		return "image/jpeg";
	if (iequals(ext.c_str(), ".jpg"))
		return "image/jpeg";
	if (iequals(ext.c_str(), ".gif"))
		return "image/gif";
	if (iequals(ext.c_str(), ".bmp"))
		return "image/bmp";
	if (iequals(ext.c_str(), ".ico"))
		return "image/vnd.microsoft.icon";
	if (iequals(ext.c_str(), ".tiff"))
		return "image/tiff";
	if (iequals(ext.c_str(), ".tif"))
		return "image/tiff";
	if (iequals(ext.c_str(), ".svg"))
		return "image/svg+xml";
	if (iequals(ext.c_str(), ".svgz"))
		return "image/svg+xml";
	return "application/text";
}

template <class context, error_handler error_responder>
template <class socket_stream>
asio::awaitable<bool> common_responder<context, error_responder>::error(
	request_context<socket_stream> &r_context, int code) {
	error_handler_base::response_ptr error_res =
		_error_handler->response_for(r_context.request(), code);
	if (!error_res)
		co_return false;

	co_await r_context.write(*error_res);
	co_return true;
}

} // namespace http

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_COMMON_RESPONDER_HPP_ */
