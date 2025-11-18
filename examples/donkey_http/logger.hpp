/*
 * logger.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef EXAMPLES_DONKEY_HTTP_LOGGER_HPP_
#define EXAMPLES_DONKEY_HTTP_LOGGER_HPP_

#include <string_view>

struct logger {
	virtual void log(std::string_view s) = 0;

	virtual ~logger() = default;
};

#endif /* EXAMPLES_DONKEY_HTTP_LOGGER_HPP_ */
