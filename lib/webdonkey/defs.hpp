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
#include <boost/asio/detached.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>

namespace webdonkey {

namespace beast = boost::beast;	  // from <boost/beast.hpp>
namespace asio = boost::asio;	  // from <boost/asio.hpp>
namespace ssl = asio::ssl;		  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

using tcp_stream = beast::tcp_stream;
using ssl_stream = beast::ssl_stream<tcp_stream>;

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_DEFS_HPP_ */
