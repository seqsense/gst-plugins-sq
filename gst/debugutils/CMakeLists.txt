add_library(gstdebugutilssq SHARED
  debugutilssq.c
  gstwatchdogeos.c
)
target_link_libraries(gstdebugutilssq ${GST_LIBRARIES})

install(
  TARGETS
    gstdebugutilssq
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0
)
