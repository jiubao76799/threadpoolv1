#test/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)


#定义一个函数来添加测试
function (add_pool_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${test_name} PRIVATE threadpool)
endfunction()

#添加测试
add_pool_test(test_day1_basic test1.cpp)
add_pool_test(test_day2_basic test2.cpp)
add_pool_test(test_day3_basic test3.cpp)
add_pool_test(test_day4_basic test4.cpp)
add_pool_test(test_day5_basic test5.cpp)
#add_pool_test(test_day6_basic test6.cpp)
add_pool_test(test_day7_basic test7.cpp)
