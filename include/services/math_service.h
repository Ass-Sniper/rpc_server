// include/services/math_service.h
#ifndef MATH_SERVICE_H
#define MATH_SERVICE_H

#include "services/rpc_service.h"
#include <nlohmann/json.hpp>

#if 0
// C++98兼容版本
class MathService : public RpcService {
public:
    MathService() {
        registerMethod("add", &MathService::addWrapper, this);
        registerMethod("divide", &MathService::divideWrapper, this);
    }

private:
    // 静态包装函数
    static nlohmann::json addWrapper(void* context, const nlohmann::json& params) {
        MathService* self = static_cast<MathService*>(context);
        return nlohmann::json::object({{"result", self->add(params["a"], params["b"])}});
    }

    static nlohmann::json divideWrapper(void* context, const nlohmann::json& params) {
        MathService* self = static_cast<MathService*>(context);
        return nlohmann::json::object({{"result", 
            self->divide(params["numerator"], params["denominator"])}});
    }

    // 保持原有成员函数不变
    double add(double a, double b) { /*...*/ }
    double divide(double n, double d) { /*...*/ }
};

// RpcService基类需修改为支持以下注册接口：
// void registerMethod(const std::string& name, 
//     nlohmann::json (*func)(void*, const nlohmann::json&), 
//     void* context);
#endif

class MathService : public RpcService {
    public:
        MathService() {
            // 注册加法方法
            registerMethod("add", [this](const nlohmann::json& params) {
                return nlohmann::json{
                    {"result", add(params["a"], params["b"])}
                };
            });
    
            // 注册除法方法
            registerMethod("divide", [this](const nlohmann::json& params) {
                return nlohmann::json{
                    {"result", divide(params["numerator"], params["denominator"])}
                };
            });
        }
    
    private:
        double add(double a, double b) {
            return a + b;
        }
    
        double divide(double numerator, double denominator) {
            if (denominator == 0) {
                throw std::runtime_error("Division by zero");
            }
            return numerator / denominator;
        }
    };

#endif