if (NOT WINDOWS_STORE AND NOT WINDOWS_PHONE)
  add_executable(carddav carddavclient.cpp)
  target_link_libraries(carddav cpprest)
endif()