
add_executable(msf msf_test.cpp common/msf.cpp)
target_link_libraries(msf ABY::aby ENCRYPTO_utils::encrypto_utils)
