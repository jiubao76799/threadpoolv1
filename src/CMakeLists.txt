#src 目录的CMakeLists.txt
set(SOURCES ThreadPool.cpp )

#创建线程池库
add_library(threadpool ${SOURCES})

#链接线程库
find_package(Threads REQUIRED)
target_link_libraries(threadpool PRIVATE Threads::Threads)


#安装库
install (TARGETS threadpool
         LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

