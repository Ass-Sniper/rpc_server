// include/framework/rpc_server.h
#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <string>
#include <map>
#include <vector> // 添加 vector 头文件包含
#include <event2/http.h>
#include <openssl/ssl.h>
#include <nlohmann/json.hpp> // 添加 json 头文件包含

class RpcServer {
public:
    RpcServer(int port, const char* certPath, const char* keyPath);
    void start();

private:
    static bufferevent* bevCallback(event_base* base, void* arg);
    void requestHandler(evhttp_request* req, void* arg);
    void logAudit(const std::map<std::string, std::string>& auditData); // 添加 logAudit 函数声明

    // 添加 splitUri 函数声明
    std::vector<std::string> splitUri(const std::string& uri);

    // 添加 sendSuccessResponse 函数声明
    void sendSuccessResponse(evhttp_request* req, const nlohmann::json& result, const nlohmann::json& id);

    // 添加 sendErrorResponse 函数声明
    void sendErrorResponse(evhttp_request* req, int code, const std::string& message, const nlohmann::json& id);

    SSL_CTX* sslCtx_;
    event_base* base_;
    evhttp* http_;
};

#endif // RPC_SERVER_H