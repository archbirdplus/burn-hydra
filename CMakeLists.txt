cmake_minimum_required(VERSION 3.18)

# find_package(MPI)

# set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
set(CMAKE_CXX_STANDARD 17)

if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER, "mpic++")
endif()

if(NOT DEFINED FLINT_PATH)
    set(FAILED_DEPS TRUE)
    message("Need -DFLINT_PATH to proceed.")
endif()

if(NOT DEFINED GMP_PATH)
    set(FAILED_DEPS TRUE)
    message("Need -DGMP_PATH to proceed.")
endif()

if(NOT DEFINED MPI_PATH)
    set(FAILED_DEPS TRUE)
    message("Need -DMPI_PATH to proceed.")
endif()

if(FAILED_DEPS)
    message(FATAL_ERROR "Dependencies were not found.")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error LANGUAGES CXX)

project(BurnHydra)
include(CTest)

function(equip target)
    target_link_directories(${target}
        PUBLIC ${FLINT_PATH}/lib ${GMP_PATH}/lib ${MPI_PATH}/lib
    )
    target_include_directories(${target}
        PRIVATE
        ${FLINT_PATH}/include ${GMP_PATH}/include ${MPI_PATH}/include
    )
    target_link_libraries(${target}
        flint gmp mpi
    )
endfunction()

# BigCollatz library
add_library(BigCollatz SHARED)

target_sources(BigCollatz
    PRIVATE
    src/fluent/fluent.cpp src/state/init.cpp # src/state/parse.cpp
    src/state/burner/burner_mpi.cpp
    src/state/burner/burner_simple.cpp src/state/burner/burner_openmp.cpp
        src/state/burner/burner_m2exp.cpp
    src/state/burner/basecase_simple.cpp src/state/burner/basecase_m2exp.cpp
        src/state/burner/basecase_table.cpp
    src/utils/communicate.cpp src/utils/friendly_assert.cpp src/utils/metrics.cpp
    src/utils/timed_fmpz.cpp src/utils/locked_fmpz.cpp
    include/communicate.h include/friendly_assert.h include/parse.h

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/fluent.h include/state.h
)

if(ipo_supported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET BigCollatz PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${ipo_error}>")
endif()

equip(BigCollatz)

# googletests

# option(PACKAGE_TESTS "Build the tests" ON)
# if(PACKAGE_TESTS)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# endif()

# test cases
add_executable(my_tests)
target_sources(my_tests
    PRIVATE
    src/test/test_init.cpp
    src/test/test_burners.cpp
    src/test/test_fluent.cpp
    src/test/test.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
)

equip(my_tests)

target_link_libraries(
    my_tests
    BigCollatz
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(my_tests)

# burn_hydra demo
add_executable(burn_hydra)
target_sources(burn_hydra
    PRIVATE
    src/apps/burn_hydra.cpp
)

target_include_directories(burn_hydra
    PRIVATE
    ${FLINT_PATH}/include ${GMP_PATH}/include ${MPI_PATH}/include
)

target_link_libraries(burn_hydra
    PRIVATE
    BigCollatz
)




