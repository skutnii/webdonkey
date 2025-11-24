/*
 * defs.hpp
 *
 *  Created on: Nov 16, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_DEFS_HPP_
#define LIB_WEBDONKEY_DEFS_HPP_

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pack_options.hpp>

namespace webdonkey {

namespace beast = boost::beast;	  // from <boost/beast.hpp>
namespace asio = boost::asio;	  // from <boost/asio.hpp>
namespace ssl = asio::ssl;		  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

using tcp_stream = beast::tcp_stream;
using ssl_stream = beast::ssl_stream<tcp_stream>;

template <typename value_type> using awaitable = asio::awaitable<value_type>;
using socket_ptr = std::shared_ptr<tcp::socket>;

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_DEFS_HPP_ */
