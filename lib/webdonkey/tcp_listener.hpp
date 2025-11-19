/*
 * tcp_socket_listener.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_TCP_LISTENER_HPP_
#define LIB_WEBDONKEY_TCP_LISTENER_HPP_

#include <webdonkey/defs.hpp>

#include <atomic>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/signals2.hpp>
#include <variant>
#include <webdonkey/contextual.hpp>

namespace webdonkey {

template <class context, class handler, class executor> class tcp_listener {
public:
	using failure = std::variant<beast::error_code, std::exception *>;
	using failure_signal = boost::signals2::signal<void(failure)>;

	using executor_ptr = managed_ptr<context, executor>;

	void bind(const tcp::endpoint &endpoint) {
		boost::asio::co_spawn(*_executor, listen(endpoint), asio::detached);
	}

	failure_signal &on_failure() { return _on_failure; }

	void fail(failure f) { _on_failure(f); }

	void stop() { _stopped = true; }
	bool stopped() const { return _stopped; }

private:
	asio::awaitable<void> listen(const tcp::endpoint &endpoint);
	handler *as_handler() { return static_cast<handler *>(this); }

	executor_ptr _executor;
	std::atomic<bool> _stopped = false;
	failure_signal _on_failure;
};

//==============================================================================

template <class context, class handler, class executor>
asio::awaitable<void> tcp_listener<context, handler, executor>::listen(
	const tcp::endpoint &endpoint) {
	beast::error_code ec;

	// Open the acceptor
	tcp::acceptor acceptor(*_executor);
	acceptor.open(endpoint.protocol(), ec);
	if (ec)
		co_return fail(ec);

	// Allow address reuse
	acceptor.set_option(asio::socket_base::reuse_address(true), ec);
	if (ec)
		co_return fail(ec);

	// Bind to the server address
	acceptor.bind(endpoint, ec);
	if (ec)
		co_return fail(ec);

	// Start listening for connections
	acceptor.listen(asio::socket_base::max_listen_connections, ec);
	if (ec)
		co_return fail(ec);
	while (!_stopped) {
		tcp::socket socket = co_await acceptor.async_accept(
			asio::make_strand(*_executor), asio::use_awaitable);
		boost::asio::co_spawn(*_executor,
							  as_handler()->handle(std::move(socket)),
							  asio::detached);
	}
}

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_TCP_LISTENER_HPP_ */
