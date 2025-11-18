cmake_minimum_required(VERSION 3.10...3.27)

add_executable(donkey_http donkey_http.cpp server_context.cpp stderr_logger.cpp example_error_handler.cpp)

find_package(Boost)
find_package(OpenSSL REQUIRED)

target_include_directories(donkey_http PRIVATE ${WEBDONKEY_SOURCE_DIR})
target_link_libraries(donkey_http PRIVATE ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})