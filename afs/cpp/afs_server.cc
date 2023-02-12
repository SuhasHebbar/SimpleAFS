#include <grpc++/grpc++.h>
#include "afs.grpc.pb.h"
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using afs::Path;
using afs::FileData;
using afs::IOResult;
using afs::AfsService;
using afs::StoreData;

class AfsServiceImpl final : public AfsService::Service{
    Status Fetch(ServerContext* context, const Path* path, ServerWriter<FileData>* writer){
        std::string pathname = path->name();
        int block_size = 8000;
        FILE* fp;
        fp = fopen(pathname.c_str(), "r");
        if(fp == NULL){
            IOResult* result = new IOResult;
            result->set_success(false);
            result->set_err_message("Error while opening file");
            FileData filedata;
            filedata.set_allocated_status(result);
            writer->Write(filedata);
            return Status::OK;
        }
        while(!feof(fp)){
            IOResult* result = new IOResult;
            FileData filedata;
            std::string data(block_size, '\0');
            fread(&data[0], block_size, 1, fp);
            if(ferror(fp)){
                result->set_success(false);
                result->set_err_message("Error while reading file");
                filedata.set_allocated_status(result);
                writer->Write(filedata);
                return Status::OK;
            }
            filedata.set_contents(data);
            result->set_success(true);
            filedata.set_allocated_status(result);
            writer->Write(filedata);
        }
        fclose(fp);
        return Status::OK;
    }

    Status Store(ServerContext* context, const StoreData* storedata, IOResult* result){
        std::string targetPath = (storedata->path()).name();
        int targetfd = open(targetPath.c_str(), O_RDWR | O_CREAT, S_IRWXU);
        if(targetfd == -1){
            result->set_success(false);
            result->set_err_message("Error while creating or opening target file");
            return Status::OK;
        }
        std::string data = (storedata->filedata()).contents();
        if(write(targetfd, &data[0], data.size()) == -1){
            result->set_success(false);
            result->set_err_message("Error while writing to target file");
            return Status::OK;
        }
        close(targetfd);
        result->set_success(true);
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


