// main.cpp
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/evp.h> // 添加 OpenSSL EVP 头文件包含
#include <openssl/x509.h> // 添加 OpenSSL X509 头文件包含
#include <openssl/pem.h> // 添加 OpenSSL PEM 头文件包含
#include <openssl/err.h> // 添加 OpenSSL 错误处理头文件包含
#include "framework/ioc_container.h"
#include "framework/rpc_server.h"
#include "services/math_service.h"

// 守护进程化函数
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "第一次fork失败" << std::endl;
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS); // 父进程退出
    }

    // 创建新会话
    if (setsid() < 0) {
        std::cerr << "无法创建新会话" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 第二次fork防止重新获取终端
    pid = fork();
    if (pid < 0) {
        std::cerr << "第二次fork失败" << std::endl;
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS); // 父进程退出
    }

    // 设置文件权限掩码
    umask(0);

    // 切换工作目录
    if (chdir("/") < 0) {
        std::cerr << "无法切换根目录" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 关闭标准文件描述符
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 重定向标准IO到/dev/null
    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
}

int main(int argc, char* argv[]) {
    int port = 8443;  // 默认端口
    bool daemon = false;
    char * serverCertPath = const_cast<char*>("cert/server.crt");
    char * serverKeyPath = const_cast<char*>("cert/server.key");

    // 解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "p:d:m:n")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port < 1 || port > 65535) {
                    std::cerr << "无效端口号: " << optarg << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            case 'd':
                daemon = true;
                break;
            case 'm':
                // 处理服务器证书路径
                serverCertPath = optarg ?  optarg : serverCertPath; 
                if(access(serverCertPath, F_OK) != 0) {
                    std::cerr << "服务器证书文件(" << serverCertPath << ")不存在" << std::endl;    
                }
                break;
            case 'n':
                // 处理服务器密钥路径
                serverKeyPath = optarg ?  optarg : serverKeyPath; 
                if(access(serverKeyPath, F_OK) != 0) {
                    std::cerr << "服务器密钥文件(" << serverKeyPath << ")不存在" << std::endl;    
                }                
                break;
            default:
                std::cerr << "用法: " << argv[0] 
                         << " [-p port] [-d] [-m servercert] [-n serverkey]" << std::endl;
                return EXIT_FAILURE;
        }
    }

    if (daemon) {
        daemonize();
    }

    // 验证服务器证书和私钥匹配性
    {
        FILE* certFile = fopen(serverCertPath, "r");
        FILE* keyFile = fopen(serverKeyPath, "r");

        if (!certFile || !keyFile) {
            std::cerr << "无法打开证书或密钥文件" << std::endl;
            return EXIT_FAILURE;
        }

        X509* cert = PEM_read_X509(certFile, NULL, NULL, NULL);
        EVP_PKEY* key = PEM_read_PrivateKey(keyFile, NULL, NULL, NULL);

        fclose(certFile);
        fclose(keyFile);

        if (!cert || !key) {
            std::cerr << "无法读取证书或密钥" << std::endl;
            if (cert) X509_free(cert);
            if (key) EVP_PKEY_free(key);
            return EXIT_FAILURE;
        }

        unsigned char certModulus[EVP_MAX_MD_SIZE];
        unsigned int certModulusLen;
        unsigned char keyModulus[EVP_MAX_MD_SIZE];
        unsigned int keyModulusLen;

        if (!X509_digest(cert, EVP_sha256(), certModulus, &certModulusLen)) {
            std::cerr << "无法计算证书模数" << std::endl;
            X509_free(cert);
            EVP_PKEY_free(key);
            return EXIT_FAILURE;
        }

        if (!EVP_PKEY_digest(key, EVP_sha256(), keyModulus, &keyModulusLen)) {
            std::cerr << "无法计算密钥模数" << std::endl;
            X509_free(cert);
            EVP_PKEY_free(key);
            return EXIT_FAILURE;
        }

        X509_free(cert);
        EVP_PKEY_free(key);

        if (certModulusLen != keyModulusLen || memcmp(certModulus, keyModulus, certModulusLen) != 0) {
            std::cerr << "\033[31m客户端证书和私钥不匹配\033[0m" << std::endl;
            return EXIT_FAILURE;
        } else {
            std::cout << "\033[32m客户端证书和私钥匹配\033[0m" << std::endl;
        }
    }

    try {
        // 初始化IoC容器
        auto& container = IocContainer::getInstance();
        container.registerService<MathService>("mathService");

        // 启动RPC服务器
        RpcServer server(port, serverCertPath, serverKeyPath);
        std::cout << "服务已启动，监听端口: " << port 
                 << (daemon ? " (守护进程模式)" : "") << std::endl;

        // 进入事件循环
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "服务器异常: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}