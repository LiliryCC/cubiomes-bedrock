cmake_minimum_required(VERSION 3.10)
project(shv_rate_heatmap C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_BUILD_TYPE Release)

find_package(OpenMP REQUIRED)

set(CUBIOMES_CORE_SOURCES
    generator.c
    finders.c
    layers.c
    biomenoise.c
    noise.c
    biomes.c
    mt.c
    util.c
)

add_executable(heatmap 1_heatmap.c ${CUBIOMES_CORE_SOURCES})

if(OpenMP_C_FOUND)
    target_link_libraries(heatmap PUBLIC OpenMP::OpenMP_C)
endif()