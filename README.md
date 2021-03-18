<center><font face="黑体" size=10>Distributed-File-System </font></center>

<table width="300">
<tr>
    <td width="80"><B>题目</B></td>
    <td width="220"><center>Distributed-File-System</center></td>
</tr>
<tr>
    <td width="80"><B>姓名</B></td>
    <td width="220"><center>夏一溥</center></td>
</tr>
 <tr>
    <td width="80"><B>学号</B></td>
    <td width="220"><center>18340178</center></td>
</tr>
    <tr>
    <td width="80"><B>班级</B></td>
    <td width="220"><center>计科七班</center></td>
</tr>

<center><font face="黑体">目录 </font></center>
[TOC]

## 1. 题目要求

- [x] 基本的文件系统功能，如打开，删除，上传，下载等
- [x] 不同客户机拥有多副本，且保证一致性
- [x] 实现同步读写，也就是文件锁
- [x] 服务器与客户机间使用RPC进行通信

## 2. 实验环境

- 开发语言：c/c++, shell
- 编译环境：visual studio 2019
- 编译所需库：libprotobufd.lib(protobuf编译得到)，libboost_date_time.lib(boost MTd编译得到)，myRPC.lib(自己实现的RPC库)
- 操作系统：windows 10

## 3. 架构设计

### 3.1 基本说明

本项目使用传统的Server-Client架构，支持了一些基本的文件系统功能，如打开，删除，上传，下载等。能维护不同客户机的不同副本，并可以多地部署提高可用性。实现了锁服务器，当进行敏感操作，如删除，打开文件等操作时，所服务器会记录操作的文件名，阻止其他的客户机进行访问，而对服务器间的行为保持信任，从而可以在多个客户端上进行同步读写。各进程间通过RPC进行通信，不同文件的不同副本拥有一致性机制为瞬时一致性，即当某一服务器上的文件发生改变时，会异步复制保证所有副本的一致，而本地文件则作为缓存被客户机所拥有，不参与上述过程，本地文件会先优先参与各种操作，且客户机对本地文件进行操作不会导致锁服务器的更新与记录。同时服务器有日志功能，可以方便后台找到崩溃发生时执行的命令，但由于技术有限，没能实现原子事务回滚与回退恢复。 

该次实验所用RPC为基于protobuf实现的RPC，相对比较简陋，只实现了相对基础的结构框架，客户机交互通过提供的_channel通道与Server_Stub接口，服务器监听使用循环且同时监听键盘，当键入'ESC'时退出。RPC序列化的工具选择使用的是Protobuf，编译需要有Protobuf环境的支持。

### 3.2 文件系统架构

![image-20210126115917852](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126115917852.png)

![image-20210126151416008](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126151416008.png)

## 4. 具体实现

### 4.1 RPC实现

在Protobuf文档中给出了3个虚函数继承类：Service，channel，controller，其中controller用于记录RPC执行数据，Service用于服务器端，channel用于客户端。

#### 4.1.1 Service

查看编译proto文件生成的代码：

```c++
void File_Server::create(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                         const ::myRPC::Filesystem::File_Request*,
                         ::myRPC::Filesystem::bool_Response*,
                         ::google::protobuf::Closure* done) {
  controller->SetFailed("Method create() not implemented.");
  done->Run();
}
```

得知这里我们需要通过一个子类来继承Service，实现过程调用的具体功能，其中done为回调函数，在执行完成过程调用后调用执行，一般类似于析构。这里功能实现很具体，每个过程调用不同则实现不同，所以我在RPC的lib中没有实现Service的继承，而是在各个具体实现中是完成继承类的声明。

而更底层的，Service调用函数为：![image-20210126125406778](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126125406778.png)

其中 method 通过 service中API：GetDescriptor()来获取，通过service的虚类继承来实现调用对应的过程。而当一个服务器有多个过程调用时，又如何区分各个调用？ 这一问题需要我们自己来实现。

我的想法是先将所有的service与method保存在类中所维护的一个数组里:

```c++
// myRPC/rpc_server_impl.h
namespace myRPC {
        ...//不相关代码先隐去
        private:
            std::string   server_addr_;
            struct ServiceInfo {
                ::google::protobuf::Service* service_;
                std::map <std::string, const ::google::protobuf::MethodDescriptor*> mdescriptor_;
            };
            std::map<std::string, ServiceInfo> services_;
        };

}
```

在需要具体执行的时候，通过API获取传入的指针的信息:( rpc_meta_data_proto 为RPC间类似数据头的包)

```c++
(rpc_meta_data_proto.service_id(),rpc_meta_data_proto.method_id())
```

获取具体调用的过程信息。

#### 4.1.2 Client

还是先看生成部分：以ls为例（由于空格的关系，直接贴代码不方便看，这里贴图）

![image-20210126132706740](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126132706740.png)

发现其实是通过channel的CallMethod()，再看：

![image-20210126132845151](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126132845151.png)

这个与Service::CallMethod()有所不同，需要我们自己实现。而与Server端进行交互，需要指定的为server_id，method_id与data_size，封装为rpc_meta。于是有了如下实现：

```c++
void RpcChannelImpl::CallMethod(const ::google::protobuf::MethodDescriptor* method,
            ::google::protobuf::RpcController* controller,
            const ::google::protobuf::Message* request,
            ::google::protobuf::Message* response,
            ::google::protobuf::Closure* done)
        {
            std::string request_data_str;
            request->SerializeToString(&request_data_str);

            // 发送格式: meta_size + meta_data + request_data
            RpcMeta rpc_meta;
            rpc_meta.set_service_id(method->service()->name());
            rpc_meta.set_method_id(method->name());
            rpc_meta.set_data_size(request_data_str.size());
            std::string rpc_meta_str;
            rpc_meta.SerializeToString(&rpc_meta_str);

            int rpc_meta_str_size = rpc_meta_str.size();
            std::string serialzied_str;
            serialzied_str.insert(0, std::string((const char*)&rpc_meta_str_size, sizeof(int)));
            serialzied_str += rpc_meta_str;
            serialzied_str += request_data_str;
            socket_->send(boost::asio::buffer(serialzied_str));


            // 接受格式: response_size + response_data
            char resp_data_size[sizeof(int)];
            socket_->receive(boost::asio::buffer(resp_data_size));
            int resp_data_len = *(int*)resp_data_size;
            std::vector<char> resp_data(resp_data_len, 0);
            socket_->receive(boost::asio::buffer(resp_data));
            response->ParseFromString(std::string(&resp_data[0], resp_data.size()));
        }

```

### 4.2 File_Server

文件服务器要求能分别部署在多台电脑上，为了在本地模拟，这里选择将目录起始地址设置在启动处的上级目录里。可以通过建立多个文件夹实现模拟。

文件服务器的ip与port对客户端透明，客户端通过目录服务器选择连接的文件服务器。并根据protobuf的生成文件，在继承类中实现对应的功能如loadup，open等。

#### 4.2.1 开启文件服务器的流程

- 与目录服务器建立连接

```c++
myRPC::DirServer::Login_Request request;
    myRPC::DirServer::Login_Response response;

    request.set_ip(_ip);
    request.set_port(_port);

    std::string dir_server_addr("127.0.0.1:11111");
    RpcChannel rpc_channel(dir_server_addr);
    LoginServer_Stub stub(&rpc_channel);

    RpcController controller;
    stub.Login(&controller, &request, &response, nullptr);
```

- 文件服务启动

```c++
if (!rpc_server.Start(server_addr)) {
        std::cout << "error: start server failed" << std::endl;
        return -1;
    }
```

- 注销在目录服务器中的连接

```c++
myRPC::DirServer::Logout_Request request_out;
    myRPC::DirServer::Logout_Response response_out;
    RpcChannel rpc_channel2(dir_server_addr);
    LoginServer_Stub stub2(&rpc_channel2);
    request_out.set_ip(_ip);
    request_out.set_port(_port);
    stub2.Logout(&controller, &request_out, &response_out, nullptr);
```

#### 4.2.2 异步复制

以create调用举例，想法是当客户端发来调用后，在本地完成后将更新后副本广播给除自己以外的在线文件服务器，获取服务器列表通过与目录服务器交互完成：

```c++
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
```

值得注意的是，异步转发会设置flag=2，此时其他服务器进行该过程则不会重复转发。

其余功能不一一在报告中展示，详细实验参看File_Server.cc

### 4.3 Dir_Server

Dir_Server主要功能有文件服务器的登录登出，还有查询在线列表：

```c++
service LoginServer {
  rpc Login(Login_Request) returns(Login_Response);
  rpc Logout(Logout_Request) returns(Logout_Response);
  rpc ServerInfo(ServerInfo_Request) returns(ServerInfo_Response);
```

值得注意的是，Dir_Server维护了一个services数组，用于存储已经登录的文件服务器的IP与Port，具体实现见Dir_Server.cc

### 4.4 Lock_Server

Lock_Server只与Client通信，负责对文件加锁与解锁：

```
service Lock_Server {
  rpc Lock(LockInfo_Requst) returns(Lock_Response);
  rpc unLock(unLockInfo_Requst) returns(Lock_Response);
}
```

维护一个数组fname用来存储加锁文件信息。上锁只会发生在敏感操作且操作对象不是客户端本地资源时，所服务器只被动的接受上锁和解锁消息，其本身并不对文件进行操作，Client访问上锁文件时会持续忙等待知道查询锁服务器中锁信息被注销,具体实现见Lock_Server.cc

### 4.5 Client

client上线会与内置了IP：Port的Dir_Server交互获得课选择的文件服务器列表，选择后与File_Server建立了连接，就可以进行交互。

#### 4.5.1 优先检索本地

```c++
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
```

即每次open会优先检查本地文件是否存在，若存在则优先打开本地副本。

同时，open操作只是模拟了真实 的文件的打开状态，并没有实现内置的文本编辑器，退出需要键入"ESC"。

#### 4.5.2 锁

敏感操作时会先查询锁服务器，若有锁则忙等待：

```c++
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
```



## 5. 功能测试

测试文件结构：![image-20210126141841149](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126141841149.png)

每个文件夹下![image-20210126141914550](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126141914550.png)

### 5.1 文件服务器登录

![image-20210126142045772](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142045772.png)

#### 5.2 客户端上线

![image-20210126142318461](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142318461.png)

#### 5.3 连接文件服务器 与部分功能展示

- 连接

![image-20210126142407585](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142407585.png)

- ls

![image-20210126142429584](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142429584.png)

![image-20210126142500748](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142500748.png)

![image-20210126142519513](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142519513.png)

- create

![image-20210126142713812](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142713812.png)

- open

![image-20210126142936810](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126142936810.png)

![image-20210126143107526](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126143107526.png)

- download

![image-20210126150841278](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126150841278.png)

![image-20210126150906789](C:\Users\29334\AppData\Roaming\Typora\typora-user-images\image-20210126150906789.png)

## 6. 遇到的问题与总结

实现中最头疼的是建立起整个文件系统整体架构的阶段与实现RPC底层与Protobuf接口配对的看源码阶段。最开始心气很大想将共识算法加入其中，但做的过程中遇到了障碍，即Paxos共识中第二阶段的Propose_id的选举出了问题，因为文件更新不能违背因果一致性，而Propose_id其实起到的作用一部分是逻辑时钟，所以Propose_id只能增大，所以在实现Paxos是每个远程过程调用只能循环等待，即若第一阶段Prepare中Accept节点返回的Last_rnd>Propose_id则重新发起提议。这样对应每个文件 都有不同的Propose_id记录的逻辑时钟，为了避免这一问题需要在本地计时，由于时间优先Debug能力有限放弃了加入Paxos共识机制。

因为自己在Ubuntu上一直配不好环境，这次项目总的来说增强了自己在Windows上编程与使用VS的能力，带我温习了文件一致性的关系，文件锁的应用和分布式框架中透明性，让我有了对它们更直观的认识，同时还更深入的了解了Protobuf的源码，从源码出发还原了RPC的过程，收获还是很多。但同时，由于时间限制，实现系统的鲁棒性比较糟糕，如服务器打开过程中如果让File_Server等待太长时间可能会发生错误，没有 进行失效测试，等等。

## References

1. [基于protobuf的RPC实现](https://blog.csdn.net/kevinlynx/article/details/39379957?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-2.control)
2. [CS4032-Distributed-File-System](https://github.com/PinPinIre/CS4032-Distributed-File-System)



## README

测试样例的打开顺序为

Dir_Server - > File_Server/Lock_Server -> Client

否则有可能会因为超时报错。