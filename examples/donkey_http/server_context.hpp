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

class server_context : public webdonkey::context_base<server_context> {
public:
	using base = webdonkey::context_base<server_context>;

	template <typename instance_type>
	using shared_instance = base::shared_instance<instance_type>;

	template <typename instance_type>
	using instance_constructor = base::instance_constructor<instance_type>;

	template <typename instance_type>
	using managed_ptr = base::managed_ptr<instance_type>;

	static shared_instance<logger> shared_logger;

	static shared_instance<webdonkey::http::error_handler_base>
		shared_error_handler;
};

#endif /* EXAMPLES_DONKEY_HTTP_SERVER_CONTEXT_HPP_ */
