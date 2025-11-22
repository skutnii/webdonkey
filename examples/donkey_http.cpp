/*
 * donkey_http.cpp
 *
 *  Created on: Nov 15, 2025
 *      Author: Sergii Kutnii
 */

#include "webdonkey/defs.hpp"
#include "webdonkey/tcp_listener.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <exception>
#include <iostream>
#include <webdonkey/contextual.hpp>
#include <webdonkey/http.hpp>
#include <webdonkey/static_responder.hpp>

struct server_context {};

using thread_pool = boost::asio::thread_pool;

int main(int argc, char **argv) {

	using namespace webdonkey;

	// Check command line arguments.
	if (argc != 2) {
		std::cerr << "Usage: donkey_http <doc_root>" << std::endl
				  << "Example:" << std::endl
				  << "    donkey_http /path/to/htdocs" << std::endl;
		return EXIT_FAILURE;
	}

	shared_object<server_context, thread_pool> shared_pool{
		std::make_shared<thread_pool>(8)};

	std::filesystem::path doc_root{argv[1]};
	std::string version = "webdonkey HTTP example";

	static_responder serve_static{doc_root, "index.html", version};

	auto simple_server =
		[&](request_context<tcp_stream> &ctx) -> awaitable<response_ptr> {
		std::cout << "Serving " + ctx.method_string() + " " + ctx.target() +
						 "\n";
		expected_response response_or = serve_static(ctx, ctx.target());
		if (response_or.has_value())
			co_return response_or.value();

		std::cerr << "[HTTP error] " + response_or.error().message + "\n";
		beast::http::response<beast::http::string_body> res{
			response_or.error().status, ctx.request().version()};
		res.set(boost::beast::http::field::server, version);
		res.set(boost::beast::http::field::content_type, "text/html");
		res.keep_alive(ctx.request().keep_alive());
		res.body() = response_or.error().message;
		res.prepare_payload();
		co_return std::make_shared<response_generator>(std::move(res));
	};

	auto const address = boost::asio::ip::make_address("0.0.0.0");
	boost::asio::ip::tcp::endpoint http_endpoint{address, 80};

	tcp_listener<server_context, thread_pool> http_listener{
		http_endpoint, [&](tcp::socket &socket) -> awaitable<void> {
			try {
				co_await http(socket, simple_server);
			} catch (std::exception &err) {
				std::cerr << std::string{err.what()} + "\n";
			} catch (...) {
				std::cerr << "Unknown error occurred.\n";
			}
		}};

	shared_pool->join();

	return 0;
}
