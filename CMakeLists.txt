cmake_minimum_required(VERSION 3.10.2)

project (unreliablefs
         DESCRIPTION "A FUSE-based fault injection filesystem."
         LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

add_subdirectory(unreliablefs)

add_custom_target(check DEPENDS check-mandoc check-rockspec)

set(ROCKSPEC "unreliablefs-scm-1.rockspec")
add_custom_target(check-rockspec DEPENDS ${ROCKSPEC})
add_custom_command(TARGET check-rockspec
                   COMMAND luarocks --local build ${PROJECT_SOURCE_DIR}/${ROCKSPEC}
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                   COMMENT "Build Lua rockspec")

set(MANUAL_PAGES "unreliablefs/unreliablefs.1;unreliablefs/unreliablefs.conf.5")
add_custom_target(check-mandoc DEPENDS ${MANUAL_PAGES})
add_custom_command(TARGET check-mandoc
                   COMMAND mandoc -T lint -W warning,stop ${MANUAL_PAGES}
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                   COMMENT "Run mandoc static analysis")
