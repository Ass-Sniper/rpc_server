// src/services/math_service.cpp
#include "services/math_service.h"

MathService::MathService() {
#if CPP11_SUPPORTED
    // 使用 lambda 表达式
    registerMethod("add", [this](const nlohmann::json& params) {
        return nlohmann::json{{"result", add(params["a"], params["b"])}};
    });

    registerMethod("subtract", [this](const nlohmann::json& params) {
        return nlohmann::json{{"result", subtract(params["a"], params["b"])}};
    });
#else
    // 使用静态成员函数
    registerMethod("add", &MathService::addHandler, this);
    registerMethod("subtract", &MathService::subtractHandler, this);
#endif
}

// 通用成员函数
double MathService::add(double a, double b) {
    return a + b;
}

double MathService::subtract(double a, double b) {
    return a - b;
}

#if !CPP11_SUPPORTED
// 处理 add 方法的静态函数
nlohmann::json MathService::addHandler(void* context, const nlohmann::json& params) {
    MathService* self = static_cast<MathService*>(context);
    return nlohmann::json{{"result", self->add(params["a"], params["b"])}};
}

// 处理 subtract 方法的静态函数
nlohmann::json MathService::subtractHandler(void* context, const nlohmann::json& params) {
    MathService* self = static_cast<MathService*>(context);
    return nlohmann::json{{"result", self->subtract(params["a"], params["b"])}};
}
#endif