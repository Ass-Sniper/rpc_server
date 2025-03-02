// include/framework/ioc_container.h
#ifndef IOC_CONTAINER_H
#define IOC_CONTAINER_H

#include <memory>
#include <string>
#include <unordered_map>

class ServiceFactoryBase {
public:
    virtual ~ServiceFactoryBase() {}
    virtual std::unique_ptr<class RpcService> create() = 0;
};

class IocContainer {
public:
    template<typename T>
    void registerService(const std::string& serviceId) {
        // C++14 标准写法
        // factories_[serviceId] = std::make_unique<ServiceFactory<T>>();
        
        // C++11 兼容写法
        factories_[serviceId].reset(new ServiceFactory<T>());
    }
    
    std::unique_ptr<class RpcService> getService(const std::string& serviceId);

    static IocContainer& getInstance();

private:
    template<typename T>
    class ServiceFactory : public ServiceFactoryBase {
    public:
        std::unique_ptr<class RpcService> create() 
        {
            return std::unique_ptr<class RpcService>(new T());
        }
        
    };

    std::unordered_map<std::string, std::unique_ptr<ServiceFactoryBase>> factories_;
};

#endif