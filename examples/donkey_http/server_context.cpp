/*
 * context.cpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#include "server_context.hpp"
#include "example_error_handler.hpp"
#include "stderr_logger.hpp"
#include <memory>

server_context::shared_object<server_context, logger>
	server_context::shared_logger{std::make_shared<stderr_logger>()};

server_context::shared_object<server_context,
							  webdonkey::http::error_handler_base>
	server_context::shared_error_handler{
		std::make_shared<example_error_handler>()};