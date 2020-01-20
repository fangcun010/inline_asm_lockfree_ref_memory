add_library(lockfree lockfree.cpp)
target_include_directories(lockfree PRIVATE ../include ../../boost_1_72_0)
target_sources(lockfree PRIVATE ../include/lockfree.h)

add_executable(test_lockfree test_lockfree.cpp)
target_include_directories(test_lockfree PRIVATE ../include ../../boost_1_72_0)
target_link_libraries(test_lockfree lockfree)