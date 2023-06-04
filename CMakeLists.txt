cmake_minimum_required(VERSION 3.25)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})
project(async VERSION ${PROJECT_VESRION})

include(FetchContent)

set(BOOST_INCLUDE_LIBRARIES lockfree algorithm)
set(BOOST_ENABLE_CMAKE ON)
#Windows help debug
#set(Boost_DEBUG ON)
FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.81.0
)

FetchContent_MakeAvailable(Boost)

add_executable(
    test 
    test.cpp
)

add_library(async SHARED async.cpp)

set_target_properties(
    test
    async
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(
    async 
    Boost::algorithm
    Boost::lockfree
)

target_link_libraries(
    test 
    async 
)

if (MSVC)
    target_compile_options(async PRIVATE /W4)
    target_compile_options(test PRIVATE /W4)
else ()
    target_link_options(test PRIVATE -static-libgcc -static-libstdc++)
    target_link_options(async PRIVATE -shared -static-libgcc -static-libstdc++)

    target_compile_options(
        async 
        PRIVATE
        -Wall 
        -Wextra 
        -pedantic 
    )

    target_compile_options(
        test 
        PRIVATE
        -Wall 
        -Wextra 
        -pedantic 
    )
endif()

install (
    TARGETS 
    async
    test 
    RUNTIME 
    DESTINATION 
    bin
)

# CPack area
if (WIN32)
    set(CPACK_GENERATOR "ZIP")
else ()
    set(CPACK_GENERATOR "DEB")
endif()
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT https://github.com/4l3x777/homework9)
include(CPack)
