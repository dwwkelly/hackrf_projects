INCLUDE_DIRECTORIES(../include)
find_library(hackrf
   NAME hackrf
   PATHS /opt/sdr/lib /usr/local/lib
   NODEFAULTPATH)

add_library(hackrf_tools hackrf_tools.cpp)
target_link_libraries(hackrf_tools zmq)

add_executable(hackrf_fft hackrf_fft.cpp realtime.pb.cc)
target_link_libraries(hackrf_fft hackrf_tools protobuf)

add_executable(hackrf_check hackrf_check.cpp)
target_link_libraries(hackrf_check hackrf)

add_executable(capture_to_file capture_to_file.cpp)
target_link_libraries(capture_to_file hackrf)

add_executable(fftw_test fftw_test.c)
target_link_libraries(fftw_test fftw3)
target_link_libraries(fftw_test m)
