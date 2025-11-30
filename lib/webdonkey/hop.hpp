/*
 * hop.hpp
 *
 *  Created on: Nov 30, 2025
 *      Author: Sergii Kutnii
 */

#ifndef LIB_WEBDONKEY_HOP_HPP_
#define LIB_WEBDONKEY_HOP_HPP_

#include <webdonkey/continuation.hpp>
#include <webdonkey/defs.hpp>

namespace webdonkey {

namespace coroutine {

template <class executor>
continuation<void, continuation_flavor::blocking> hop(executor &exec) {
	using continuation = continuation<void, continuation_flavor::blocking>;
	continuation then;
	asio::defer(exec, then);
	return then;
}

} // namespace coroutine

} // namespace webdonkey

#endif /* LIB_WEBDONKEY_HOP_HPP_ */
