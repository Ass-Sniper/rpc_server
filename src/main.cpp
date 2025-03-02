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

struct Arguments {
    int port = 8443;  // 默认端口
    bool daemon = false;
    std::string serverCertPath = "cert/server.crt";
    std::string serverKeyPath = "cert/server.key";
    std::string logFilePath = "rpc_server.log"; // 默认日志文件路径
    bool verbose = false;
};


// 提取参数解析逻辑到单独的函数
void parseArguments(int argc, char* argv[], Arguments& args) {
    int opt;
    while ((opt = getopt(argc, argv, "p:dl:m:n:v")) != -1) {
        switch (opt) {
            case 'p':
                args.port = atoi(optarg);
                if (args.port < 1 || args.port > 65535) {
                    std::cerr << "无效端口号: " << optarg << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                args.daemon = true;
                break;
            case 'm':
                // 处理服务器证书路径
                args.serverCertPath = optarg ? optarg : args.serverCertPath;
                if (access(args.serverCertPath.c_str(), F_OK) != 0) {
                    std::cerr << "服务器证书文件(" << args.serverCertPath << ")不存在" << std::endl;
                }
                break;
            case 'n':
                // 处理服务器密钥路径
                args.serverKeyPath = optarg ? optarg : args.serverKeyPath;
                if (access(args.serverKeyPath.c_str(), F_OK) != 0) {
                    std::cerr << "服务器密钥文件(" << args.serverKeyPath << ")不存在" << std::endl;
                }
                break;
            case 'l':
                // 处理日志文件路径
                args.logFilePath = optarg ? optarg : args.logFilePath;
                if (access(args.logFilePath.c_str(), F_OK) != 0) {
                    std::cerr << "日志文件(" << args.logFilePath << ")不存在" << std::endl;
                }
                break;
            case 'v':
                // 启用详细日志输出
                args.verbose = true;
                break;
            default:
                std::cerr << "用法: " << argv[0] << std::endl;
                std::cerr << "  -p <port>        指定服务器监听端口 (默认: 8443)" << std::endl;
                std::cerr << "  -d               以守护进程模式运行" << std::endl;
                std::cerr << "  -m <servercert>  指定服务器证书文件路径" << std::endl;
                std::cerr << "  -n <serverkey>   指定服务器密钥文件路径" << std::endl;
                std::cerr << "  -l <logfile>     指定日志文件路径" << std::endl;
                std::cerr << "  -v               启用详细日志输出" << std::endl;
                exit(EXIT_FAILURE);
        }
    }
}

// 提取验证服务器证书和私钥匹配性的逻辑到单独的函数
bool verifyCertificateAndKeyMatch(const char* serverCertPath, const char* serverKeyPath) {
    FILE* certFile = fopen(serverCertPath, "r");
    FILE* keyFile = fopen(serverKeyPath, "r");

    if (!certFile || !keyFile) {
        std::cerr << "无法打开证书或密钥文件" << std::endl;
        if (certFile) fclose(certFile);
        if (keyFile) fclose(keyFile);
        return false;
    }

    X509* cert = PEM_read_X509(certFile, NULL, NULL, NULL);
    EVP_PKEY* key = PEM_read_PrivateKey(keyFile, NULL, NULL, NULL);

    fclose(certFile);
    fclose(keyFile);

    if (!cert || !key) {
        std::cerr << "无法读取证书或密钥" << std::endl;
        if (cert) X509_free(cert);
        if (key) EVP_PKEY_free(key);
        return false;
    }

    unsigned char certModulus[EVP_MAX_MD_SIZE];
    unsigned char keyModulus[EVP_MAX_MD_SIZE];
    unsigned int certModulusLen;
    unsigned int keyModulusLen;

    if (!X509_digest(cert, EVP_sha256(), certModulus, &certModulusLen)) {
        std::cerr << "无法计算证书模数" << std::endl;
        X509_free(cert);
        EVP_PKEY_free(key);
        return false;
    }

    RSA* rsa = EVP_PKEY_get1_RSA(key);
    if (!rsa) {
        std::cerr << "无法获取RSA密钥" << std::endl;
        X509_free(cert);
        EVP_PKEY_free(key);
        return false;
    }

    const BIGNUM* n = RSA_get0_n(rsa);
    keyModulusLen = BN_bn2bin(n, keyModulus);
    RSA_free(rsa);

    if (keyModulusLen <= 0) {
        std::cerr << "无法计算密钥模数" << std::endl;
        X509_free(cert);
        EVP_PKEY_free(key);
        return false;
    }

    X509_free(cert);
    EVP_PKEY_free(key);

    if (certModulusLen != keyModulusLen || memcmp(certModulus, keyModulus, certModulusLen) != 0) {
        std::cerr << "\033[31m客户端证书和私钥不匹配\033[0m" << std::endl;
        return false;
    } else {
        std::cout << "\033[32m客户端证书和私钥匹配\033[0m" << std::endl;
        return true;
    }
}

int main(int argc, char* argv[]) {
    Arguments args;

    // 解析命令行参数
    parseArguments(argc, argv, args);

    // 根据参数决定是否以守护进程模式运行
    if (args.daemon) {
        daemonize();
    }

    // 验证服务器证书和私钥匹配性
    if (!verifyCertificateAndKeyMatch(args.serverCertPath.c_str(), args.serverKeyPath.c_str())) {
        return EXIT_FAILURE;
    }

    try {
        // 初始化IoC容器
        auto& container = IocContainer::getInstance();
        container.registerService<MathService>("MathService");

        // 启动RPC服务器
        RpcServer server(args.port, args.serverCertPath.c_str(), args.serverKeyPath.c_str());
        std::cout << "服务已启动，监听端口: " << args.port
                  << (args.daemon ? " (守护进程模式)" : "") << std::endl;

        // 进入事件循环
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "服务器异常: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
