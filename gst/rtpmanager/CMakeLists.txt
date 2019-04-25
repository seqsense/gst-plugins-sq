include_directories(./)

add_library(gstrtpmanagersq SHARED
  gstrtpmanagersq.c
  gstsrtpptdemux.c
)
target_link_libraries(gstrtpmanagersq ${GST_LIBRARIES} gstrtp-1.0)

install(
  TARGETS
    gstrtpmanagersq
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0
)
