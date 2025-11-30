/*
 * donkey_https.cpp
 *
 *  Created on: Nov 21, 2025
 *      Author: Sergii Kutnii
 */

#include "webdonkey/coroutines.hpp"
#include "webdonkey/defs.hpp"
#include "webdonkey/tcp_listener.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/cobalt.hpp>
#include <boost/cobalt/spawn.hpp>
#include <coroutine>
#include <iostream>
#include <sstream>
#include <webdonkey/contextual.hpp>
#include <webdonkey/http.hpp>
#include <webdonkey/static_responder.hpp>

struct server_context {};

using thread_pool = boost::asio::thread_pool;

class secure_server {
public:
	secure_server(const char *doc_root);

	static std::string version() { return "webdonkey HTTP example"; }

	webdonkey::coroutine::returning<void, std::suspend_always>
	serve_content(webdonkey::accept_result socket_or);

	webdonkey::coroutine::returning<void, std::suspend_always>
	redirect(webdonkey::accept_result socket_or);

private:
	webdonkey::ssl::context _ssl_ctx;
	webdonkey::static_responder _respond;
};

//==============================================================================

secure_server::secure_server(const char *doc_root) :
	_ssl_ctx(webdonkey::ssl::context::tlsv12),
	_respond{doc_root, "index.html", version()} {
	// Load certificates

	/*
		The certificate was generated from CMD.EXE on Windows 10 using:

		winpty openssl dhparam -out dh.pem 2048
		winpty openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days
	   10000 -out cert.pem -subj "//C=US\ST=CA\L=Los
	   Angeles\O=Beast\CN=www.example.com"
	*/

	std::string const cert =
		"-----BEGIN CERTIFICATE-----\n"
		"MIIDaDCCAlCgAwIBAgIJAO8vBu8i8exWMA0GCSqGSIb3DQEBCwUAMEkxCzAJBgNV\n"
		"BAYTAlVTMQswCQYDVQQIDAJDQTEtMCsGA1UEBwwkTG9zIEFuZ2VsZXNPPUJlYXN0\n"
		"Q049d3d3LmV4YW1wbGUuY29tMB4XDTE3MDUwMzE4MzkxMloXDTQ0MDkxODE4Mzkx\n"
		"MlowSTELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMS0wKwYDVQQHDCRMb3MgQW5n\n"
		"ZWxlc089QmVhc3RDTj13d3cuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\n"
		"A4IBDwAwggEKAoIBAQDJ7BRKFO8fqmsEXw8v9YOVXyrQVsVbjSSGEs4Vzs4cJgcF\n"
		"xqGitbnLIrOgiJpRAPLy5MNcAXE1strVGfdEf7xMYSZ/4wOrxUyVw/Ltgsft8m7b\n"
		"Fu8TsCzO6XrxpnVtWk506YZ7ToTa5UjHfBi2+pWTxbpN12UhiZNUcrRsqTFW+6fO\n"
		"9d7xm5wlaZG8cMdg0cO1bhkz45JSl3wWKIES7t3EfKePZbNlQ5hPy7Pd5JTmdGBp\n"
		"yY8anC8u4LPbmgW0/U31PH0rRVfGcBbZsAoQw5Tc5dnb6N2GEIbq3ehSfdDHGnrv\n"
		"enu2tOK9Qx6GEzXh3sekZkxcgh+NlIxCNxu//Dk9AgMBAAGjUzBRMB0GA1UdDgQW\n"
		"BBTZh0N9Ne1OD7GBGJYz4PNESHuXezAfBgNVHSMEGDAWgBTZh0N9Ne1OD7GBGJYz\n"
		"4PNESHuXezAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCmTJVT\n"
		"LH5Cru1vXtzb3N9dyolcVH82xFVwPewArchgq+CEkajOU9bnzCqvhM4CryBb4cUs\n"
		"gqXWp85hAh55uBOqXb2yyESEleMCJEiVTwm/m26FdONvEGptsiCmF5Gxi0YRtn8N\n"
		"V+KhrQaAyLrLdPYI7TrwAOisq2I1cD0mt+xgwuv/654Rl3IhOMx+fKWKJ9qLAiaE\n"
		"fQyshjlPP9mYVxWOxqctUdQ8UnsUKKGEUcVrA08i1OAnVKlPFjKBvk+r7jpsTPcr\n"
		"9pWXTO9JrYMML7d+XRSZA1n3856OqZDX4403+9FnXCvfcLZLLKTBvwwFgEFGpzjK\n"
		"UEVbkhd5qstF6qWK\n"
		"-----END CERTIFICATE-----\n";

	std::string const key =
		"-----BEGIN PRIVATE KEY-----\n"
		"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDJ7BRKFO8fqmsE\n"
		"Xw8v9YOVXyrQVsVbjSSGEs4Vzs4cJgcFxqGitbnLIrOgiJpRAPLy5MNcAXE1strV\n"
		"GfdEf7xMYSZ/4wOrxUyVw/Ltgsft8m7bFu8TsCzO6XrxpnVtWk506YZ7ToTa5UjH\n"
		"fBi2+pWTxbpN12UhiZNUcrRsqTFW+6fO9d7xm5wlaZG8cMdg0cO1bhkz45JSl3wW\n"
		"KIES7t3EfKePZbNlQ5hPy7Pd5JTmdGBpyY8anC8u4LPbmgW0/U31PH0rRVfGcBbZ\n"
		"sAoQw5Tc5dnb6N2GEIbq3ehSfdDHGnrvenu2tOK9Qx6GEzXh3sekZkxcgh+NlIxC\n"
		"Nxu//Dk9AgMBAAECggEBAK1gV8uETg4SdfE67f9v/5uyK0DYQH1ro4C7hNiUycTB\n"
		"oiYDd6YOA4m4MiQVJuuGtRR5+IR3eI1zFRMFSJs4UqYChNwqQGys7CVsKpplQOW+\n"
		"1BCqkH2HN/Ix5662Dv3mHJemLCKUON77IJKoq0/xuZ04mc9csykox6grFWB3pjXY\n"
		"OEn9U8pt5KNldWfpfAZ7xu9WfyvthGXlhfwKEetOuHfAQv7FF6s25UIEU6Hmnwp9\n"
		"VmYp2twfMGdztz/gfFjKOGxf92RG+FMSkyAPq/vhyB7oQWxa+vdBn6BSdsfn27Qs\n"
		"bTvXrGe4FYcbuw4WkAKTljZX7TUegkXiwFoSps0jegECgYEA7o5AcRTZVUmmSs8W\n"
		"PUHn89UEuDAMFVk7grG1bg8exLQSpugCykcqXt1WNrqB7x6nB+dbVANWNhSmhgCg\n"
		"VrV941vbx8ketqZ9YInSbGPWIU/tss3r8Yx2Ct3mQpvpGC6iGHzEc/NHJP8Efvh/\n"
		"CcUWmLjLGJYYeP5oNu5cncC3fXUCgYEA2LANATm0A6sFVGe3sSLO9un1brA4zlZE\n"
		"Hjd3KOZnMPt73B426qUOcw5B2wIS8GJsUES0P94pKg83oyzmoUV9vJpJLjHA4qmL\n"
		"CDAd6CjAmE5ea4dFdZwDDS8F9FntJMdPQJA9vq+JaeS+k7ds3+7oiNe+RUIHR1Sz\n"
		"VEAKh3Xw66kCgYB7KO/2Mchesu5qku2tZJhHF4QfP5cNcos511uO3bmJ3ln+16uR\n"
		"GRqz7Vu0V6f7dvzPJM/O2QYqV5D9f9dHzN2YgvU9+QSlUeFK9PyxPv3vJt/WP1//\n"
		"zf+nbpaRbwLxnCnNsKSQJFpnrE166/pSZfFbmZQpNlyeIuJU8czZGQTifQKBgHXe\n"
		"/pQGEZhVNab+bHwdFTxXdDzr+1qyrodJYLaM7uFES9InVXQ6qSuJO+WosSi2QXlA\n"
		"hlSfwwCwGnHXAPYFWSp5Owm34tbpp0mi8wHQ+UNgjhgsE2qwnTBUvgZ3zHpPORtD\n"
		"23KZBkTmO40bIEyIJ1IZGdWO32q79nkEBTY+v/lRAoGBAI1rbouFYPBrTYQ9kcjt\n"
		"1yfu4JF5MvO9JrHQ9tOwkqDmNCWx9xWXbgydsn/eFtuUMULWsG3lNjfst/Esb8ch\n"
		"k5cZd6pdJZa4/vhEwrYYSuEjMCnRb0lUsm7TsHxQrUd6Fi/mUuFU/haC0o0chLq7\n"
		"pVOUFq5mW8p0zbtfHbjkgxyF\n"
		"-----END PRIVATE KEY-----\n";

	std::string const dh =
		"-----BEGIN DH PARAMETERS-----\n"
		"MIIBCAKCAQEArzQc5mpm0Fs8yahDeySj31JZlwEphUdZ9StM2D8+Fo7TMduGtSi+\n"
		"/HRWVwHcTFAgrxVdm+dl474mOUqqaz4MpzIb6+6OVfWHbQJmXPepZKyu4LgUPvY/\n"
		"4q3/iDMjIS0fLOu/bLuObwU5ccZmDgfhmz1GanRlTQOiYRty3FiOATWZBRh6uv4u\n"
		"tff4A9Bm3V9tLx9S6djq31w31Gl7OQhryodW28kc16t9TvO1BzcV3HjRPwpe701X\n"
		"oEEZdnZWANkkpR/m/pfgdmGPU66S2sXMHgsliViQWpDCYeehrvFRHEdR9NV+XJfC\n"
		"QMUk26jPTIVTLfXmmwU0u8vUkpR7LQKkwwIBAg==\n"
		"-----END DH PARAMETERS-----\n";

	_ssl_ctx.set_password_callback(
		[](std::size_t, boost::asio::ssl::context_base::password_purpose) {
			return "test";
		});

	_ssl_ctx.set_options(boost::asio::ssl::context::default_workarounds |
						 boost::asio::ssl::context::no_sslv2 |
						 boost::asio::ssl::context::single_dh_use);

	_ssl_ctx.use_certificate_chain(
		boost::asio::buffer(cert.data(), cert.size()));

	_ssl_ctx.use_private_key(boost::asio::buffer(key.data(), key.size()),
							 boost::asio::ssl::context::file_format::pem);

	_ssl_ctx.use_tmp_dh(boost::asio::buffer(dh.data(), dh.size()));
}

//=============================================================================-

webdonkey::coroutine::returning<void, std::suspend_always>
secure_server::serve_content(webdonkey::accept_result socket_or) {
	using namespace webdonkey;
	if (!socket_or.has_value()) {
		std::cerr << std::string{"Socket error: "} +
						 socket_or.error().message() + "\n";
		co_return;
	}

	auto request_gen = https(*socket_or.value(), _ssl_ctx);
	while (std::optional<expected_request<ssl_stream>> request_or =
			   co_await request_gen) {
		if (!request_or->has_value()) {
			std::cerr << request_or->error().message() + "\n";
			continue;
		}

		request_context_ptr ctx = request_or->value();
		std::cout << "Serving " + ctx->method_string() + " " + ctx->target() +
						 "\n";
		expected_response response_or = _respond(*ctx, ctx->target());
		response_ptr response{nullptr};
		if (response_or.has_value())
			response = response_or.value();
		else {
			std::cerr << "[HTTP error] " + response_or.error().message + "\n";
			beast::http::response<beast::http::string_body> res{
				response_or.error().status, ctx->request().version()};
			res.set(boost::beast::http::field::server, version());
			res.set(boost::beast::http::field::content_type, "text/html");
			res.keep_alive(ctx->request().keep_alive());
			res.body() = response_or.error().message;
			res.prepare_payload();
			response = std::make_shared<response_generator>(std::move(res));
		}

		auto status = co_await ctx->write(*response);
		if (!status.has_value())
			std::cerr << status.error().message() + "\n";
	}
}

//=============================================================================-

webdonkey::coroutine::returning<void, std::suspend_always>
secure_server::redirect(webdonkey::accept_result socket_or) {
	using namespace webdonkey;
	if (!socket_or.has_value()) {
		std::cerr << std::string{"Socket error: "} +
						 socket_or.error().message() + "\n";
		co_return;
	}

	auto request_gen = http(*socket_or.value());
	while (std::optional<expected_request<tcp_stream>> request_or =
			   co_await request_gen) {
		if (!request_or->has_value()) {
			std::cerr << request_or->error().message() + "\n";
			continue;
		}

		request_context_ptr ctx = request_or->value();
		beast::http::response<beast::http::empty_body> res{
			beast::http::status::moved_permanently, ctx->request().version()};
		res.set(boost::beast::http::field::server, version());
		res.set(boost::beast::http::field::content_type, "text/html");

		std::stringstream url_builder;
		url_builder << "https://" << ctx->request()[beast::http::field::host]
					<< ctx->target();
		std::string redirect_url = url_builder.str();

		std::stringstream log_stream;
		log_stream << "Redirect " << ctx->method_string() << " "
				   << ctx->target() << " to " << redirect_url << std::endl;
		std::cout << log_stream.str();

		res.set(beast::http::field::location, redirect_url);
		res.keep_alive(true);
		res.prepare_payload();
		auto result = co_await ctx->write(res);
	}
}

//=============================================================================-

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

	auto srv = std::make_shared<secure_server>(argv[1]);

	auto const address = boost::asio::ip::make_address("0.0.0.0");
	boost::asio::ip::tcp::endpoint https_endpoint{address, 443};
	tcp_listener<server_context, thread_pool> https_listener{
		https_endpoint,
		[srv](const accept_result &socket_or)
			-> coroutine::returning<void, std::suspend_always> {
			return srv->serve_content(socket_or);
		}};

	boost::asio::ip::tcp::endpoint http_endpoint{address, 80};
	tcp_listener<server_context, thread_pool> http_listener{
		http_endpoint,
		[srv](const accept_result &socket_or)
			-> coroutine::returning<void, std::suspend_always> {
			return srv->redirect(socket_or);
		}};

	shared_pool->join();

	return 0;
}
