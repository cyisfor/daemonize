cmake_minimum_required(VERSION 3.14)
project(daemonize VERSION 1.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/add_module")
include(AddModule)

add_module(cstuff
  GIT v3
  SIGNER 5F15F8C9C05B4D3D31EBB1C3F66D599380F88521
  file://$ENV{HOME}/repo/cstuff
  file://$ENV{HOME}/code/cstuff
  git@github.com:cyisfor/cstuff.git
  https://github.com/cyisfor/cstuff.git)

add_cstuff(record mmapfile)

add_library(lib lib.c)
target_link_libraries(lib cstuff)
set_property(TARGET lib PROPERTY OUTPUT_NAME daemonize)

add_executable(daemonize main.c)
target_link_libraries(daemonize lib)

install(TARGETS daemonize lib)
