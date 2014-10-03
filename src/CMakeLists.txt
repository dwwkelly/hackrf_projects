INCLUDE_DIRECTORIES(../include)
INCLUDE_DIRECTORIES(/opt/sdr/include)
find_library(hackrf
   NAME hackrf
   PATHS /opt/sdr/lib /usr/local/lib
   NODEFAULTPATH)

add_library(hackrf_tools hackrf_tools.cpp)
target_link_libraries(hackrf_tools zmq)

add_executable(hackrf_fft hackrf_state.c hackrf_fft.cpp realtime.pb.cc)
target_link_libraries(hackrf_fft hackrf_tools protobuf)

add_executable(hackrf_check hackrf_check.cpp)
target_link_libraries(hackrf_check hackrf)

add_executable(capture_to_file capture_to_file.c hackrf_state.c hackrf_util.c)
target_link_libraries(capture_to_file hackrf)

add_executable(energy_detector energy_detector.c hackrf_util.c)
target_link_libraries(energy_detector hackrf)
target_link_libraries(energy_detector pthread)
target_link_libraries(energy_detector zmq)
target_link_libraries(energy_detector fftw3)
