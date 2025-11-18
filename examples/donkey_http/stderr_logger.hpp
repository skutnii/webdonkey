/*
 * stderr_logger.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef EXAMPLES_DONKEY_HTTP_STDERR_LOGGER_HPP_
#define EXAMPLES_DONKEY_HTTP_STDERR_LOGGER_HPP_

#include "logger.hpp"

struct stderr_logger : public logger {
	void log(std::string_view s) override;

	virtual ~stderr_logger();
};

#endif /* EXAMPLES_DONKEY_HTTP_STDERR_LOGGER_HPP_ */
