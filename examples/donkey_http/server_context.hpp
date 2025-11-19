/*
 * context.hpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#ifndef EXAMPLES_DONKEY_HTTP_SERVER_CONTEXT_HPP_
#define EXAMPLES_DONKEY_HTTP_SERVER_CONTEXT_HPP_

#include "logger.hpp"
#include "webdonkey/http/error_handler.hpp"
#include <webdonkey/contextual.hpp>

class server_context : public webdonkey::shared_registry<server_context> {
public:
	template <class context, typename instance_type>
	using shared_object = webdonkey::shared_object<context, instance_type>;

	static shared_object<server_context, logger> shared_logger;

	static shared_object<server_context, webdonkey::http::error_handler_base>
		shared_error_handler;
};

#endif /* EXAMPLES_DONKEY_HTTP_SERVER_CONTEXT_HPP_ */
