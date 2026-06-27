# Headless engine + test. No Windows/TSF dependency — builds anywhere with a
# C++17 compiler. From windows/engine/:
#   cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure
cmake_minimum_required(VERSION 3.15)
project(bangla_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keylayout-driven engine (both Bangla Unicode + Classic) — used by the tray/demo.
add_library(bangla_kl STATIC klengine.cpp)
target_include_directories(bangla_kl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Old hand-written Unicode engine — used by the TSF IME (../tsf/) for now.
add_library(bangla_engine STATIC engine.cpp)
target_include_directories(bangla_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
  # /utf-8 so the u"..." (char16_t) literals encode as UTF-16 correctly.
  target_compile_options(bangla_kl     PUBLIC /utf-8)
  target_compile_options(bangla_engine PUBLIC /utf-8)
endif()

enable_testing()

add_executable(klengine-test klengine_test.cpp)   # both layouts vs interpreter
target_link_libraries(klengine-test PRIVATE bangla_kl)
add_test(NAME klengine_corpus COMMAND klengine-test)

add_executable(enginetest test.cpp)               # old hand engine (IME)
target_link_libraries(enginetest PRIVATE bangla_engine)
add_test(NAME corpus COMMAND enginetest)

# Standalone runnable demo (Windows only — uses the console API).
if(WIN32)
  add_executable(bangla-demo demo.cpp)
  target_link_libraries(bangla-demo PRIVATE bangla_kl)
endif()
