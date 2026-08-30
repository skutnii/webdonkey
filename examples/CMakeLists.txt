cmake_minimum_required(VERSION 3.10...3.27)

find_package(Boost)
find_package(OpenSSL REQUIRED)

add_executable(donkey_http donkey_http.cpp )
target_include_directories(donkey_http PRIVATE ${WEBDONKEY_SOURCE_DIR})
target_link_libraries(donkey_http PRIVATE libboost_cobalt.a ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

add_executable(donkey_https donkey_https.cpp )
target_include_directories(donkey_https PRIVATE ${WEBDONKEY_SOURCE_DIR})
target_link_libraries(donkey_https PRIVATE ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}  libboost_cobalt.a)