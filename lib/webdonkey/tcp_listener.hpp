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
#include <webdonkey/contextual.hpp>

namespace webdonkey {

template <class context, class executor> class tcp_listener {
public:
	void stop() { _state->stopped = true; }
	bool stopped() const { return _state->stopped; }

	using executor_ptr = managed_ptr<context, executor>;
	template <typename handler_type>
	tcp_listener(const tcp::endpoint &endpoint, handler_type socket_handler) {
		state_ptr shared_state = std::make_shared<state>();

		shared_state->acceptor.open(endpoint.protocol());
		shared_state->acceptor.set_option(
			asio::socket_base::reuse_address(true));
		shared_state->acceptor.bind(endpoint);
		shared_state->acceptor.listen(
			asio::socket_base::max_listen_connections);

		asio::co_spawn(*shared_state->exec,
					   accept_connections(shared_state, socket_handler),
					   asio::detached);

		_state = shared_state;
	}

	~tcp_listener() { stop(); }

private:
	struct state {
		managed_ptr<context, executor> exec;
		tcp::acceptor acceptor;
		std::atomic<bool> stopped = false;

		state() :
			acceptor{*exec} {}
	};

	using state_ptr = std::shared_ptr<state>;

	template <typename handler_type>
	static awaitable<void> accept_connections(state_ptr shared_state,
											  handler_type handler) {
		while (!shared_state->stopped) {
			tcp::socket socket = co_await shared_state->acceptor.async_accept(
				asio::make_strand(*shared_state->exec), asio::use_awaitable);
			asio::co_spawn(*shared_state->exec, handler(socket),
						   asio::detached);
		}
	}

	state_ptr _state;
};

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_TCP_LISTENER_HPP_ */
