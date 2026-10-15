cmake_minimum_required(VERSION 3.16)

project(gtest_criteria_consumer LANGUAGES CXX)

find_package(GTestCriteria CONFIG REQUIRED)

add_executable(gtest_criteria_consumer main.cpp)
target_link_libraries(gtest_criteria_consumer
  PRIVATE GTestCriteria::gtest_criteria)
