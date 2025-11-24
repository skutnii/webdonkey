/*
 * donkey_http.cpp
 *
 *  Created on: Nov 15, 2025
 *      Author: Sergii Kutnii
 */

#include "webdonkey/defs.hpp"
#include "webdonkey/tcp_listener.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/cobalt.hpp>
#include <boost/cobalt/spawn.hpp>
#include <boost/system/detail/error_code.hpp>
#include <coroutine>
#include <iostream>
#include <string>
#include <webdonkey/contextual.hpp>
#include <webdonkey/coroutines.hpp>
#include <webdonkey/http.hpp>
#include <webdonkey/static_responder.hpp>

struct server_context {};

using thread_pool = boost::asio::thread_pool;
namespace coroutine = webdonkey::coroutine;

class simple_server {
public:
	static std::string version() { return "webdonkey HTTP example"; }

	simple_server(std::filesystem::path doc_root) :
		_respond{doc_root, "index.html", version()} {}

	webdonkey::response_ptr
	response_for(webdonkey::request_context<webdonkey::tcp_stream> &ctx);

	boost::cobalt::task<void> serve(webdonkey::tcp::socket &socket) {
		using namespace webdonkey;
		auto next_req = http_request(socket);
		while (std::optional<expected_request<tcp_stream>> request_or =
				   co_await next_req) {
			if (!request_or->has_value()) {
				std::cerr << request_or->error().message() + "\n";
				continue;
			}

			request_context_ptr ctx = request_or->value();
			ctx->response = response_for(*ctx);
		}
	}

private:
	webdonkey::static_responder _respond;
};

//==============================================================================

webdonkey::response_ptr simple_server::response_for(
	webdonkey::request_context<webdonkey::tcp_stream> &ctx) {
	using namespace webdonkey;

	std::cout << "Serving " + ctx.method_string() + " " + ctx.target() + "\n";
	expected_response response_or = _respond(ctx, ctx.target());
	if (response_or.has_value())
		return response_or.value();

	std::cerr << "[HTTP error] " + response_or.error().message + "\n";
	beast::http::response<beast::http::string_body> res{
		response_or.error().status, ctx.request().version()};
	res.set(boost::beast::http::field::server, version());
	res.set(boost::beast::http::field::content_type, "text/html");
	res.keep_alive(ctx.request().keep_alive());
	res.body() = response_or.error().message;
	res.prepare_payload();
	return std::make_shared<response_generator>(std::move(res));
}

//==============================================================================

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
	simple_server server{doc_root};

	auto const address = boost::asio::ip::make_address("0.0.0.0");
	boost::asio::ip::tcp::endpoint http_endpoint{address, 80};

	tcp_listener<server_context, thread_pool> http_listener{http_endpoint};

	boost::cobalt::spawn(
		*shared_pool,
		[&]() -> boost::cobalt::task<void> {
			auto accept = http_listener.accept();
			while (auto result = co_await accept) {
				if (!result->has_value()) {
					std::cerr
						<< "Socket error: " + result->error().message() + "\n";
					co_return;
				}

				boost::cobalt::spawn(*shared_pool,
									 server.serve(*result->value()),
									 asio::detached);
				result->value().reset();
			}
		}(),
		asio::detached);

	shared_pool->join();

	return 0;
}
