/*
 * example_error_handler.cpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#include <webdonkey/defs.hpp>

#include "example_error_handler.hpp"
#include "webdonkey/http/request_context.hpp"

example_error_handler::example_error_handler() {
	using response = webdonkey::http::error_handler_base::error_response;
	using response_ptr = webdonkey::http::error_handler_base::response_ptr;

	set_default([](const webdonkey::http::request &rq,
				   int code) -> response_ptr {
		response_ptr res = std::make_shared<response>(
			boost::beast::http::int_to_status(code), rq.version());
		res->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res->set(boost::beast::http::field::content_type, "text/html");
		res->keep_alive(rq.keep_alive());
		//		res->body() = rq.target();
		res->prepare_payload();
		return res;
	});
}