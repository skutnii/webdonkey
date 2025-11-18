/*
 * example_error_handler.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef EXAMPLES_DONKEY_HTTP_EXAMPLE_ERROR_HANDLER_HPP_
#define EXAMPLES_DONKEY_HTTP_EXAMPLE_ERROR_HANDLER_HPP_

#include <webdonkey/http/default_error_handler.hpp>

class example_error_handler : public webdonkey::http::default_error_handler {
public:
	example_error_handler();
};

#endif /* EXAMPLES_DONKEY_HTTP_EXAMPLE_ERROR_HANDLER_HPP_ */
