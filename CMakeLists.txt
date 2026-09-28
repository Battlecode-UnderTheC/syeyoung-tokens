# Builds the shared layer, every variant under variants/ as a plugin, and the tools.
#   cmake -S . -B build && cmake --build build -j
cmake_minimum_required(VERSION 3.20)
project(syeyoung_tokens CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# every plugin keeps its code to itself: variants may reuse names without clashing
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
find_package(Threads REQUIRED)

# the shared layer: engine, protocol, the Agent interface and loop, helpers
add_library(shared STATIC
  shared/engine/dragons/board.cpp shared/engine/dragons/game.cpp
  shared/protocol/observation.cpp
  shared/bot/protocol_io.cpp
  shared/lib/board_memory.cpp shared/lib/pathing.cpp)
target_include_directories(shared PUBLIC shared/engine shared/protocol shared/bot shared/lib)
target_compile_options(shared PRIVATE -O2 -Wall -Wextra)

# every variants/<name>/ with a *.cpp is a plugin: build/variants/lib<name>.so
file(GLOB variant_dirs LIST_DIRECTORIES true CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/variants/*)
foreach(dir ${variant_dirs})
  if(IS_DIRECTORY ${dir})
    get_filename_component(name ${dir} NAME)
    file(GLOB sources CONFIGURE_DEPENDS ${dir}/*.cpp)
    if(sources)
      add_library(${name} MODULE ${sources})
      target_include_directories(${name} PRIVATE ${dir})
      target_compile_options(${name} PRIVATE -O2 -Wall -Wextra)   # the judge builds with -O2
      target_link_libraries(${name} PRIVATE shared)
      set_target_properties(${name} PROPERTIES PREFIX "lib" SUFFIX ".so"
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/variants)
    endif()
  endif()
endforeach()

# the tools; play.* is what they share: loading plugins, reading maps, playing a match
add_library(play STATIC tools/play.cpp)
target_link_libraries(play PUBLIC shared ${CMAKE_DL_LIBS})
target_compile_options(play PRIVATE -O2 -Wall -Wextra)
add_executable(sim tools/sim_main.cpp)
target_link_libraries(sim PRIVATE play Threads::Threads)
add_executable(match tools/match_main.cpp)
target_link_libraries(match PRIVATE play)
add_executable(check tools/check_main.cpp)
target_link_libraries(check PRIVATE play)
set_target_properties(sim match check PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)
