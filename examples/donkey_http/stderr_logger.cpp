/*
 * stderr_logger.cpp
 *
 *  Created on: Nov 17, 2025
 *      Author: Sergii Kutnii
 */

#include "stderr_logger.hpp"
#include <iostream>

void stderr_logger::log(std::string_view s) { std::cerr << s << std::endl; }

stderr_logger::~stderr_logger() { log("Logger deinitialized."); }