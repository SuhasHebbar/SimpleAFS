cmake_minimum_required(VERSION 3.10.2)

include(CheckFunctionExists)


# set(EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(GRPC_AS_SUBMODULE TRUE)
# set(GRPC_FETCHCONTENT TRUE)
# set(vGRPC_TAG_VERSION_OF_YOUR_CHOICE "v1.50.0")
include(../grpc_setup.cmake)

# Proto file
get_filename_component(afs_proto "../afs/afs.proto" ABSOLUTE)
get_filename_component(afs_proto_path "${afs_proto}" PATH)

# Generated sources
set(afs_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/afs.pb.cc")
set(afs_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/afs.pb.h")
set(afs_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/afs.grpc.pb.cc")
set(afs_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/afs.grpc.pb.h")

# message(afs_proto="${afs_proto}", "${afs_proto_srcs}","${hw_proto_hdrs}", "${hw_grpc_srcs}", "${hw_grpc_hdrs}")
# message(protoc="${_PROTOBUF_PROTOC}")

add_custom_command(
      OUTPUT "${afs_proto_srcs}" "${afs_proto_hdrs}" "${afs_grpc_srcs}" "${afs_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${afs_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${afs_proto}"
      DEPENDS "${afs_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
add_library(afs_grpc_proto
  ${afs_grpc_srcs}
  ${afs_grpc_hdrs}
  ${afs_proto_srcs}
  ${afs_proto_hdrs})
target_link_libraries(afs_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})


set(UNRELIABLEFS_SRC conf.c
                     unreliablefs.c
                     unreliablefs_errinj.c
                     unreliablefs_ops.c
                     afs_client.cpp)

find_package(FUSE 2.9 REQUIRED)


add_definitions(${FUSE_DEFINITIONS})

set(AFS_CLIENT_EXECUTABLE client)
set(AFS_SERVER_EXECUTABLE server)

add_executable(${AFS_CLIENT_EXECUTABLE} ${UNRELIABLEFS_SRC})
target_include_directories(${AFS_CLIENT_EXECUTABLE} PRIVATE ${FUSE_INCLUDE_DIRS})
target_link_libraries(${AFS_CLIENT_EXECUTABLE} ${FUSE_LIBRARIES} afs_grpc_proto ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

add_executable(${AFS_SERVER_EXECUTABLE} afs_server.cpp)
target_link_libraries(${AFS_SERVER_EXECUTABLE} afs_grpc_proto ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

target_compile_options(${AFS_CLIENT_EXECUTABLE} PUBLIC -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wno-unused-parameter)
target_compile_options(${AFS_CLIENT_EXECUTABLE} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++11 -D_FILE_OFFSET_BITS=64>)

target_compile_options(${AFS_SERVER_EXECUTABLE} PUBLIC -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wno-unused-parameter)
target_compile_options(${AFS_SERVER_EXECUTABLE} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++11 -D_FILE_OFFSET_BITS=64>)

check_function_exists(fallocate HAVE_FALLOCATE)
check_function_exists(fallocate HAVE_FLOCK)
check_function_exists(utimensat HAVE_UTIMENSAT)
check_function_exists(setxattr HAVE_XATTR)
if (${HAVE_FALLOCATE})
    target_compile_definitions(${AFS_CLIENT_EXECUTABLE} PUBLIC HAVE_FALLOCATE)
endif ()
if (${HAVE_FLOCK})
    target_compile_definitions(${AFS_CLIENT_EXECUTABLE} PUBLIC HAVE_FLOCK)
endif ()
if (${HAVE_UTIMENSAT})
    target_compile_definitions(${AFS_CLIENT_EXECUTABLE} PUBLIC HAVE_UTIMENSAT)
endif ()
if (${HAVE_XATTR})
    target_compile_definitions(${AFS_CLIENT_EXECUTABLE} PUBLIC HAVE_XATTR)
endif ()

add_subdirectory(tests)

install(TARGETS ${AFS_CLIENT_EXECUTABLE} ${AFS_SERVER_EXECUTABLE} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES "unreliablefs.1" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
install(FILES "unreliablefs.conf.5" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man5)
