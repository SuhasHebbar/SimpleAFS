#include <grpc++/grpc++.h>
#include "afs.grpc.pb.h"
#include <fstream>
#include <cstdio>
#include <vector>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using afs::Path;
using afs::FileData;
using afs::IOResult;
using afs::AfsService;
using afs::StoreData;
using afs::RenameArgs;
using afs::DirectoryData;
using afs::StatData;

class AfsClient{
    public:
        AfsClient(std::shared_ptr<Channel> channel) : stub_{AfsService::NewStub(channel)} {}

        std::string Fetch(std::string filepath){
            Path path;
            path.set_name(filepath);
            FileData data;
            ClientContext context;

            std::unique_ptr<ClientReader<FileData> > reader(
            stub_->Fetch(&context, path));

            std::string response = "";

            while(reader->Read(&data)){
                if(!data.status().success()){
                    response.clear();
                    response = data.status().err_message();
                }
                else{
                    response += data.contents();
                }
            }
            Status status = reader->Finish();
            if(status.ok()){
                return response;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                return "Everything Sucks";
            }
        }

        IOResult Store(std::string filepath, std::string filedata){
            ClientContext context;
            StoreData storedata;
            Path* path =  new Path;
            path->set_name(filepath);
            storedata.set_allocated_path(path);
            FileData* data = new FileData;
            data->set_contents(filedata);
            storedata.set_allocated_filedata(data);
            IOResult result;
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

void Run() {
    std::string address("0.0.0.0:5000");
    AfsClient client(
        grpc::CreateChannel(
            address,
            grpc::InsecureChannelCredentials()
        )
    );

    // Fetch
    std::cout << "Fetch..." << std::endl;
    std::string response;
    std::string filepath = "../../README.md";
    response = client.Fetch(filepath);
    std::cout << response << std::endl;

    //Store
    std::cout << "Store..." << std::endl;
    IOResult result = client.Store("/home/aliasgar/CS739-P1/afs/cpp/hello_world.txt", "Hello World"); 
    if(result.success()){
        std::cout<<"Sucess"<<"\n";
    }
    else{
        std::cout<<"Failure"<<"\n";
        std::cout<<result.err_message();
    }

    //Remove
    std::cout << "Remove..." << std::endl;
    result = client.Remove("/home/aliasgar/CS739-P1/afs/cpp/hello_world.txt"); 
    if(result.success()){
        std::cout<<"Sucess"<<"\n";
    }
    else{
        std::cout<<"Failure"<<"\n";
        std::cout<<result.err_message();
    }

    //Create
    std::cout << "Create..." << std::endl;
    result = client.Create("/home/aliasgar/CS739-P1/afs/cpp/hello_wisc.txt");
    if(result.success()){
        std::cout<<"Sucess"<<"\n";
    }
    else{
        std::cout<<"Failure"<<"\n";
        std::cout<<result.err_message();
    }

    //Rename
    std::cout << "Rename..." << std::endl;
    result = client.Rename("/home/aliasgar/CS739-P1/afs/cpp/hello_wisc.txt", "/home/aliasgar/CS739-P1/afs/cpp/hello_uw.txt");
    if(result.success()){
        std::cout<<"Sucess"<<"\n";
    }
    else{
        std::cout<<"Failure"<<"\n";
        std::cout<<result.err_message();
    }

    //Makedir
    std::cout << "Makedir..." << std::endl;
    result = client.Makedir("/home/aliasgar/CS739-P1/afs/cpp/temp");
    if(result.success()){
        std::cout<<"Sucess"<<"\n";
    }
    else{
        std::cout<<"Failure"<<"\n";
        std::cout<<result.err_message();
    }

    //Removedir
    std::cout << "Removedir..." << std::endl;
    result = client.Removedir("/home/aliasgar/CS739-P1/afs/cpp/temp");
    if(result.success()){
        std::cout<<"Sucess"<<"\n";
    }
    else{
        std::cout<<"Failure"<<"\n";
        std::cout<<result.err_message();
    }

    // GetFileStat
    std::cout << "GetFileStat..." << std::endl;
    std::vector<std::string> files = client.GetFileStat("/home/aliasgar/CS739-P1/afs/cpp");
    for(int i=0;i<files.size();i++){
        std::cout<<files[i]<<std::endl;
    }
}

extern "C" {
    void test_afs() {
        Run();
    }
}

// global variable to maintain the connection state to the server
std::unique_ptr<AfsClient> g_afsClient = nullptr;

void afs_client_init() {
    std::string address("0.0.0.0:5000");
    g_afsClient = std::make_unique<AfsClient>(
        grpc::CreateChannel(
            address,
            grpc::InsecureChannelCredentials()
        )
    );
}

void afs_client_destroy() {
    g_afsClient.reset();
}

// Flags
// - O_CREAT - creates the file if not present
// - O_RDONLY
// - O_WRONLY
// - O_RDWR
//
// Returns
// - a file descriptor if successful
// - an error otherwise
int afs_client_open(const char* filepath, int flags) {
    if (flags != O_RDONLY) return -ENOTSUP;

    int ret;

    // check for cached file
    ret = open(filepath, O_RDONLY);
    if (ret >= 0) return ret;

    // file not cached
    // - open a file on local fs for read-write
    // - read file from remote
    // - write file to local
    // - reset file offset
    ret = open(filepath, O_RDWR);
    if (ret < 0) return -errno;
    int fd = ret;

    // remote fetch
    auto response = g_afsClient->Fetch(std::string(filepath));

    // local write
    ret = write(fd, response.c_str(), response.size());
    if (ret < 0) return -errno;

    // reset
    ret = lseek(fd, 0, SEEK_SET);
    if (ret < 0) return -errno;

    return fd;
}
