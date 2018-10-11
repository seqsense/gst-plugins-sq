cmake_minimum_required(VERSION 2.8.3)
project(gst-plugins-sq)

include(GNUInstallDirs)

find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4
  gstreamer-base-1.0>=1.4
  gobject-2.0
)

add_definitions(
  -DVERSION="0.0.0"
  -DGST_LICENSE="LGPL"
  -DPACKAGE="gstpluginssq"
  -DGST_PACKAGE_NAME="gst-plugins-sq"
  -DGST_PACKAGE_ORIGIN="https://github.com/seqsense"
)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})

add_subdirectory(gst)
