/*
 * donkey_http.cpp
 *
 *  Created on: Nov 15, 2025
 *      Author: Sergii Kutnii
 */

#include "server_context.hpp"
#include "webdonkey/http/error_handler.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <iostream>
#include <webdonkey/contextual.hpp>
#include <webdonkey/http/static_responder.hpp>
#include <webdonkey/http_listener.hpp>
#include <webdonkey/https_listener.hpp>

using thread_pool = boost::asio::thread_pool;
using io_context = boost::asio::io_context;

template <class context, typename instance_type>
using managed_ptr = webdonkey::managed_ptr<context, instance_type>;

int main(int argc, char **argv) {
	managed_ptr<server_context, logger> managed_logger;
	managed_logger->log("It works!");

	// Check command line arguments.
	if (argc != 2) {
		std::cerr << "Usage: donkey_http <doc_root>" << std::endl
				  << "Example:" << std::endl
				  << "    donkey_http /path/to/htdocs" << std::endl;
		return EXIT_FAILURE;
	}

	webdonkey::shared_object<server_context, thread_pool> shared_thread_pool{
		std::make_shared<thread_pool>(8)};

	std::filesystem::path doc_root{argv[1]};

	using responder =
		webdonkey::http::static_responder<server_context,
										  webdonkey::http::error_handler_base>;
	webdonkey::shared_object<server_context, responder> main_responder{
		std::make_shared<responder>(doc_root)};

	webdonkey::http_listener<server_context, responder,
							 webdonkey::http::error_handler_base, thread_pool>
		http_server;

	auto const address = boost::asio::ip::make_address("0.0.0.0");
	boost::asio::ip::tcp::endpoint http_endpoint{address, 80};

	http_server.bind(http_endpoint);

	managed_ptr<server_context, thread_pool> pool;
	pool->join();

	return 0;
}
