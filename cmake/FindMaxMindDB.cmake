include(FindPackageHandleStandardArgs)
include(CMakeFindDependencyMacro)

# Use system OGG
find_dependency(PkgConfig)
pkg_check_modules(PC_MaxMindDB REQUIRED libmaxminddb)

if(PC_MaxMindDB_FOUND)
  set(MaxMindDB_LIBRARIES ${PC_MaxMindDB_LIBRARIES})
  set(MaxMindDB_INCLUDE_DIRS ${PC_MaxMindDB_INCLUDE_DIRS})

  mark_as_advanced(
      MaxMindDB_LIBRARIES
      MaxMindDB_INCLUDE_DIRS
  )

  set(MaxMindDB_FOUND TRUE)
  add_library(MaxMindDB::MaxMindDB INTERFACE IMPORTED)
  if(MaxMindDB_INCLUDE_DIRS)
      set_property(TARGET MaxMindDB::MaxMindDB PROPERTY
          INTERFACE_INCLUDE_DIRECTORIES "${MaxMindDB_INCLUDE_DIRS}"
      )
  endif()
  set_property(TARGET MaxMindDB::MaxMindDB PROPERTY
      INTERFACE_LINK_LIBRARIES "${MaxMindDB_LIBRARIES}"
  )
endif()
