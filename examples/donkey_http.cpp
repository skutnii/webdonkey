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
#include <webdonkey/hop.hpp>
#include <webdonkey/http.hpp>
#include <webdonkey/static_responder.hpp>

struct server_context {};

using thread_pool = boost::asio::thread_pool;

class simple_server {
public:
	simple_server(const char *doc_root) :
		_respond{doc_root, "index.html", version()} {}

	static std::string version() { return "webdonkey HTTP example"; }

	webdonkey::coroutine::returning<void, std::suspend_never>
	serve(webdonkey::accept_result socket_or) {
		using namespace webdonkey;
		try {
			if (!socket_or.has_value()) {
				std::cerr << std::string{"Socket error: "} +
								 socket_or.error().message() + "\n";
				co_return;
			}

			auto next_request = http(*socket_or.value());

			// Possibly switch to a new thread
			co_await coroutine::hop(*_executor);
			while (co_await next_request) {
				if (!next_request->has_value()) {
					std::cerr << next_request->error().message() + "\n";
					continue;
				}

				request_context_ptr ctx = next_request->value();
				std::cout << "Serving " + ctx->method_string() + " " +
								 ctx->target() + "\n";
				expected_response response_or = _respond(*ctx, ctx->target());
				response_ptr response{nullptr};
				if (response_or.has_value())
					response = response_or.value();
				else {
					std::cerr
						<< "[HTTP error] " + response_or.error().message + "\n";
					beast::http::response<beast::http::string_body> res{
						response_or.error().status, ctx->request().version()};
					res.set(boost::beast::http::field::server, version());
					res.set(boost::beast::http::field::content_type,
							"text/html");
					res.keep_alive(ctx->request().keep_alive());
					res.body() = response_or.error().message;
					res.prepare_payload();
					response =
						std::make_shared<response_generator>(std::move(res));
				}

				std::cerr << "Writing response\n";
				auto status = co_await ctx->write(*response);
				std::cerr << "Response written\n";
				if (!status.has_value())
					std::cerr << status.error().message() + "\n";
			}
		} catch (boost::system::system_error &err) {
			if (err.code() == beast::http::error::end_of_stream)
				co_return;
			else {
				std::cerr << err.what() << std::endl;
				co_return;
			}
		} catch (std::exception &err) {
			std::cerr << err.what() << std::endl;
		} catch (...) {
			std::cerr << "Unknown error" << std::endl;
		}
	}

private:
	webdonkey::static_responder _respond;
	webdonkey::managed_ptr<server_context, thread_pool> _executor;
};

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

	auto srv = std::make_shared<simple_server>(argv[1]);
	auto const address = boost::asio::ip::make_address("0.0.0.0");
	boost::asio::ip::tcp::endpoint http_endpoint{address, 80};
	tcp_listener<server_context, thread_pool> http_listener{
		http_endpoint,
		[srv](accept_result socket_or)
			-> coroutine::returning<void, std::suspend_never> {
			return srv->serve(socket_or);
		}};

	shared_pool->join();

	return 0;
}
