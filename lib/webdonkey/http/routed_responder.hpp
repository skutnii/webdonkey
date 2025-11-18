/*
 * routed_responder.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_ROUTED_RESPONDER_HPP_
#define LIB_WEBDONKEY_HTTP_ROUTED_RESPONDER_HPP_

#include <boost/asio/awaitable.hpp>
#include <memory>
#include <regex>
#include <webdonkey/http/request_context.hpp>

namespace webdonkey {

namespace http {

template <class upstream> class routed_responder {
public:
	using upstream_ptr = std::shared_ptr<upstream>;

	routed_responder(const std::regex &route_regex, upstream_ptr main) :
		_route_regex{route_regex}, _upstream{main} {}

	std::string_view route_prefix(std::string_view path) {
		std::cmatch matches;
		std::regex_search(path.begin(), path.end(), matches, _route_regex);

		if (matches.empty() || (matches.prefix().length() > 0))
			return std::string_view{};

		return path.substr(0, matches[0].length());
	}

	template <class socket_stream>
	asio::awaitable<bool> respond(request_context<socket_stream> &r_context,
								  std::string_view path) {
		std::string_view route = route_prefix(path);
		if (route.data() == nullptr)
			co_return false;

		std::string_view resource = path.substr(route.length());
		co_return _upstream->respond(r_context, resource);
	}

private:
	upstream_ptr _upstream;
	std::regex _route_regex;
};

} // namespace http

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_ROUTED_RESPONDER_HPP_ */
