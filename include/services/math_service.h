// include/services/math_service.h
#ifndef MATH_SERVICE_H
#define MATH_SERVICE_H

#include "services/rpc_service.h"
#include <nlohmann/json.hpp>

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