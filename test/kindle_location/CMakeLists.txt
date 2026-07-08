add_executable(KindleLocationTest
  KindleLocationTest.cpp
)

target_include_directories(KindleLocationTest PRIVATE
  ${REPO_ROOT}/src/activities/reader
)

target_link_libraries(KindleLocationTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(KindleLocationTest)
