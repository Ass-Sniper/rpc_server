// include/framework/ioc_container.h
#ifndef IOC_CONTAINER_H
#define IOC_CONTAINER_H

#include <memory>
#include <string>
#include <pthread.h> // 引入pthread库以支持线程安全
#include "mem_mgmt/safe_ptr.h"
#include "services/rpc_service.h" // 包含RpcService的头文件

// 编译器特性检测
#if !defined(CPP11_SUPPORTED)
    #if defined(__cplusplus) && __cplusplus >= 201103L
        #define CPP11_SUPPORTED 1
    #else
        #define CPP11_SUPPORTED 0
    #endif
#endif

// 容器类型选择
#if CPP11_SUPPORTED
    #include <unordered_map>
    #define SERVICE_MAP std::unordered_map
#else
    #include <map>
    #define SERVICE_MAP std::map
#endif

// 前向声明
class RpcService;

// 服务工厂基类
class ServiceFactoryBase {
public:
    virtual ~ServiceFactoryBase() {}
#if CPP11_SUPPORTED
    virtual std::unique_ptr<RpcService> create() = 0;
#else
    virtual SafePtr<RpcService> create() = 0; // 使用自定义安全指针
#endif
};

// 依赖注入容器
class IocContainer {
public:
    // 注册服务
    template<typename T>
    void registerService(const std::string& serviceId) {
#if CPP11_SUPPORTED
        // C++11及以上版本使用unique_ptr
        factories_[serviceId] = std::unique_ptr<ServiceFactoryBase>(new ServiceFactory<T>());
#else
        // C++98模式下添加异常安全保护
        SafePtr<ServiceFactoryBase> temp(new ServiceFactory<T>());
        factories_[serviceId] = temp; // 强异常安全保证
#endif
    }

    // 获取服务实例
#if CPP11_SUPPORTED
    std::unique_ptr<RpcService> getService(const std::string& serviceId) {
#else
    SafePtr<RpcService> getService(const std::string& serviceId) {
#endif
        auto it = factories_.find(serviceId);
        if (it == factories_.end()) {
            throw std::runtime_error("Service not registered");
        }
#if CPP11_SUPPORTED
        return it->second->create();
#else
        // 添加中间变量转为左值（关键修改）
        SafePtr<RpcService> service = it->second->create();
        return service;
#endif
    }

    // 单例模式线程安全：
    // C++11及以上：依赖Magic Static特性保证线程安全
    // C++98及以下：需额外添加双检锁(DCLP)实现
#if CPP11_SUPPORTED
    static IocContainer& getInstance() {
        static IocContainer instance; // C++11保证线程安全初始化
        return instance;
    }
#else
    // C++98双检锁示例
    static IocContainer& getInstance() {
        static IocContainer* instance = NULL;
        if (!instance) {
            LockGuard lock(&mutex_); // 需要实现锁机制
            if (!instance) {
                instance = new IocContainer();
            }
        }
        return *instance;
    }
#endif

private:
    // 服务工厂模板类
    template<typename T>
    class ServiceFactory : public ServiceFactoryBase {
    public:
#if CPP11_SUPPORTED
        std::unique_ptr<RpcService> create() override {
#else
        SafePtr<RpcService> create() override {
#endif
#if CPP11_SUPPORTED
            return std::unique_ptr<RpcService>(new T());
#else
            return SafePtr<RpcService>(new T());
#endif
        }
    };

    // 服务工厂映射
#if CPP11_SUPPORTED
    SERVICE_MAP<std::string, std::unique_ptr<ServiceFactoryBase>> factories_;
#else
    SERVICE_MAP<std::string, SafePtr<ServiceFactoryBase> > factories_;
#endif

    static pthread_mutex_t mutex_;
};

#endif // IOC_CONTAINER_H