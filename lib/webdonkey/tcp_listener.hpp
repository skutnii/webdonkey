/*
 * tcp_socket_listener.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_TCP_LISTENER_HPP_
#define LIB_WEBDONKEY_TCP_LISTENER_HPP_

#include "webdonkey/continuation.hpp"
#include <coroutine>
#include <webdonkey/defs.hpp>

#include <atomic>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/cobalt.hpp>
#include <boost/cobalt/spawn.hpp>
#include <expected>
#include <webdonkey/contextual.hpp>
#include <webdonkey/coroutines.hpp>

namespace webdonkey {

using accept_result = std::expected<socket_ptr, boost::system::error_code>;
using socket_acceptor = coroutine::yielding<accept_result, std::suspend_always>;

template <class context, class executor> class tcp_listener {
public:
	void stop() { _state->stopped = true; }
	bool stopped() const { return _state->stopped; }

	template <typename socket_handler>
	tcp_listener(const tcp::endpoint &endpoint, socket_handler handler) {
		state_ptr shared_state = std::make_shared<state>();
		_state = shared_state;

		_state->acceptor.open(endpoint.protocol());
		_state->acceptor.set_option(asio::socket_base::reuse_address(true));
		_state->acceptor.bind(endpoint);
		_state->acceptor.listen(asio::socket_base::max_listen_connections);

		asio::post(*_state->exec, handle_connections(_state, handler));
	}

	~tcp_listener() { stop(); }

private:
	using executor_ptr = managed_ptr<context, executor>;

	struct state {
		executor_ptr exec;
		tcp::acceptor acceptor;
		std::atomic<bool> stopped = false;

		coroutine::continuation<accept_result,
								coroutine::continuation_flavor::copy>
		accept() {
			using continuation =
				coroutine::continuation<accept_result,
										coroutine::continuation_flavor::copy>;
			continuation then;
			acceptor.async_accept(
				asio::make_strand(*exec),
				[then](const boost::system::error_code &error,
					   boost::asio::ip::tcp::socket peer) {
					if (error)
						const_cast<continuation &>(then)(
							std::unexpected{error});
					else
						const_cast<continuation &>(then)(
							std::make_shared<tcp::socket>(std::move(peer)));
				});

			return then;
		}

		state() :
			acceptor{*exec} {}
	};

	using state_ptr = std::shared_ptr<state>;

	template <typename socket_handler>
	static coroutine::returning<void, std::suspend_always>
	handle_connections(state_ptr shared_state, socket_handler handler) {
		while (!shared_state->stopped) {
			accept_result socket_or = co_await shared_state->accept();
			handler(socket_or);
		}
	}

	state_ptr _state;
};

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_TCP_LISTENER_HPP_ */
