#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <event2/http.h>
#include <event2/bufferevent_ssl.h>
#include <openssl/ssl.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Libevent 类型前向声明
struct event_base;
struct evhttp;

class RpcServer {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param certPath HTTPS证书路径
     * @param keyPath HTTPS私钥路径
     */
    RpcServer(int port, const char* certPath, const char* keyPath);

    /**
     * @brief 启动服务器事件循环
     */
    void start();

    // 禁止拷贝和赋值
    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;

private:
    SSL_CTX* sslCtx_;       // SSL上下文
    event_base* base_;       // 事件循环基础
    evhttp* http_;           // HTTP服务器实例

    /**
     * @brief 处理RPC服务器接收到的HTTP请求
     * 
     * @param req evhttp请求对象指针，用于获取请求信息和发送响应
     * @param arg 未使用的泛型参数（根据常见libevent用法保留）
     */
    static void requestHandler(evhttp_request* req, void* arg);

    /**
     * @brief SSL连接创建回调
     * @param base 事件循环基础
     * @param arg 用户自定义参数
     * @return 返回新的bufferevent对象
     */
    static bufferevent* bevCallback(event_base* base, void* arg);

    // 内部工具函数
    static std::vector<std::string> splitUri(const std::string& uri);

    static void sendSuccessResponse(evhttp_request* req, const nlohmann::json& result, const nlohmann::json& id);
    
    static void sendErrorResponse(evhttp_request* req, int code, const std::string& msg, const nlohmann::json& id);    
};

#endif // RPC_SERVER_H