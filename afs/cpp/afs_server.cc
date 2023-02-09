#include <grpc++/grpc++.h>
#include "afs.grpc.pb.h"
#include <string>
#include <fstream>
#include <iostream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using afs::Path;
using afs::FileData;
using afs::IOResult;
using afs::AfsService;

class AfsServiceImpl final : public AfsService::Service{
    Status Fetch(ServerContext* context, const Path* filepath, FileData* filedata){
        std::string pathname = filepath->path();
        IOResult* result = new IOResult;
        std::ifstream in;
        in.open(pathname);
        if(in.fail()){
            std::cout<<"Failed";
            result->set_success(false);
            result->set_err_message("File open failed");
            return Status::OK;
        }
        result->set_success(true);
        std::string data = "";
        std::string line = "";
        while(getline (in, line)){
            data+=line + "\n";
        }
        std :: cout<<data;
        filedata->set_contents(data);
        filedata->set_allocated_status(result);
        return Status::OK;
    }

    Status Store(ServerContext* context, FileData* data, IOResult* result){
       result->set_success(true);
       result->set_err_message("No error");
       return Status::OK;
    }
};

void RunServer() {
    std::string server_address{"0.0.0.0:5000"};
    AfsServiceImpl service;

    // Build server
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server{builder.BuildAndStart()};

    // Run server
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}


