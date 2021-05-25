add_executable (Test test_main.cpp test_redis.cpp ../src/redis_client.cpp )
target_link_libraries (Test 
            ${CMAKE_SOURCE_DIR}/third_party/hiredis/libhiredis.a
            ${Boost_LIBRARIES} 
            pthread
            )