cmake_minimum_required(VERSION 3.14)
project(daemonize VERSION 1.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/add_module")
include(AddModule)

add_module(cstuff
  GIT v2
  SIGNER 5F15F8C9C05B4D3D31EBB1C3F66D599380F88521
  file://$ENV{HOME}/repo/cstuff
  file://$ENV{HOME}/code/cstuff
  git@github.com:cyisfor/cstuff.git
  https://github.com/cyisfor/cstuff.git)

add_cstuff(record mmapfile)
