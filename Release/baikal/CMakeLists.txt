add_subdirectory(caldav)
add_subdirectory(carddav)

add_custom_target(baikal
  DEPENDS  caldav carddav
  )
