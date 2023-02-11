#include <grpc++/grpc++.h>
#include "afs.grpc.pb.h"
#include <fstream>
#include <cstdio>

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

    std::string filepath = "/home/aliasgar/temp/hello_world.txt";
    response = client.Fetch(filepath);
    std::cout << response << std::endl;
}

int main(int argc, char** argv) {
    Run();
    return 0;
}