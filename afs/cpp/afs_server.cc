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
using afs::StoreResult;
using afs::RenameArgs;
using afs::AuthData;
using afs::DirectoryData;
using afs::StatData;

using google::protobuf::Timestamp;

static char SERVER_PATH[] = "/home/aliasgar/CS739-P1/afs/cpp/";

class AfsServiceImpl final : public AfsService::Service{
    Status Fetch(ServerContext* context, const Path* path, ServerWriter<FileData>* writer){
        std::string pathname = SERVER_PATH + path->name();
        int block_size = 8000;
        FILE* fp;
        fp = fopen(pathname.c_str(), "r");
        if(fp == NULL){
            IOResult* result = new IOResult;
            result->set_success(false);
            std :: string err_message = "Error while opening file: ";
            err_message.append(strerror(errno)); 
            result->set_err_message(err_message);
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
                std :: string err_message = "Error while reading file: ";
                err_message.append(strerror(errno)); 
                result->set_err_message(err_message);
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

    Status Store(ServerContext* context, const StoreData* storedata, StoreResult* result){
        // Writing to a temp file
        char tempPath[10000] = {'\0'};
        strcpy(tempPath,SERVER_PATH);
        strcat(tempPath,"/tmp/temp.XXXXXX");
        int tmpfd = mkstemp(tempPath);
        IOResult* status = new IOResult;
        if(tmpfd == -1){
            status->set_success(false); 
            std :: string err_message = "Error while creating and opening a temp file: ";
            err_message.append(strerror(errno)); 
            status->set_err_message(err_message);
            result->set_allocated_status(status);
            return Status::OK;
        }
        const char* data = (storedata->filedata()).contents().c_str();
        if(write(tmpfd, data, strlen(data)) == -1){
            close(tmpfd);
            unlink(tempPath);
            status->set_success(false);
            std :: string err_message = "Error while writing to temp file: ";
            err_message.append(strerror(errno));
            status->set_err_message(err_message);
            result->set_allocated_status(status);
            return Status::OK;
        }

        // Swapping temp file with target file
        std::string targetPath = SERVER_PATH + (storedata->path()).name();
        if(creat(targetPath.c_str(), 00664) == -1){
            close(tmpfd);
            unlink(tempPath);
            status->set_success(false);
            std :: string err_message = "Error while creating target file: ";
            err_message.append(strerror(errno));
            status->set_err_message(err_message);
            result->set_allocated_status(status);
            return Status::OK;
        }
        if(rename(tempPath, targetPath.c_str()) == -1){
            close(tmpfd);
            unlink(tempPath);
            status->set_success(false);
            std :: string err_message = "Error while swapping target file: ";
            err_message.append(strerror(errno));
            status->set_err_message(err_message);
            result->set_allocated_status(status);
            return Status::OK;
        }
        
        close(tmpfd);
        unlink(tempPath);

        struct stat* statbuf = new struct stat;
        if(stat(targetPath.c_str(), statbuf) == -1){
            status->set_success(false);
            std :: string err_message = "Error while retrieving modified time: ";
            err_message.append(strerror(errno));
            status->set_err_message(err_message);
            result->set_allocated_status(status);
            return Status::OK;
        }
        status->set_success(true);
        result->set_allocated_status(status);
        Timestamp* lmtime = new Timestamp;
        lmtime->set_seconds(statbuf->st_mtim.tv_sec);
        lmtime->set_nanos(statbuf->st_mtim.tv_nsec);
        result->set_allocated_lmtime(lmtime);
        return Status::OK;   
    }

    Status Remove(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = SERVER_PATH + path->name();
        if(unlink(pathname.c_str()) == -1){
            result->set_success(false);
            std :: string err_message = "Error while removing file: ";
            err_message.append(strerror(errno));
            result->set_err_message(err_message);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;  
    }

    Status Create(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = SERVER_PATH + path->name();
        if(creat(pathname.c_str(), 00664) == -1){
            result->set_success(false);
            std :: string err_message = "Error while creating file: ";
            err_message.append(strerror(errno));
            result->set_err_message(err_message);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;  
    }

    Status Rename(ServerContext* context, const RenameArgs* args, IOResult* result){
        std::string oldpath = SERVER_PATH + (args->oldname()).name();
        std::string newpath = SERVER_PATH + (args->newname()).name();
        if(rename(oldpath.c_str(), newpath.c_str()) == -1){
            result->set_success(false);
            std :: string err_message = "Error while renaming file: ";
            err_message.append(strerror(errno));
            result->set_err_message(err_message);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status Makedir(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = SERVER_PATH + path->name();
        if(mkdir(pathname.c_str(), 0664) == -1){
            result->set_success(false);
            std :: string err_message = "Error while creating directory: ";
            err_message.append(strerror(errno));
            result->set_err_message(err_message);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status Removedir(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = SERVER_PATH + path->name();
        if(rmdir(pathname.c_str()) == -1){
            result->set_success(false);
            std :: string err_message = "Error while removing directory: ";
            err_message.append(strerror(errno));
            result->set_err_message(err_message);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status TestAuth(ServerContext* context, const Path* path, AuthData* authdata){
        std::string pathname = SERVER_PATH + path->name();
        struct stat* statbuf = new struct stat;
        IOResult* result = new IOResult;
        if(stat(pathname.c_str(), statbuf) == -1){
            result->set_success(false);
            std :: string err_message = "Error while retrieving modified time: ";
            err_message.append(strerror(errno));
            result->set_err_message(err_message);
            authdata->set_allocated_status(result);
            return Status::OK;
        }
        result->set_success(true);
        authdata->set_allocated_status(result);
        Timestamp* lmtime = new Timestamp;
        lmtime->set_seconds(statbuf->st_mtim.tv_sec);
        lmtime->set_nanos(statbuf->st_mtim.tv_nsec);
        authdata->set_allocated_lmtime(lmtime);
        return Status::OK;
    }

    Status GetFileStat(ServerContext* context, const Path* path, StatData* statdata ){
        std::string pathname = SERVER_PATH + path->name();
        struct dirent* dentry;
        
        DIR* dr = opendir(pathname.c_str());
        if(dr == NULL){
            IOResult* result = new IOResult;
            result->set_success(false);
            std :: string err_message = "Error while opening directory: ";
            err_message.append(strerror(errno));
            result->set_err_message(err_message);
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


