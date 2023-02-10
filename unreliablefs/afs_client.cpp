#include<grpc++/grpc++.h>
#include "afs.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

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

            Status status = stub_->Fetch(&context, path, &data);

            if(status.ok() && data.status().success()){
                return data.contents();
            } else if(status.ok()){
                return data.status().err_message();  
            } else {
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

    int a = 5;
    int b = 10;
    std::string filepath = "./README.md";
    response = client.Fetch(filepath);
    std::cout << response << std::endl;
}

extern "C" {
    void suhas_runs() {
        Run();
    }
}
