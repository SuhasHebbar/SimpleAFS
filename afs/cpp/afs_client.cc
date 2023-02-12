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
using afs::StoreData;

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
            //std::cout<<path->name()<<"\n";
            //std::cerr<<"0"<<"\n";
            storedata.set_allocated_path(path);
            FileData* data = new FileData;
            data->set_contents(filedata);
            //std::cout<<data->contents()<<"\n";
            storedata.set_allocated_filedata(data);
            IOResult result;
            //std::cerr<<"1"<<"\n";
            Status status = stub_->Store(&context, storedata, &result);
            if(status.ok()){
                return result;
            }
            else{
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std :: cout << "Everything Sucks"<<std::endl;
                return result;
            }
            //std::cerr<<"2"<<"\n";
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

    std::string filepath = "../../README.md";
    response = client.Fetch(filepath);
    std::cout << response << std::endl;
    IOResult result = client.Store("/home/aliasgar/CS739-P1/afs/cpp/hello_world.txt", "Hello World"); 
    if(result.success()){
        std::cout<<"Sucess"<<"\n";
    }
    else{
        std::cout<<"Failure"<<"\n";
        std::cout<<result.err_message();
    }
}

int main(int argc, char** argv) {
    Run();
    return 0;
}