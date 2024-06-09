cmake_minimum_required(VERSION 3.28)
project(spw_ethernet_producer)

# Set custom Boost installation path
set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/include/boost/install")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")

# Provide hints to find Boost
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

set(Boost_INCLUDE_DIR ${BOOST_INCLUDEDIR})
set(Boost_LIBRARY_DIR ${BOOST_LIBRARYDIR})
add_definitions(-DBOOST_LOG_DYN_LINK)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Define the static library directory
set(SPW_ETH_LIBS ${CMAKE_SOURCE_DIR}/include/SpW_Eth_Bridge/src/libs)

add_executable(spw_ethernet_producer
        src/publisher_main.cpp
        src/TCPReceiver.h
        src/TCPReceiverForSPW.cpp
        src/TCPReceiverForSPW.h
        src/SpW_Eth_utils.cpp
        src/SpW_Eth_utils.h
        src/logging.cpp
        src/TCPReceiverSyncSPW.cpp
        src/TCPReceiverSyncSPW.h
)

add_executable(tester
        src/tester.cpp
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include/SpW_Eth_Bridge/src/headers ${Boost_INCLUDE_DIR})

# Link directories
link_directories(${SPW_ETH_LIBS} ${Boost_LIBRARY_DIR})

# Explicitly set Boost libraries
set(Boost_LIBRARIES
        ${Boost_LIBRARY_DIR}/libboost_atomic.so
        ${Boost_LIBRARY_DIR}/libboost_chrono.so
        ${Boost_LIBRARY_DIR}/libboost_date_time.so
        ${Boost_LIBRARY_DIR}/libboost_filesystem.so
        ${Boost_LIBRARY_DIR}/libboost_log.so
        ${Boost_LIBRARY_DIR}/libboost_log_setup.so
        ${Boost_LIBRARY_DIR}/libboost_regex.so
        ${Boost_LIBRARY_DIR}/libboost_system.so
        ${Boost_LIBRARY_DIR}/libboost_thread.so
)

# Set runtime search path for the executable
set_target_properties(spw_ethernet_producer PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH ${Boost_LIBRARY_DIR}
        INSTALL_RPATH ${Boost_LIBRARY_DIR}
        LINK_FLAGS "-no-pie"
)

# Ensure the rpath is set correctly to find the shared libraries
set_target_properties(spw_ethernet_producer PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "${Boost_LIBRARY_DIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Link libraries
target_link_libraries(spw_ethernet_producer
        PRIVATE Threads::Threads
        PUBLIC ${Boost_LIBRARIES}
        PUBLIC ${SPW_ETH_LIBS}/libspw_eth_unix.a
)

target_link_libraries(tester
        PRIVATE Threads::Threads
)

add_executable(spw_ethernet_consumer
        src/SPWReceiver.cpp
        src/consumer_main.cpp
        src/logging.cpp
        src/SpW_Eth_utils.cpp
        src/SPWReceiverForTCP.cpp
        src/SPWReceiverForTCP.h
        src/ResilientSocket.h
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include/SpW_Eth_Bridge/src/headers ${Boost_INCLUDE_DIR})

# Set runtime search path for the executable
set_target_properties(spw_ethernet_consumer PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH ${Boost_LIBRARY_DIR}
        INSTALL_RPATH ${Boost_LIBRARY_DIR}
        LINK_FLAGS "-no-pie"
)

# Link libraries
target_link_libraries(spw_ethernet_consumer
        PRIVATE Threads::Threads
        PUBLIC ${Boost_LIBRARIES}
        PUBLIC ${SPW_ETH_LIBS}/libspw_eth_unix.a
)

option(BUILD_TESTS "Build the unit tests." OFF)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})

    add_executable(tests
            src/TCPReceiver.h
            src/TCPReceiverForSPW.cpp
            src/SpW_Eth_utils.cpp
            src/SpW_Eth_utils.h
            src/logging.h
    )

    target_link_libraries(tests
            PRIVATE Threads::Threads ${GTEST_LIBRARIES} GTest::GTest GTest::gmock
            PUBLIC ${Boost_LIBRARIES}
            PUBLIC ${SPW_ETH_LIBS}/libspw_eth_unix.a
    )

    gtest_discover_tests(tests)
endif()
