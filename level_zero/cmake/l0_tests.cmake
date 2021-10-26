#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

#Extract compute runtime COMPILE_DEFINITIONS
get_property(COMPUTE_RUNTIME_MOCKABLE_DEFINITIONS
             TARGET ${NEO_SHARED_MOCKABLE_LIB_NAME}
             PROPERTY COMPILE_DEFINITIONS
)

#Extract compute runtime INCLUDE_DIRECTORIES
get_property(COMPUTE_RUNTIME_MOCKABLE_INCLUDES
             TARGET ${NEO_SHARED_MOCKABLE_LIB_NAME}
             PROPERTY INCLUDE_DIRECTORIES
)

# Create a library that has the missing ingredients to link
add_library(compute_runtime_mockable_extra
            STATIC
            EXCLUDE_FROM_ALL
            ${CMAKE_CURRENT_LIST_DIR}/l0_tests.cmake
            ${NEO_SHARED_TEST_DIRECTORY}/common/aub_stream_mocks/aub_stream_interface_mock.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_gmm_client_context.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_cif.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_command_stream_receiver.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_compiler_interface_spirv.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_compiler_interface_spirv.h
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_compilers.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_deferred_deleter.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_device.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_gmm_client_context_base.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_gmm_resource_info_common.cpp
            ${NEO_SHARED_TEST_DIRECTORY}/common/mocks/mock_sip.cpp
)

set_property(TARGET compute_runtime_mockable_extra APPEND_STRING PROPERTY COMPILE_FLAGS ${ASAN_FLAGS} ${TSAN_FLAGS})

# These need to be added to a project to enable platform support in ULTs
#Additional includes for ULT builds
target_include_directories(compute_runtime_mockable_extra
                           PUBLIC
                           ${COMPUTE_RUNTIME_MOCKABLE_INCLUDES}
)

#Additional compile definitions for ULT builds
target_compile_definitions(compute_runtime_mockable_extra
                           PUBLIC
                           ${COMPUTE_RUNTIME_MOCKABLE_DEFINITIONS}
)

target_link_libraries(compute_runtime_mockable_extra
                      gmock-gtest
                      ${NEO_EXTRA_LIBS}
)
set_target_properties(compute_runtime_mockable_extra PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      FOLDER ${TARGET_NAME_L0}
)
