// include/services/rpc_service.h
#ifndef RPC_SERVICE_H
#define RPC_SERVICE_H

#include <string>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

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