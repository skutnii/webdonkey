/*
 * utils.hpp
 *
 *  Created on: Nov 21, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_UTILS_HPP_
#define LIB_WEBDONKEY_HTTP_UTILS_HPP_

#include <filesystem>
#include <regex>
#include <string>
#include <webdonkey/defs.hpp>

namespace webdonkey {

static std::string mime_type(const std::filesystem::path &file_path) {
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

static std::string_view prefix_matching(std::string_view path,
										const std::regex &regex) {
	std::cmatch matches;
	std::regex_search(path.begin(), path.end(), matches, regex);

	if (matches.empty() || (matches.prefix().length() > 0))
		return std::string_view{};

	return path.substr(0, matches[0].length());
}

class defer {
public:
	template <typename functor>
	defer(functor on_delete) :
		_on_delete(on_delete) {}

	~defer() { _on_delete(); }

private:
	std::function<void()> _on_delete;
};

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_UTILS_HPP_ */
