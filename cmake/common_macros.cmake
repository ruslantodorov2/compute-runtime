#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

macro(hide_subdir subdir)
  file(RELATIVE_PATH subdir_relative ${NEO_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
  set(${subdir_relative}_hidden} TRUE)
endmacro()

macro(add_subdirectory_unique subdir)
  file(RELATIVE_PATH subdir_relative ${NEO_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
  if(NOT ${subdir_relative}_hidden})
    add_subdirectory(${subdir} ${ARGN})
  endif()
  hide_subdir(${subdir})
endmacro()

macro(add_subdirectories)
  file(GLOB subdirectories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
  foreach(subdir ${subdirectories})
    file(RELATIVE_PATH subdir_relative ${NEO_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt AND NOT ${subdir_relative}_hidden})
      add_subdirectory(${subdir})
    endif()
  endforeach()
endmacro()

macro(create_project_source_tree target)
  if(MSVC)
    set(prefixes ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN} ${NEO_SOURCE_DIR})
    get_target_property(source_list ${target} SOURCES)
    foreach(source_file ${source_list})
      if(NOT ${source_file} MATCHES "\<*\>")
        string(TOLOWER ${source_file} source_file_relative)
        foreach(prefix ${prefixes})
          if(source_file_relative)
            string(TOLOWER ${prefix} prefix)
            string(REPLACE "${prefix}" "" source_file_relative ${source_file_relative})
          endif()
        endforeach()
        get_filename_component(source_path_relative ${source_file_relative} PATH)
        if(source_path_relative)
          string(REPLACE "/" "\\" source_path_relative ${source_path_relative})
        endif()
        source_group("Source Files\\${source_path_relative}" FILES ${source_file})
      endif()
    endforeach()
  endif()
endmacro()

macro(create_project_source_tree_with_exports target exports_filename)
  create_project_source_tree(${target} ${ARGN})
  if(MSVC)
    if(NOT "${exports_filename}" STREQUAL "")
      source_group("exports" FILES "${exports_filename}")
    endif()
  endif()
endmacro()

macro(apply_macro_for_each_core_type type)
  set(given_type ${type})
  foreach(CORE_TYPE ${ALL_CORE_TYPES})
    string(TOLOWER ${CORE_TYPE} CORE_TYPE_LOWER)
    CORE_CONTAINS_PLATFORMS(${given_type} ${CORE_TYPE} COREX_HAS_PLATFORMS)
    if(${COREX_HAS_PLATFORMS})
      macro_for_each_core_type()
    endif()
  endforeach()
endmacro()

macro(apply_macro_for_each_platform)
  GET_PLATFORMS_FOR_CORE_TYPE(${given_type} ${CORE_TYPE} TESTED_COREX_PLATFORMS)
  foreach(PLATFORM_IT ${TESTED_COREX_PLATFORMS})
    string(TOLOWER ${PLATFORM_IT} PLATFORM_IT_LOWER)
    macro_for_each_platform()
  endforeach()
endmacro()

macro(get_family_name_with_type core_type platform_type)
  string(REPLACE "GEN" "Gen" core_type_capitalized ${core_type})
  string(TOLOWER ${platform_type} platform_type_lower)
  set(family_name_with_type ${core_type_capitalized}${platform_type_lower})
endmacro()

macro(append_sources_from_properties list_name)
  foreach(name ${ARGN})
    get_property(${name} GLOBAL PROPERTY ${name})
    list(APPEND ${list_name} ${${name}})
  endforeach()
endmacro()
