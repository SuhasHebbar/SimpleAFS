#include <grpc++/grpc++.h>
#include "afs.grpc.pb.h"
#include <fstream>
#include <cstdio>
#include <memory>

#include <sys/stat.h>
#include <fcntl.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using afs::Path;
using afs::FileData;
using afs::IOResult;
using afs::AfsService;

class AfsClient{
    public:
        AfsClient(std::shared_ptr<Channel> channel) : stub_{AfsService::NewStub(channel)} {}

        std::string Fetch(std::string filepath){
            Path path;
            path.set_path(filepath);
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

    std::string response;

    std::string filepath = "../README.md";
    response = client.Fetch(filepath);
    std::cout << response << std::endl;
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
