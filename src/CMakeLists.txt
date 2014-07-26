find_library(hackrf
   NAME hackrf
   PATHS /usr/local/lib)
add_executable (hackrf_check hackrf_check.cpp) 
target_link_libraries(hackrf_check hackrf)
