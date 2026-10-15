add_executable(example main.cpp)
set_source_files_properties(
  "${PROJECT_SOURCE_DIR}/include/test_fixture.tpp"
  PROPERTIES HEADER_FILE_ONLY TRUE
)
target_sources(example PRIVATE
  "${PROJECT_SOURCE_DIR}/include/test_fixture.tpp"
)
target_link_libraries(example PRIVATE GTestCriteria::gtest_criteria)

if(CMAKE_GENERATOR STREQUAL "Xcode")
  set_target_properties(example PROPERTIES
    XCODE_GENERATE_SCHEME ON
    XCODE_SCHEME_ARGUMENTS
      "--gtest_repeat=5;--gtest_output=sqlite:test_output.db"
  )
endif()
