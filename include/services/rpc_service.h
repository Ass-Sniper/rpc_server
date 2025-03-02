// include/services/rpc_service.h
#ifndef RPC_SERVICE_H
#define RPC_SERVICE_H

#include <string>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

#if 0

// include/services/rpc_service.h
#ifndef RPC_SERVICE_H
#define RPC_SERVICE_H

#include <string>
#include <map>
#include <stdexcept>

// C++98兼容的JSON库前置声明
class json;  // 需确保nlohmann/json有C++98兼容版本

class RpcService {
public:
    // 定义函数指针类型（带上下文）
    typedef json (*MethodHandler)(void* context, const json& params);
    
protected:
    // 注册方法接口（带上下文指针）
    void registerMethod(const std::string& name, 
                       MethodHandler handler, 
                       void* context) {
        HandlerInfo info = { handler, context };
        methodHandlers_[name] = info;
    }

public:
    json executeMethod(const std::string& method, 
                      const json& params) {
        std::map<std::string, HandlerInfo>::iterator it = methodHandlers_.find(method);
        if (it == methodHandlers_.end()) {
            throw std::runtime_error("Method not found");
        }
        return it->second.handler(it->second.context, params);
    }

private:
    // 处理器信息结构体
    struct HandlerInfo {
        MethodHandler handler;
        void* context;
    };
    
    std::map<std::string, HandlerInfo> methodHandlers_;
};

#endif  // RPC_SERVICE_H

#endif

class RpcService {
public:
    using MethodHandler = std::function<nlohmann::json(const nlohmann::json&)>;
    
protected:
    void registerMethod(const std::string& name, MethodHandler handler) {
        methodHandlers_[name] = handler;
    }

public:
    nlohmann::json executeMethod(const std::string& method, 
                                const nlohmann::json& params) {
        auto it = methodHandlers_.find(method);
        if (it == methodHandlers_.end()) {
            throw std::runtime_error("Method not found");
        }
        return it->second(params);
    }

private:
    std::unordered_map<std::string, MethodHandler> methodHandlers_;
};

#endif