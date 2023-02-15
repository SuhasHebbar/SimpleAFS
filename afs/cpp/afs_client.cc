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

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using afs::Path;
using afs::FileData;
using afs::IOResult;
using afs::AfsService;
using afs::StoreData;
using afs::StoreResult;
using afs::RenameArgs;
using afs::DirectoryData;
using afs::AuthData;
using afs::StatData;

using google::protobuf::Timestamp;

static char CLIENT_PATH[] = "/home/aliasgar/CS739-P1/afs/cpp/.cache/";

class AfsClient{
    public:
        AfsClient(std::shared_ptr<Channel> channel) : stub_{AfsService::NewStub(channel)} {}

        int Fetch(std::string filepath, std::string& response){
            Path path;
            path.set_name(filepath);
            FileData data;
            ClientContext context;

            std::unique_ptr<ClientReader<FileData> > reader(
            stub_->Fetch(&context, path));
            
            while(reader->Read(&data)){
                if(!data.status().success()){
                    response.clear();
                    response = data.status().err_message();
                    return -1;
                }
                else{
                    response += data.contents();
                }
            }  
            Status status = reader->Finish();
            if(status.ok()){
                return 0;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                return -1;
            }
        }

        StoreResult Store(std::string filepath, std::string filedata){
            ClientContext context;
            StoreData storedata;
            Path* path =  new Path;
            path->set_name(filepath);
            storedata.set_allocated_path(path);
            FileData* data = new FileData;
            data->set_contents(filedata);
            storedata.set_allocated_filedata(data);
            StoreResult result;
            Status status = stub_->Store(&context, storedata, &result);
            if(status.ok()){
                return result;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return result;
            }
        }

        IOResult Remove(std::string filepath){
            ClientContext context;
            Path path;
            path.set_name(filepath);
            IOResult result;
            Status status = stub_->Remove(&context, path, &result);
            if(status.ok()){
                return result;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return result;
            }
        }

        IOResult Create(std::string filepath){
            ClientContext context;
            Path path;
            path.set_name(filepath);
            IOResult result;
            Status status = stub_->Create(&context, path, &result);
            if(status.ok()){
                return result;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return result;
            }
        }

        IOResult Rename(std::string oldname, std::string newname){
            ClientContext context;
            Path* oldpath = new Path;
            oldpath->set_name(oldname);
            Path* newpath = new Path;
            newpath->set_name(newname);
            RenameArgs args;
            args.set_allocated_oldname(oldpath);
            args.set_allocated_newname(newpath);
            IOResult result;
            Status status = stub_->Rename(&context, args, &result);
            if(status.ok()){
                return result;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return result;
            }
        }

        IOResult Makedir(std::string filepath){
            ClientContext context;
            Path path;
            path.set_name(filepath);
            IOResult result;
            Status status = stub_->Makedir(&context, path, &result);
            if(status.ok()){
                return result;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return result;
            }
        }

        IOResult Removedir(std::string filepath){
            ClientContext context;
            Path path;
            path.set_name(filepath);
            IOResult result;
            Status status = stub_->Removedir(&context, path, &result);
            if(status.ok()){
                return result;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return result;
            }
        }

        AuthData TestAuth(std::string filepath){
            ClientContext context;
            Path path;
            path.set_name(filepath);
            AuthData authdata;
            Status status = stub_->TestAuth(&context, path, &authdata);
            if(status.ok()){
                return authdata;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return authdata;
            }
        }

        std::vector<std::string> GetFileStat(std::string filepath){
            ClientContext context;
            Path path;
            path.set_name(filepath);
            StatData statdata;
            Status status = stub_->GetFileStat(&context, path, &statdata);
            std::vector<std::string> files;
            if(status.ok()){
                if(statdata.status().success()){
                 int size = statdata.dd().files_size();
                 for(int i=0;i<size;i++){
                    files.push_back(statdata.dd().files(i));
                 }
                }
                else{
                   files.push_back(statdata.status().err_message()); 
                }
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                files.push_back(statdata.status().err_message());
            }
            return files;
        } 

    private:
        std::unique_ptr<AfsService::Stub> stub_;
};

int store_in_cache(const char* pathname, const char* filedata){
    // Writing to a temp file
    char tempPath[10000] = {'\0'};
    strcpy(tempPath,CLIENT_PATH);
    strcat(tempPath,"/tmp/temp.XXXXXX");
    int tmpfd = mkstemp(tempPath);
    if(tmpfd == -1){
        std::cout<<"Error with creating temporary file"<<"\n";
        return -1;
    }
    if(write(tmpfd, filedata, strlen(filedata)) == -1){
        std::cout<<"Error with writing temporary file"<<"\n";
        close(tmpfd);
        unlink(tempPath);
        return -1;
    }

   // Swapping temp file with target file
    
    char lpathname[10000] = {'\0'};
    strcpy(lpathname, CLIENT_PATH);
    int i = 0, j = strlen(lpathname);
    if(pathname[0] == '/')
      i++;
    while(pathname[i] != '\0'){     
     lpathname[j] = pathname[i];
     if(pathname[i] == '/'){
        if((mkdir(lpathname, 00664) == -1) && errno!=17){
            std::cout<<"Error with creating target file"<<"\n";
            return -1;
        }
     }
     i++;
     j++;     
    }

    if(creat(lpathname, 00664) == -1){
        std::cout<<strerror(errno)<<"\n";
        std::cout<<"Error with creating target file"<<"\n";
        return -1;
     }

    if(rename(tempPath, lpathname) == -1){
        std::cout<<"Error with renaming target file"<<"\n";
        return -1;
    }

    close(tmpfd);
    unlink(tempPath);
    return 0;    
}

int afs_open(AfsClient* client, const char* pathname, int flags){
    char lpathname[10000] = {'\0'};
    strcpy(lpathname, CLIENT_PATH);
    if(pathname[0] == '/'){
      strcat(lpathname, pathname+1);
    }
    else{
      strcat(lpathname, pathname);      
    }

    if(access(lpathname, F_OK) == -1){
        std :: string filedata;
        if(client->Fetch(pathname, filedata) == -1)
            return -1;
        if(store_in_cache(pathname, filedata.c_str()) == -1){
            return -1;
        }
    }
    else{
        struct stat* statbuf = new struct stat;
        if(stat(lpathname, statbuf) == -1){
            return -1;
        }
        AuthData authdata = client->TestAuth(pathname);
        if(!authdata.status().success())
            return -1;
        if(authdata.lmtime().seconds() != statbuf->st_mtim.tv_sec){
            std :: string filedata;
            if(client->Fetch(pathname, filedata) == -1)
                return -1;
            if(store_in_cache(pathname, filedata.c_str()) == -1){
                return -1;
            }
        }
    }
    int fd = open(lpathname, flags);
    return fd;
}

int afs_close(AfsClient* client, int fd, const char* pathname){
    char lpathname[10000] = {'\0'};
    strcpy(lpathname, CLIENT_PATH);
    if(pathname[0] == '/'){
      strcat(lpathname, pathname+1);
    }
    else{
      strcat(lpathname, pathname);      
    }
    int block_size = 8000;
    FILE* fp;
    fp = fopen(lpathname, "r");
    if(fp == NULL)
      return -1;
    std::string filedata = "";  
    while(!feof(fp)){
        std::string data(block_size, '\0');
        fread(&data[0], block_size, 1, fp);
        if(ferror(fp))
           return -1;
        filedata += data;
    }
    fclose(fp);
    StoreResult result = client->Store(pathname, filedata);
    struct timespec times[2];
    // Don't change the last access time...
    times[0].tv_nsec = UTIME_OMIT;
    // Set last modified time to be the same as server
    times[1].tv_nsec = result.lmtime().nanos();
    times[1].tv_sec = result.lmtime().seconds();
    if(futimens(fd, times) == -1){
        return -1;
    }
    return 0;
}

void Run() {
    std::string address("0.0.0.0:5000");
    AfsClient* client = new AfsClient(
        grpc::CreateChannel(
            address, 
            grpc::InsecureChannelCredentials()
        )
    );
    
    int fd = afs_open(client, "/tmp/temp1/hello_uw.txt", O_RDONLY);
    afs_close(client, fd, "/tmp/temp1/hello_uw.txt");
    
    // // Fetch
    // std::cout << "Fetch..." << std::endl;
    // std::string response;
    // std::string filepath = "../../README.md";
    // response = client.Fetch(filepath);
    // std::cout << response << std::endl;

    // //Store
    // std::cout << "Store..." << std::endl;
    // IOResult result = client->Store("/home/aliasgar/CS739-P1/afs/cpp/hello_world.txt", "Hello World"); 
    // if(result.success()){
    //     std::cout<<"Sucess"<<"\n";
    // }
    // else{
    //     std::cout<<"Failure"<<"\n";
    //     std::cout<<result.err_message();
    // }

    // //Remove
    // std::cout << "Remove..." << std::endl;
    // result = client.Remove("/home/aliasgar/CS739-P1/afs/cpp/hello_world.txt"); 
    // if(result.success()){
    //     std::cout<<"Sucess"<<"\n";
    // }
    // else{
    //     std::cout<<"Failure"<<"\n";
    //     std::cout<<result.err_message();
    // }

    // //Create
    // std::cout << "Create..." << std::endl;
    // result = client.Create("/home/aliasgar/CS739-P1/afs/cpp/hello_wisc.txt");
    // if(result.success()){
    //     std::cout<<"Sucess"<<"\n";
    // }
    // else{
    //     std::cout<<"Failure"<<"\n";
    //     std::cout<<result.err_message();
    // }

    // //Rename
    // std::cout << "Rename..." << std::endl;
    // result = client.Rename("/home/aliasgar/CS739-P1/afs/cpp/hello_wisc.txt", "/home/aliasgar/CS739-P1/afs/cpp/hello_uw.txt");
    // if(result.success()){
    //     std::cout<<"Sucess"<<"\n";
    // }
    // else{
    //     std::cout<<"Failure"<<"\n";
    //     std::cout<<result.err_message();
    // }

    // //Makedir
    // std::cout << "Makedir..." << std::endl;
    // result = client.Makedir("/home/aliasgar/CS739-P1/afs/cpp/temp");
    // if(result.success()){
    //     std::cout<<"Sucess"<<"\n";
    // }
    // else{
    //     std::cout<<"Failure"<<"\n";
    //     std::cout<<result.err_message();
    // }

    // //Removedir
    // std::cout << "Removedir..." << std::endl;
    // result = client.Removedir("/home/aliasgar/CS739-P1/afs/cpp/temp");
    // if(result.success()){
    //     std::cout<<"Sucess"<<"\n";
    // }
    // else{
    //     std::cout<<"Failure"<<"\n";
    //     std::cout<<result.err_message();
    // }

    // // TestAuth
    // std::cout << "TestAuth..." << std::endl;
    // AuthData authdata = client.TestAuth("/home/aliasgar/CS739-P1/afs/cpp/hello_uw.txt");
    // if(authdata.status().success()){
    //     std::cout<<"Sucess"<<"\n";
    //     std::cout<<authdata.lmtime()<<"\n";
    // }
    // else{
    //     std::cout<<"Failure"<<"\n";
    //     std::cout<<authdata.status().err_message();
    // }

    // // GetFileStat
    // std::cout << "GetFileStat..." << std::endl;
    // std::vector<std::string> files = client.GetFileStat("/home/aliasgar/CS739-P1/afs/cpp");
    // for(int i=0;i<files.size();i++){
    //     std::cout<<files[i]<<std::endl;
    // }
}

int main(int argc, char** argv) {
    Run();
    return 0;
}