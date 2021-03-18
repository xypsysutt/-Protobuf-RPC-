#pragma once
#include <iostream>
#include <fstream>
#include <conio.h>
#include <sstream>
#include "../myRPC/rpc_server.h"
#include "../myRPC/rpc_controller.h"
#include "../myRPC/rpc_channel.h"
#include "../Filesystem/File_message.pb.h"
//#include "../Directory_Server/Dir_message.pb.cc"
#include "../Directory_Server/Dir_message.pb.h"
#include "../Lock_Server/Lock_message.pb.h"

using namespace myRPC;

void help() {
    std::cout << "                                        -ls 展开文件树" << std::endl;
	std::cout << "                                        -open 打开文件" << std::endl;
	std::cout << "                                        -create 创建文本文件" << std::endl;
	std::cout << "                                        -delete 删除文本文件" << std::endl;
	std::cout << "                                        -upload 上传文件并复制" << std::endl;
	std::cout << "                                        -download 缓存文件到本地" << std::endl;
}
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
int lock(std::string fname) {
    Lock::LockInfo_Requst request;
    Lock::Lock_Response response;
    
    request.set_filename(fname);

    std::string addr = "127.0.0.1:11112";
    RpcChannel _channel(addr);
    Lock::Lock_Server_Stub _stub(&_channel);
    RpcController _controller;
    _stub.Lock(&_controller, &request, &response, nullptr);
    return response.flag();
}
void barriar(std::string fname) {
    while (lock(fname) == 2) {};
}
int unlock(std::string fname) {
    Lock::unLockInfo_Requst request;
    Lock::Lock_Response response;

    request.set_filename(fname);

    std::string addr = "127.0.0.1:11112";
    RpcChannel _channel(addr);
    Lock::Lock_Server_Stub _stub(&_channel);
    RpcController _controller;
    _stub.unLock(&_controller, &request, &response, nullptr);
    return response.flag();
}
int main(int argc, char* argv[])
{
    /*if (argc < 3) {
        print_usage();
        return -1;
    }*/
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
    std::cout << "可用文件服务器有: " << std::endl;
    for (int i = 0; i < num_services; i++) {
        tp = serverinfo_response.server_list(i);
        std::cout << "Server_Id: " << tp.server_id() << " IP: " << tp.ip() << " Port: " << tp.port() << std::endl;
    }

    std::cout << "input Server_Id of the server you want to connect:\n>>> ";
    int k = 0;
    std::cin >> k;
    tp = serverinfo_response.server_list(k);// ip:port


    std::string ip = tp.ip();
    std::string port = tp.port();
    std::string addr = ip + ":" + port;
    //RpcChannel    rpc_channel(addr);
    //Filesystem::File_Server_Stub stub(&rpc_channel);
    RpcController controller;
    std::string cmd;
    help();
    while (1) {

        std::cout << ">>> ";
        std::cin >> cmd;
        if (cmd == "ls") {
            RpcChannel    rpc_channel(addr);
            Filesystem::File_Server_Stub stub(&rpc_channel);
            Filesystem::File_Request   request;
            Filesystem::Dir_Response response;
            request.set_file_name("ls");
            stub.ls(&controller, &request, &response, nullptr);
            if (controller.Failed()) {
                std::cout << "request failed: %s" << controller.ErrorText().c_str();
                break;
            }
            int size = response.filename_size();
            for (int i = 0; i < size; i++) {
                std::cout << response.filename(i) << "   ";
            }
            std::cout << std::endl;
        }
        else if (cmd == "upload") {
            RpcChannel    rpc_channel(addr);
            Filesystem::File_Server_Stub stub(&rpc_channel);
            Filesystem::File_Request   request;
            Filesystem::bool_Response  response;
            std::cout << "Filename you want to upload: ";
            std::string fname;
            std::cin >> fname;
            barriar(fname);
            std::string fdata = readFileIntoString(fname.c_str());
            request.set_file_data(fdata.c_str());
            request.set_file_name(fname.c_str());
            request.set_flag(1);
            stub.upload(&controller, &request, &response, nullptr);
            if (controller.Failed()) {
                std::cout << "request failed: %s" << controller.ErrorText().c_str()<<std::endl;
                break;
            }// 重传机制 暂时未设置
            else {
                if (response.flag() == 1) {
                    std::cout << "upload successed!" << std::endl;
                }
            }
            unlock(fname);
        }
        else if (cmd == "download") {
            RpcChannel    rpc_channel(addr);
            Filesystem::File_Server_Stub stub(&rpc_channel);
            Filesystem::File_Request   request;
            Filesystem::File_Response  response;

            std::cout << "Filename you want to download: ";
            std::string fname;
            std::cin >> fname;
            barriar(fname);
            request.set_file_name(fname.c_str());
            stub.download(&controller, &request, &response, nullptr);
            if (controller.Failed()) {
                std::cout << "request failed: %s" << controller.ErrorText().c_str() << std::endl;
                break;
            }// 重传机制 暂时未设置
            if (response.file_data().size() == 0) {
                std::cout << "Failed. 请检查文件是否存在" << std::endl;
            }

            std::ofstream outfile;
            unlock(fname);
            fname = "..\\" + fname;
            outfile.open(fname.c_str());
            outfile << response.file_data();
            outfile.close();
            std::cout << "download Done!" << std::endl;
        }
        else if (cmd == "delete") {
            RpcChannel    rpc_channel(addr);
            Filesystem::File_Server_Stub stub(&rpc_channel);
            Filesystem::File_Request   request;
            Filesystem::bool_Response  response;
            std::cout << "Filename you want to delete: ";
            std::string fname;
            std::cin >> fname;
            barriar(fname);
            request.set_file_name(fname.c_str());
            request.set_flag(1);
            stub.ddelete(&controller, &request, &response, nullptr);
            if (controller.Failed()) {
                std::cout << "request failed: %s" << controller.ErrorText().c_str() << std::endl;
                break;
            }// 重传机制 暂时未设置
            else {
                if (response.flag() == 1) {
                    std::cout << "delete successed!" << std::endl;
                }
            }
            unlock(fname);
        }
        else if (cmd == "mkdir") {
            RpcChannel    rpc_channel(addr);
            Filesystem::File_Server_Stub stub(&rpc_channel);
            Filesystem::File_Request   request;
            Filesystem::bool_Response  response;
            std::cout << "FileDirectory name you want to make: ";
            std::string fname;
            std::cin >> fname;

            request.set_file_name(fname.c_str());
            stub.mkdir(&controller, &request, &response, nullptr);
            if (controller.Failed()) {
                std::cout << "request failed: %s" << controller.ErrorText().c_str() << std::endl;
                break;
            }// 重传机制 暂时未设置
            else {
                if (response.flag() == 1) {
                    std::cout << "mkdir successed!" << std::endl;
                }
            }
        }
        else if (cmd == "create") {
            RpcChannel    rpc_channel(addr);
            Filesystem::File_Server_Stub stub(&rpc_channel);
            Filesystem::File_Request   request;
            Filesystem::bool_Response  response;
            std::cout << "File name you want to create: ";
            std::string fname;
            std::cin >> fname;
            barriar(fname);
            request.set_file_name(fname.c_str());
            request.set_file_data("hello world!");
            request.set_flag(1);
            stub.create(&controller, &request, &response, nullptr);
            if (controller.Failed()) {
                std::cout << "request failed: %s" << controller.ErrorText().c_str() << std::endl;
                break;
            }// 重传机制 暂时未设置
            else {
                if (response.flag() == 1) {
                    std::cout << "create successed!" << std::endl;
                }
            }
            unlock(fname);
        }
        else if (cmd == "open") {
        
        std::cout << "File name you want to open: ";
        std::string fname;
        std::cin >> fname;

        std::ifstream File;
        std::string pwd = "..\\";
        pwd += fname;
        File.open(pwd.c_str(), std::ios::in);
        if (File) { readFileIntoString(pwd.c_str()); break; }
        else { std::cout << "本地不存在该文件，调用RPC"<<std::endl; };
        File.close();
        barriar(fname);

        RpcChannel    rpc_channel(addr);
        Filesystem::File_Server_Stub stub(&rpc_channel);
        Filesystem::File_Request   request;
        Filesystem::File_Response  response;
        request.set_file_name(fname.c_str());
        stub.open(&controller, &request, &response, nullptr);
        if (controller.Failed()) {
            std::cout << "request failed: %s" << controller.ErrorText().c_str() << std::endl;
            unlock(fname);
            break;
        }// 重传机制 暂时未设置
        

        std::cout << response.file_data() << std::endl;
        while (1) {
            if (_kbhit()) {
                char ch = _getch();
                if (ch == 27) {
                    break;
                }
            }
        }
        
        unlock(fname);
        }
        else {
        help();
}
       
    }

    return 0;
}

