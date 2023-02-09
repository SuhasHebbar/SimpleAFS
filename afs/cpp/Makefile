LDFLAGS = -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl

CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11

GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH = `which $(GRPC_CPP_PLUGIN)`

all: afs_server afs_client

afs_client: afs.pb.o afs.grpc.pb.o afs_client.o
	$(CXX) $^ $(LDFLAGS) -o $@

afs_server: afs.pb.o afs.grpc.pb.o afs_server.o
	$(CXX) $^ $(LDFLAGS) -o $@

%.grpc.pb.cc: %.proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $< --experimental_allow_proto3_optional

%.pb.cc: %.proto
	protoc --cpp_out=. $< --experimental_allow_proto3_optional

clean:
	rm -f *.o *.pb.cc *.pb.h