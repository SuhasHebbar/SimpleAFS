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
#include <dirent.h>

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
using afs::RenameArgs;
using afs::DirectoryData;
using afs::StatData;

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
        int targetfd = open(targetPath.c_str(), O_RDWR | O_CREAT, 00664);
        if(targetfd == -1){
            result->set_success(false);
            result->set_err_message("Error while creating or opening target file");
            return Status::OK;
        }
        std::string data = (storedata->filedata()).contents();
        if(write(targetfd, &data[0], data.size()) == -1){
            result->set_success(false);
            result->set_err_message("Error while writing to target file");
            close(targetfd);
            return Status::OK;
        }
        close(targetfd);
        result->set_success(true);
        return Status::OK;     
    }

    Status Remove(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = path->name();
        if(unlink(pathname.c_str()) == -1){
            result->set_success(false);
            result->set_err_message("Error while removing file");
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;  
    }

    Status Create(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = path->name();
        if(creat(pathname.c_str(), 00664) == -1){
            result->set_success(false);
            result->set_err_message("Error while creating file");
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;  
    }

    Status Rename(ServerContext* context, const RenameArgs* args, IOResult* result){
        std::string oldpath = (args->oldname()).name();
        std::string newpath = (args->newname()).name();
        if(rename(oldpath.c_str(), newpath.c_str()) == -1){
            result->set_success(false);
            result->set_err_message("Error while renaming file");
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status Makedir(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = path->name();
        if(mkdir(pathname.c_str(), 0664) == -1){
            result->set_success(false);
            result->set_err_message("Error while creating directory");
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status Removedir(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = path->name();
        if(rmdir(pathname.c_str()) == -1){
            result->set_success(false);
            result->set_err_message("Error while removing directory");
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status GetFileStat(ServerContext* context, const Path* path, StatData* statdata ){
        std::string pathname = path->name();
        struct dirent* dentry;
        
        DIR* dr = opendir(pathname.c_str());
        if(dr == NULL){
            IOResult* result = new IOResult;
            result->set_success(false);
            result->set_err_message("Error while opening directory");
            statdata->set_allocated_status(result);
            return Status::OK;
        }
        DirectoryData* dd = new DirectoryData;
        while((dentry = readdir(dr)) != NULL){
            dd->add_files(dentry->d_name);
        }
        statdata->set_allocated_dd(dd);
        closedir(dr);
        IOResult* result = new IOResult;
        result->set_success(true);
        statdata->set_allocated_status(result);
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


