INCLUDE_DIRECTORIES(../include)
find_library(hackrf
   NAME hackrf
   PATHS /usr/local/lib /opt/sdr/lib)

add_library(hackrf_tools hackrf_tools.cpp)
target_link_libraries(hackrf_tools zmq)

add_executable(hackrf_fft hackrf_fft.cpp realtime.pb.cc)
target_link_libraries(hackrf_fft hackrf_tools protobuf)

add_executable(hackrf_check hackrf_check.cpp)
target_link_libraries(hackrf_check hackrf)

add_executable(capture_to_file capture_to_file.cpp)
target_link_libraries(capture_to_file hackrf)
