// include/services/rpc_service.h
#ifndef RPC_SERVICE_H
#define RPC_SERVICE_H

#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>
#if CPP11_SUPPORTED
#include <unordered_map>
#endif

// 编译器特性检测
#if !defined(CPP11_SUPPORTED)
    #if defined(__cplusplus) && __cplusplus >= 201103L
        #define CPP11_SUPPORTED 1
    #else
        #define CPP11_SUPPORTED 0
    #endif
#endif

class RpcService {
public:
#if CPP11_SUPPORTED
    // C++11实现版本
    using MethodHandler = std::function<nlohmann::json(const nlohmann::json&)>;
    
    void registerMethod(const std::string& name, MethodHandler handler) {
        methodHandlers_[name] = handler;
    }
#else
    // C++98兼容版本
    typedef nlohmann::json (*MethodHandler)(void* context, const nlohmann::json&);
    
    struct HandlerInfo {
        MethodHandler handler;
        void* context;
    };

    void registerMethod(const std::string& name, 
                       MethodHandler handler, 
                       void* context) {
        HandlerInfo info = { handler, context };
        methodHandlers_[name] = info;
    }
#endif

    nlohmann::json executeMethod(const std::string& method, 
                                const nlohmann::json& params) {
#if CPP11_SUPPORTED
        auto it = methodHandlers_.find(method);
#else
        std::map<std::string, HandlerInfo>::iterator it = methodHandlers_.find(method);
#endif
        if (it == methodHandlers_.end()) {
            throw std::runtime_error("Method not found");
        }
        
#if CPP11_SUPPORTED
        return it->second(params);
#else
        return it->second.handler(it->second.context, params);
#endif
    }

private:
#if CPP11_SUPPORTED
    std::unordered_map<std::string, MethodHandler> methodHandlers_;
#else
    std::map<std::string, HandlerInfo> methodHandlers_;
#endif
};

#endif // RPC_SERVICE_H