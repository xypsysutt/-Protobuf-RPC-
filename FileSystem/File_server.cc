#include <iostream>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <vector>
#include <fstream>  
#include <string>    
#include <cmath>
#include <sstream>
#include <cstdio>
#include "../myRPC/rpc_server.h"
#include "../myRPC/rpc_controller.h"
#include "../myRPC/rpc_channel.h"
#include "File_message.pb.h"
#include "../Directory_Server/Dir_message.pb.h"
#include "../Directory_Server/Dir_message.pb.cc"
#include "../Lock_Server/Lock_message.pb.h"
#define PROTOBUF_NAMESPACE_ID google::protobuf
using namespace myRPC::Filesystem;
using namespace myRPC::DirServer;
using namespace myRPC;

# define _ip "127.0.0.1"
# define _port "12322"
// 读取文件
std::string readFileIntoString(const char* filename)
{
    std::ifstream ifile(filename);
    std::ostringstream buf;
    char ch;
    while (buf && ifile.get(ch))
        buf.put(ch);
    return buf.str();
}
class File_ServerImpl : public File_Server {
public:
    File_ServerImpl() {};
    virtual ~File_ServerImpl() {};

private:
    /// response 格式为 repeated string filename
    /// 功能：读取当前目录下所有文件名
    /// 待完善： 文件夹异色
    virtual void ls(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Filesystem::File_Request* request,
        ::myRPC::Filesystem::Dir_Response* response,
        ::google::protobuf::Closure* done) {

        intptr_t Handle;
        struct _finddata_t FileInfo;
        std::string* fname;
        std::string p;
        std::string path = "..";
        if ((Handle = _findfirst(p.assign(path).append("\\*").c_str(), &FileInfo)) == -1)
            printf("NULL\n");
        else
        {
            fname = response->add_filename();
            *fname = FileInfo.name;
            while (_findnext(Handle, &FileInfo) == 0) {
                fname = response->add_filename();
                *fname = FileInfo.name;
            }
            _findclose(Handle);
        }
        done->Run();
    };
    /// 保存upload中request的文件数据
    virtual void upload(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Filesystem::File_Request* request,
        ::myRPC::Filesystem::bool_Response* response,
        ::google::protobuf::Closure* done) {
        
        std::ofstream outfile;
        std::string fname;
        fname = request->file_name();
        fname = "..\\" + fname;
        outfile.open(fname.c_str());
        outfile << request->file_data();
        outfile.close();
        response->set_flag(1);

        if (request->flag() == 1)
        {
            DirServer::ServerInfo_Request serverinfo_request;
            DirServer::ServerInfo_Response serverinfo_response;
            std::string DirServer_addr = "127.0.0.1:11111";
            RpcChannel dir_channel(DirServer_addr);
            DirServer::LoginServer_Stub dir_stub(&dir_channel);
            serverinfo_request.set_flag(1); // 1 查询在线服务武器
            RpcController dir_controller;
            dir_stub.ServerInfo(&dir_controller, &serverinfo_request, &serverinfo_response, nullptr);

            int num_services = serverinfo_response.server_list_size();
            DirServer::ServiceInfo tp;
            for (int i = 0; i < num_services; i++) {
                tp = serverinfo_response.server_list(i);
                if (tp.ip() != _ip || tp.port() != _port) {
                    Filesystem::File_Request _request;
                    Filesystem::bool_Response _response;

                    _request.set_flag(2);
                    _request.set_file_name(request->file_name());
                    _request.set_file_data(request->file_data());

                    std::string addr = tp.ip() + ":" + tp.port();
                    RpcChannel _channel(addr);
                    Filesystem::File_Server_Stub _stub(&_channel);
                    RpcController _controller;
                    _stub.upload(&_controller, &_request, &_response, nullptr);
                }
            }

        }

        done->Run();
    };
    virtual void download(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Filesystem::File_Request* request,
        ::myRPC::Filesystem::File_Response* response,
        ::google::protobuf::Closure* done) {
        
        std::string fname;
        fname = request->file_name();
        fname = "..\\" + fname;
        std::string fdata = readFileIntoString(fname.c_str());

        response->set_file_name(fname.c_str());
        response->set_file_data(fdata.c_str());

        done->Run();
    };
    virtual void ddelete(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Filesystem::File_Request* request,
        ::myRPC::Filesystem::bool_Response* response,
        ::google::protobuf::Closure* done) {
    
        std::string fname="..\\";
        fname += request->file_name();
        if (remove(fname.c_str()) == 0)
            response->set_flag(1);
        else
            response->set_flag(0);

        if(request->flag()==1)
        {
            DirServer::ServerInfo_Request serverinfo_request;
            DirServer::ServerInfo_Response serverinfo_response;
            std::string DirServer_addr = "127.0.0.1:11111";
            RpcChannel dir_channel(DirServer_addr);
            DirServer::LoginServer_Stub dir_stub(&dir_channel);
            serverinfo_request.set_flag(1); // 1 查询在线服务武器
            RpcController dir_controller;
            dir_stub.ServerInfo(&dir_controller, &serverinfo_request, &serverinfo_response, nullptr);

            int num_services = serverinfo_response.server_list_size();
            DirServer::ServiceInfo tp;
            for (int i = 0; i < num_services; i++) {
                tp = serverinfo_response.server_list(i);
                if (tp.ip() != _ip || tp.port() != _port) {
                    Filesystem::File_Request _request;
                    Filesystem::bool_Response _response;

                    _request.set_flag(2);
                    _request.set_file_name(request->file_name());
                    _request.set_file_data(request->file_data());

                    std::string addr = tp.ip() + ":" + tp.port();
                    RpcChannel _channel(addr);
                    Filesystem::File_Server_Stub _stub(&_channel);
                    RpcController _controller;
                    _stub.ddelete(&_controller, &_request, &_response, nullptr);
                }
            }

        }
        done->Run();
    };
    virtual void mkdir(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Filesystem::File_Request* request,
        ::myRPC::Filesystem::bool_Response* response,
        ::google::protobuf::Closure* done) {
        
        std::string dname = request->file_name();
        dname = "..\\" + dname;
        std::string folderPath = dname;
        std::string command;
        command = "mkdir -p " + folderPath;
        system(command.c_str());
        response->set_flag(1);

        done->Run();
    };
    virtual void create(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Filesystem::File_Request* request,
        ::myRPC::Filesystem::bool_Response* response,
        ::google::protobuf::Closure* done) {
    
        std::ofstream outfile;
        std::string fname;
        fname = request->file_name();
        fname = "..\\" + fname;
        outfile.open(fname.c_str());
        outfile << request->file_data();
        outfile.close();
        response->set_flag(1);

        if (request->flag() == 1)
        {
            //std::cout << "1" << std::endl;
            DirServer::ServerInfo_Request serverinfo_request;
            DirServer::ServerInfo_Response serverinfo_response;
            std::string DirServer_addr = "127.0.0.1:11111";
            RpcChannel dir_channel(DirServer_addr);
            DirServer::LoginServer_Stub dir_stub(&dir_channel);
            serverinfo_request.set_flag(1); // 1 查询在线服务武器
            RpcController dir_controller;
            dir_stub.ServerInfo(&dir_controller, &serverinfo_request, &serverinfo_response, nullptr);

            int num_services = serverinfo_response.server_list_size();
            DirServer::ServiceInfo tp;
            for (int i = 0; i < num_services; i++) {
               // std::cout << "2" << std::endl;
                tp = serverinfo_response.server_list(i);
                if (tp.ip() != _ip || tp.port() != _port) {
                    Filesystem::File_Request _request;
                    Filesystem::bool_Response _response;

                    _request.set_flag(2);
                    _request.set_file_name(request->file_name());
                    _request.set_file_data(request->file_data());

                    std::string addr = tp.ip() + ":" + tp.port();
                    RpcChannel _channel(addr);
                    Filesystem::File_Server_Stub _stub(&_channel);
                    RpcController _controller;
                    _stub.create(&_controller, &_request, &_response, nullptr);
                }
            }

        }
        done->Run();
    };
    virtual void open(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::myRPC::Filesystem::File_Request* request,
        ::myRPC::Filesystem::File_Response* response,
        ::google::protobuf::Closure* done) {
    
        std::string fname;
        fname = request->file_name();
        fname = "..\\" + fname;
        std::string fdata = readFileIntoString(fname.c_str());

        response->set_file_name(fname.c_str());
        response->set_file_data(fdata.c_str());

        done->Run();
    };
};

int main(int argc, char* argv[])
{
    RpcServer rpc_server;

    File_Server* pFileServer = new File_ServerImpl();
    if (!rpc_server.RegisterService(pFileServer, false)) {
        std::cout << "error: register service failed" << std::endl;
        return -1;
    }

    //
    myRPC::DirServer::Login_Request request;
    myRPC::DirServer::Login_Response response;

    request.set_ip(_ip);
    request.set_port(_port);

    std::string dir_server_addr("127.0.0.1:11111");
    RpcChannel rpc_channel(dir_server_addr);
    LoginServer_Stub stub(&rpc_channel);

    RpcController controller;
    stub.Login(&controller, &request, &response, nullptr);

    if (controller.Failed())
        std::cout << "request failed: %s" << controller.ErrorText().c_str();
    else
        std::cout << "resp: " << response.flag() << std::endl;
    //
    std::string server_addr("0.0.0.0:12322"); //监听0.0.0.0

    if (!rpc_server.Start(server_addr)) {
        std::cout << "error: start server failed" << std::endl;
        return -1;
    }
    myRPC::DirServer::Logout_Request request_out;
    myRPC::DirServer::Logout_Response response_out;
    RpcChannel rpc_channel2(dir_server_addr);
    LoginServer_Stub stub2(&rpc_channel2);
    request_out.set_ip(_ip);
    request_out.set_port(_port);
    stub2.Logout(&controller, &request_out, &response_out, nullptr);
    return 0;
}