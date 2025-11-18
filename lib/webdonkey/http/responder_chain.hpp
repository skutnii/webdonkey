/*
 * responder_chain.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HTTP_RESPONDER_CHAIN_HPP_
#define LIB_WEBDONKEY_HTTP_RESPONDER_CHAIN_HPP_

#include <webdonkey/defs.hpp>

#include <boost/asio/awaitable.hpp>
#include <webdonkey/http/request_context.hpp>

namespace webdonkey {

namespace http {

template <class... responders> class responder_chain;

template <class first_responder, class... other_responders>
class responder_chain<first_responder, other_responders...> {
public:
	responder_chain(std::shared_ptr<first_responder> first,
					std::shared_ptr<other_responders>... rest) :
		_first{first}, _rest{rest...} {}

	template <class socket_stream>
	asio::awaitable<bool> respond(request_context<socket_stream> &r_context,
								  std::string_view target) {
		co_return (_first->respond(r_context, target) ||
				   _rest.respond(r_context, target));
	}

private:
	std::shared_ptr<first_responder> _first;
	responder_chain<other_responders...> _rest;
};

template <> class responder_chain<> {
public:
	template <class socket_stream>
	asio::awaitable<bool> respond(request_context<socket_stream> &r_context,
								  std::string_view target) {
		co_return false;
	}
};

} // namespace http

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HTTP_RESPONDER_CHAIN_HPP_ */
