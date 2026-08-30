cmake_minimum_required(VERSION 3.16)

project(BoostAsioComparison LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BOOST_INCLUDE_LIBRARIES headers system coroutine)
set(BOOST_ENABLE_CMAKE ON)

FetchContent_Declare(
    Boost
    URL "https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(Boost)

function(add_boost_example target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE ${Boost_SOURCE_DIR})
endfunction()

add_boost_example(io_context_vs_strand io_context_vs_strand/io_context_vs_strand.cpp)
add_boost_example(post_vs_dispatch post_vs_dispatch/post_vs_dispatch.cpp)
add_boost_example(post_vs_cospawn post_vs_cospawn/post_vs_cospawn.cpp)
