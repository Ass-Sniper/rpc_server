// include/services/math_service.h
#ifndef MATH_SERVICE_H
#define MATH_SERVICE_H

#include "rpc_service.h"

// 数学服务类，继承自RpcService
class MathService : public RpcService {
public:
    MathService();

private:
    // 通用成员函数
    double add(double a, double b);
    double subtract(double a, double b);

#if !CPP11_SUPPORTED
    // 处理 add 方法的静态函数
    static nlohmann::json addHandler(void* context, const nlohmann::json& params);

    // 处理 subtract 方法的静态函数
    static nlohmann::json subtractHandler(void* context, const nlohmann::json& params);
#endif
};

#endif // MATH_SERVICE_H