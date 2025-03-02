// src/framework/rpc_server.cpp
#include "framework/rpc_server.h" // 添加 rpc_server.h 头文件包含
#include "framework/ioc_container.h" // 修改包含路径
#include "services/rpc_service.h"
#include "mem_mgmt/safe_ptr.h"
#include "mem_mgmt/weak_ptr.h"
#include "mem_mgmt/lock_guard.h"
#include <event2/listener.h>
#include <event2/buffer.h>
#include <openssl/ssl.h> // 添加 OpenSSL 头文件包含
#include <openssl/err.h> // 添加 OpenSSL 错误处理头文件包含
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <arpa/inet.h>

using namespace std;

// SSL全局上下文初始化
static void initOpenSSL() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

// URI路径分割工具
vector<string> RpcServer::splitUri(const string& uri) {
    vector<string> tokens;
    istringstream iss(uri);
    string token;
    
    while (getline(iss, token, '/')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// 发送成功响应
void RpcServer::sendSuccessResponse(evhttp_request* req,
    const nlohmann::json& result,
    const nlohmann::json& id) 
{
    nlohmann::json response = {
        {"jsonrpc", "2.0"},
        {"result", result},
        {"id", id}
    };

    evbuffer* output = evhttp_request_get_output_buffer(req);
    evhttp_add_header(evhttp_request_get_output_headers(req), 
        "Content-Type", "application/json");
    evhttp_add_header(evhttp_request_get_output_headers(req), 
        "Strict-Transport-Security", 
        "max-age=63072000; includeSubDomains");        
    evbuffer_add_printf(output, "%s", response.dump().c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, output);
}

// 发送错误响应
void RpcServer::sendErrorResponse(evhttp_request* req,
  int code,
  const std::string& message,
  const nlohmann::json& id) 
{
    nlohmann::json error = {
        {"code", code},
        {"message", message}
    };

    nlohmann::json response = {
        {"jsonrpc", "2.0"},
        {"error", error},
        {"id", id}
    };

    evbuffer* output = evhttp_request_get_output_buffer(req);
    evhttp_add_header(evhttp_request_get_output_headers(req), 
        "Content-Type", "application/json");
    evhttp_add_header(evhttp_request_get_output_headers(req), 
        "Strict-Transport-Security", 
        "max-age=63072000; includeSubDomains");
    evbuffer_add_printf(output, "%s", response.dump().c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, output);
}

// 构造函数
RpcServer::RpcServer(int port, const char* certPath, const char* keyPath) 
    : sslCtx_(nullptr), base_(nullptr), http_(nullptr) {
    
    initOpenSSL();
    
    // 创建SSL上下文
    sslCtx_ = SSL_CTX_new(TLS_server_method());
    if (!sslCtx_) {
        cerr << "Error creating SSL context: " 
             << ERR_error_string(ERR_get_error(), nullptr) << endl;
        throw runtime_error("SSL context creation failed");
    }

    // 增强安全配置
    SSL_CTX_set_options(sslCtx_, 
        SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |    // 禁用不安全协议
        SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 |  // 仅允许TLSv1.2+
        SSL_OP_SINGLE_DH_USE |                 // 提升前向安全性
        SSL_OP_CIPHER_SERVER_PREFERENCE);      // 服务端优选加密套件

    // 配置现代加密套件
    const char* ciphers = "ECDHE-ECDSA-AES128-GCM-SHA256:"
                          "ECDHE-RSA-AES128-GCM-SHA256:"
                          "ECDHE-ECDSA-AES256-GCM-SHA384:"
                          "ECDHE-RSA-AES256-GCM-SHA384";
    SSL_CTX_set_cipher_list(sslCtx_, ciphers);

    // 启用会话票据
    SSL_CTX_set_num_tickets(sslCtx_, 5); // 合理数量平衡安全与性能    

    // 加载证书链
    if (SSL_CTX_use_certificate_chain_file(sslCtx_, certPath) <= 0) {
        cerr << "Error loading certificate: " 
             << ERR_error_string(ERR_get_error(), nullptr) << endl;
        SSL_CTX_free(sslCtx_);
        throw runtime_error("Certificate loading failed");
    }

    // 加载私钥
    if (SSL_CTX_use_PrivateKey_file(sslCtx_, keyPath, SSL_FILETYPE_PEM) <= 0) {
        cerr << "Error loading private key: " 
             << ERR_error_string(ERR_get_error(), nullptr) << endl;
        SSL_CTX_free(sslCtx_);
        throw runtime_error("Private key loading failed");
    }

    // 验证私钥匹配
    if (!SSL_CTX_check_private_key(sslCtx_)) {
        cerr << "Private key does not match certificate certPath: " << certPath 
            << " keyPath: " << keyPath << endl;
        SSL_CTX_free(sslCtx_);
        throw runtime_error("Key validation failed");
    }

    // 初始化事件循环
    base_ = event_base_new();
    if (!base_) {
        SSL_CTX_free(sslCtx_);
        throw runtime_error("Could not initialize event base");
    }

    // 创建HTTP服务器
    http_ = evhttp_new(base_);
    if (!http_) {
        event_base_free(base_);
        SSL_CTX_free(sslCtx_);
        throw runtime_error("Could not create HTTP server");
    }

    // 设置SSL回调
    evhttp_set_bevcb(http_, RpcServer::bevCallback, this);

    // 绑定端口
    if (evhttp_bind_socket(http_, "0.0.0.0", port) != 0) {
        evhttp_free(http_);
        event_base_free(base_);
        SSL_CTX_free(sslCtx_);
        throw runtime_error("Could not bind to port");
    }

    cout << "Server started on port " << port << endl;
}

// SSL连接回调
bufferevent* RpcServer::bevCallback(event_base* base, void* arg) {
    auto* server = static_cast<RpcServer*>(arg);
    SSL* ssl = SSL_new(server->sslCtx_);
    return bufferevent_openssl_socket_new(base, -1, ssl,
                                        BUFFEREVENT_SSL_ACCEPTING,
                                        BEV_OPT_CLOSE_ON_FREE);
}

/**
 * @brief 处理RPC服务器接收到的HTTP请求
 * 
 * @param req evhttp请求对象指针，用于获取请求信息和发送响应
 * @param arg 未使用的泛型参数（根据常见libevent用法保留）
 */
void RpcServer::requestHandler(evhttp_request* req, void* arg) {
    nlohmann::json requestJson;
    nlohmann::json id = nullptr;

    // 新增SSL连接检测
    // 获取底层连接对象
    evhttp_connection* conn = evhttp_request_get_connection(req);
    
    // 通过连接获取 bufferevent
    bufferevent* bev = evhttp_connection_get_bufferevent(conn);

    // 获取 SSL 对象
    SSL* ssl = nullptr;
    if (bev) {
        ssl = bufferevent_openssl_get_ssl(bev);
    }

    if (!ssl) {
        sendErrorResponse(req, -32000, "Not a secure connection", nullptr);
        return;
    }

    // 验证 SSL 连接状态
    if (ssl && SSL_get_verify_result(ssl) != X509_V_OK) {
        sendErrorResponse(req, -32000, "客户端证书验证失败", nullptr);
        return;
    }

   
    // 获取客户端地址信息
    std::string clientIP;
    ev_uint16_t clientPort = 0;
   
    if (conn) { 
        char *peer_addr = nullptr;       
        evhttp_connection_get_peer(conn, &peer_addr, &clientPort);
        clientIP = peer_addr ? peer_addr : "unknown";
    }  

    std::cout << "Connection: " << clientIP << ":" << clientPort << " using protocol: " << SSL_get_version(ssl) 
        << ", cipher: " << SSL_get_cipher(ssl) << endl;   

    try {
        // ========== 请求数据读取阶段 ==========
        // 从evhttp请求中获取输入缓冲区并读取原始数据
        evbuffer* input = evhttp_request_get_input_buffer(req);
        const size_t len = evbuffer_get_length(input);
        std::string requestData(len, 0);
        evbuffer_remove(input, &requestData[0], len);

        // ========== JSON解析与验证阶段 ==========
        // 解析JSON请求并验证基础结构
        requestJson = nlohmann::json::parse(requestData);

        // 校验JSON-RPC协议版本
        if (!requestJson.contains("jsonrpc") || requestJson["jsonrpc"] != "2.0") {
            sendErrorResponse(req, -32600, "Invalid JSON-RPC version", nullptr);
            return;
        }

        // 验证必须包含method字段
        if (!requestJson.contains("method")) {
            sendErrorResponse(req, -32600, "Missing method", nullptr);
            return;
        }

        // ========== 参数提取阶段 ==========
        // 解构请求参数并校验格式
        const std::string method = requestJson["method"];
        const nlohmann::json params = requestJson.value("params", nlohmann::json());
        id = requestJson.value("id", nullptr);

        // ========== 方法名解析阶段 ==========
        // 分割service.method格式的方法名
        const size_t dotPos = method.find('.');
        if (dotPos == std::string::npos || dotPos == 0 || dotPos == method.length()-1) {
            sendErrorResponse(req, -32601, "Invalid method format", id);
            return;
        }

        // 分割并规范化服务名称
        std::string serviceName = method.substr(0, dotPos);
        std::string methodName = method.substr(dotPos+1);
        if (!serviceName.empty()) {
            serviceName[0] = toupper(serviceName[0]); // 统一服务名首字母大写规范
        }

        // ========== 服务定位阶段 ==========
        // 从IoC容器获取服务实例
        auto service = IocContainer::getInstance().getService(serviceName);
        if (!service) {
            sendErrorResponse(req, -32601, "Service not found: " + serviceName, id);
            return;
        }

        // ========== 方法执行阶段 ==========
        // 反射调用服务方法并处理结果
        try {
            const nlohmann::json result = service->executeMethod(methodName, params);
            sendSuccessResponse(req, result, id);
        } catch (const std::exception& e) {
            sendErrorResponse(req, -32602, e.what(), id);
        }

        #if 1
        // ========== 审计日志记录阶段 ==========
        try {
            nlohmann::json auditLog = {
                {"client", clientIP},
                {"port", clientPort},
                {"method", method},
                {"timestamp", time(nullptr)}
            };
            
            // 方式1：紧凑JSON
            // std::string logEntry = auditLog.dump();
            
            // 方式2：格式化JSON（推荐用于日志）
            std::string logEntry = auditLog.dump(4);
            
            // 写入日志文件或发送到日志系统
            //logService.write(logEntry);
            std::cout << "Audit Log: " << logEntry << std::endl;
            
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "JSON serialization error: " << e.what() << endl;
        }   
        #endif     

    } // ========== 异常处理阶段 ==========
    catch (const nlohmann::json::parse_error& e) {
        sendErrorResponse(req, -32700, "Parse error: " + std::string(e.what()), id);
    } catch (const nlohmann::json::exception& e) {
        sendErrorResponse(req, -32600, "Invalid request: " + std::string(e.what()), id);
    } catch (const std::exception& e) {
        sendErrorResponse(req, -32603, "Internal error: " + std::string(e.what()), id);
    } catch (...) {
        sendErrorResponse(req, -32603, "Unknown internal error", id);
    }
}

// 启动服务
void RpcServer::start() {
    // 注册通用请求处理器
    evhttp_set_gencb(http_, requestHandler, this);
    
    // 进入事件循环
    event_base_dispatch(base_);

    // 清理资源
    evhttp_free(http_);
    event_base_free(base_);
    SSL_CTX_free(sslCtx_);
}