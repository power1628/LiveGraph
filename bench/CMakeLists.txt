

find_package(PkgConfig REQUIRED)
pkg_check_modules(glog REQUIRED libglog)
pkg_check_modules(gflags REQUIRED gflags)

message(STATUS "using glog ${glog_VERSION}")
message(STATUS "using gflags ${gflags_VERSION}")

add_executable(bench_load bench_load.cc)
target_link_libraries(bench_load livegraph glog gflags)