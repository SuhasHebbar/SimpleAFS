#include <grpc++/grpc++.h>
#include "common.h"
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
using grpc::ServerReader;
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

// Disable debug logs in release builds
#ifdef NDEBUG
#  define D(x)
#else
#  define D(x) x
#endif

static const int BLOCK_SIZE = 1 << 13;
static char iobuf[BLOCK_SIZE];

class AfsServiceImpl final : public AfsService::Service{
    Status Fetch(ServerContext* context, const Path* path, ServerWriter<FileData>* writer){
        std::string pathname = basedir_ + "/" + path->name();
        FILE* fp;
        fp = fopen(pathname.c_str(), "r");
        if(fp == NULL){
            int err_code = errno;
            FileData filedata;
            auto status = filedata.mutable_status();
            status->set_success(false);
            status->set_err_code(err_code);
            writer->Write(filedata);
            return Status::OK;
        }
        while(!feof(fp)){
            FileData filedata;
            auto status = filedata.mutable_status();
            int datalen = fread(iobuf, 1, sizeof(iobuf), fp);
            if(ferror(fp)){
                status->set_err_code(errno);
                status->set_success(false);
                writer->Write(filedata);
		fclose(fp);
                return Status::OK;
            }
            status->set_success(true);
            filedata.set_contents(std::string(iobuf, datalen));
            writer->Write(filedata);
        }
        // TODO: Handle close failure
        fclose(fp);
        return Status::OK;
    }

    Status Store(ServerContext* context, ServerReader<StoreData> *reader, StoreResult* result){
        // Read filepath of the contents
        StoreData metadata;
        if (!reader->Read(&metadata)) {
            auto status = result->mutable_status();
            status->set_success(false);
            status->set_err_code(ENOLINK);
            return Status::OK;
        }
        std::string targetPath = basedir_ + "/" + metadata.path().name();

        // Writing to a temp file
        char tempPath[] = "/tmp/afs/server/store.XXXXXX";
        int tmpfd = mkstemp(tempPath);
        if(tmpfd == -1){
            auto status = result->mutable_status();
            status->set_err_code(errno);
            status->set_success(false);
            return Status::OK;
        }
	
	bool streamStoreSuccess = false;
        StoreData storedata;
        while (reader->Read(&storedata)) {
            const char* data = storedata.filedata().contents().c_str();
            auto datalen = storedata.filedata().contents().size();
	    streamStoreSuccess = storedata.filedata().status().success();
            if (write(tmpfd, data, datalen) < 0) {
                int err_code = errno;
                close(tmpfd);
                unlink(tempPath);
                auto status = result->mutable_status();
                status->set_success(false);
                status->set_err_code(err_code);
                return Status::OK;
            }
        }

        close(tmpfd);

        if (!streamStoreSuccess || rename(tempPath, targetPath.c_str()) < 0) {
            int err_code = errno;
            unlink(tempPath);
            auto status = result->mutable_status();
            status->set_success(false);
            status->set_err_code(err_code);
            return Status::OK;
        }

        struct stat statbuf;
        if (stat(targetPath.c_str(), &statbuf) < 0) {
            auto status = result->mutable_status();
            status->set_err_code(errno);
            status->set_success(false);
            unlink(targetPath.c_str());
            return Status::OK;
        }

        auto status = result->mutable_status();
        status->set_success(true);
        auto lmtime = result->mutable_lmtime();
#ifdef __APPLE__
        lmtime->set_seconds(statbuf.st_mtime);
        lmtime->set_nanos(0);
#else
        lmtime->set_seconds(statbuf.st_mtim.tv_sec);
        lmtime->set_nanos(statbuf.st_mtim.tv_nsec);
#endif
        return Status::OK;
    }

    Status Remove(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = basedir_ + "/" + path->name();
        if(unlink(pathname.c_str()) == -1){
            result->set_err_code(errno);
            result->set_success(false);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status Create(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = basedir_ + "/" + path->name();
	int fd = creat(pathname.c_str(), 0744);
        if (fd < 0) {
            result->set_err_code(errno);
            result->set_success(false);
            return Status::OK;
        }
	close(fd);
        result->set_success(true);
        return Status::OK;
    }

    Status Rename(ServerContext* context, const RenameArgs* args, IOResult* result){
        std::string oldpath = basedir_ + "/" + (args->oldname()).name();
        std::string newpath = basedir_ + "/" + (args->newname()).name();
        if (rename(oldpath.c_str(), newpath.c_str()) < 0) {
            result->set_err_code(errno);
            result->set_success(false);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status Makedir(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = basedir_ + "/" + path->name();
        if (mkdir(pathname.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            result->set_err_code(errno);
            result->set_success(false);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status Removedir(ServerContext* context, const Path* path, IOResult* result){
        std::string pathname = basedir_ + "/" + path->name();
        if (rmdir(pathname.c_str()) < 0) {
            result->set_err_code(errno);
            result->set_success(false);
            return Status::OK;
        }
        result->set_success(true);
        return Status::OK;
    }

    Status TestAuth(ServerContext* context, const Path* path, AuthData* authdata){
        std::string pathname = basedir_ + "/" + path->name();
        struct stat statbuf;
        auto status = authdata->mutable_status();
        if (stat(pathname.c_str(), &statbuf) < 0) {
            status->set_err_code(errno);
            status->set_success(false);
            return Status::OK;
        }
        status->set_success(true);
        auto lmtime = authdata->mutable_lmtime();
#ifdef __APPLE__
        lmtime->set_seconds(statbuf.st_mtime);
        lmtime->set_nanos(0);
#else
        lmtime->set_seconds(statbuf.st_mtim.tv_sec);
        lmtime->set_nanos(statbuf.st_mtim.tv_nsec);
#endif
        authdata->set_mode(statbuf.st_mode);
        return Status::OK;
    }

    Status GetFileStat(ServerContext* context, const Path* path, StatData* statdata) {
        std::string pathname = basedir_ + "/" + path->name();
        struct dirent* dentry;

        DIR* dr = opendir(pathname.c_str());
        if(dr == NULL){
            auto status = statdata->mutable_status();
            status->set_err_code(errno);
            status->set_success(false);
            return Status::OK;
        }
        auto dd = statdata->mutable_dd();
        while((dentry = readdir(dr)) != NULL){
            dd->add_files(dentry->d_name);
        }
        closedir(dr);
        auto status = statdata->mutable_status();
        status->set_success(true);
        return Status::OK;
    }

    std::string basedir_;

public:
    AfsServiceImpl(std::string const& basedir) : basedir_(basedir) {}
};

void RunServer(std::string const& basedir, std::string const& addr) {
    AfsServiceImpl service{basedir};

    // Build server
    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server{builder.BuildAndStart()};

    // Run server
    std::cout << "Server listening on " << addr << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cout << "Usage: ./server <basedir> [<addr>]" << std::endl;
        return EXIT_FAILURE;
    }

    if (mkdir("/tmp/afs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
   	perror("tmp setup");
	return EXIT_FAILURE;
    }

    if (mkdir("/tmp/afs/server", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
   	perror("tmp setup");
	return EXIT_FAILURE;
    }

    std::string basedir{argv[1]};
    std::string addr = "0.0.0.0:5000";
    if (argc > 2) {
        addr = std::string(argv[2]);
    }
    RunServer(basedir, addr);
    return 0;
}


