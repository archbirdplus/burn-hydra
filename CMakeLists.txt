cmake_minimum_required(VERSION 3.30)

# find_package(MPI)

# set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
set(CMAKE_CXX_STANDARD 17)

if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER, "mpic++ -v")
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

project(BurnHydra)

add_library(BigCollatz SHARED)

target_sources(BigCollatz
    PRIVATE
    src/fluent/fluent.cpp src/state/init.cpp # src/state/parse.cpp
    src/utils/communicate.cpp src/utils/friendly_assert.cpp src/utils/metrics.cpp
    include/communicate.h include/friendly_assert.h include/parse.h

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/fluent.h include/state.h
)

target_link_directories(BigCollatz
    PUBLIC ${FLINT_PATH}/lib ${GMP_PATH}/lib ${MPI_PATH}/lib
)

target_include_directories(BigCollatz
    PRIVATE
    ${FLINT_PATH}/include ${GMP_PATH}/include ${MPI_PATH}/include
)

target_link_libraries(BigCollatz
    flint gmp mpi
)

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




