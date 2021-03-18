#include <iostream>
#include <vector>
#include "../myRPC/rpc_server.h"
#include "../myRPC/rpc_controller.h"
#include "../myRPC/rpc_channel.h"
#include "Dir_message.pb.h"

#define PROTOBUF_NAMESPACE_ID google::protobuf

using namespace myRPC::DirServer;
using namespace myRPC;
using namespace std;
class Login_ServerImpl : public LoginServer {
public:
    Login_ServerImpl() {};
    virtual ~Login_ServerImpl() {};

    std::vector<std::string> services;

private:
    virtual void Login(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::DirServer::Login_Request* request,
        ::myRPC::DirServer::Login_Response* response,
        ::google::protobuf::Closure* done) {

        std::string tip = request->ip() + ":" + request->port();
        services.push_back(tip);
        
        response->set_flag(1);
        done->Run();
    };
    virtual void Logout(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::DirServer::Logout_Request* request,
        ::myRPC::DirServer::Logout_Response* response,
        ::google::protobuf::Closure* done) {

        std::string tip = request->ip() + ":" + request->port();
        services.erase(std::remove(services.begin(), services.end(), tip), services.end());
        
        response->set_flag(1);
        done->Run();
    };
    virtual void ServerInfo(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::DirServer::ServerInfo_Request* request,
        ::myRPC::DirServer::ServerInfo_Response* response,
        ::google::protobuf::Closure* done) {

        for (int i = 0; i < services.size(); i++) {
            size_t split_pos = services[i].find(':');
            std::string server_id = to_string(i);
            std::string ip = services[i].substr(0, split_pos);
            std::string port = services[i].substr(split_pos + 1);
            DirServer::ServiceInfo* ti = response->add_server_list();
            ti->set_server_id(server_id);
            ti->set_ip(ip);
            ti->set_port(port);
        }
        done->Run();
    };
    
};

int main(int argc, char* argv[])
{
    RpcServer rpc_server;

    LoginServer* pDirServer = new Login_ServerImpl();
    if (!rpc_server.RegisterService(pDirServer, false)) {
        std::cout << "error: register service failed" << std::endl;
        return -1;
    }

    std::string server_addr("0.0.0.0:11111"); //¼àÌý0.0.0.0

    if (!rpc_server.Start(server_addr)) {
        std::cout << "error: start server failed" << std::endl;
        return -1;
    }
    else {
        
    }
    return 0;
}