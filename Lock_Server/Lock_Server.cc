#include <iostream>
#include <vector>
#include <algorithm>
#include "../myRPC/rpc_server.h"
#include "../myRPC/rpc_controller.h"
#include "../myRPC/rpc_channel.h"
#include "Lock_message.pb.h"

using namespace myRPC::Lock;
using namespace myRPC;

#define PROTOBUF_NAMESPACE_ID google::protobuf
class Lock_ServerImpl : public Lock_Server {
public:
    Lock_ServerImpl() {};
    virtual ~Lock_ServerImpl() {};
    std::vector<std::string> fnames;
private:
    virtual void Lock(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Lock::LockInfo_Requst* request,
        ::myRPC::Lock::Lock_Response* response,
        ::google::protobuf::Closure* done) {

        auto iterator = find(fnames.begin(), fnames.end(), request->filename());
        if (iterator != fnames.end()) {
            response->set_flag(2);
        }
        else {
            response->set_flag(1);
            fnames.push_back(request->filename());
        }
        done->Run();
    }

    virtual void unLock(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Lock::unLockInfo_Requst*request,
        ::myRPC::Lock::Lock_Response*response,
        ::google::protobuf::Closure* done) {
        
        fnames.erase(std::remove(fnames.begin(), fnames.end(), request->filename()));
        response->set_flag(1);
        done->Run();
    }
};

int main(int argc, char* argv[])
{
    RpcServer rpc_server;

    Lock_Server* pLockServer = new Lock_ServerImpl();
    if (!rpc_server.RegisterService(pLockServer, false)) {
        std::cout << "error: register service failed" << std::endl;
        return -1;
    }

    std::string server_addr("0.0.0.0:11112"); //¼àÌý0.0.0.0

    if (!rpc_server.Start(server_addr)) {
        std::cout << "error: start server failed" << std::endl;
        return -1;
    }
    else {

    }
    return 0;
}