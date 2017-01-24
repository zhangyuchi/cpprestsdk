if (NOT WINDOWS_STORE AND NOT WINDOWS_PHONE)
  add_executable(caldav caldavclient.cpp)
  target_link_libraries(caldav cpprest)
endif()