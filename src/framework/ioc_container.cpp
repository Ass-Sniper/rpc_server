// src/framework/ioc_container.cpp
#include "framework/ioc_container.h"
#include "services/rpc_service.h"

std::unique_ptr<class RpcService> IocContainer::getService(const std::string& serviceId) 
{
    if (factories_.count(serviceId) == 0) {
        throw std::runtime_error("Service not registered: " + serviceId);
    }
    return factories_[serviceId]->create();
}

IocContainer& IocContainer::getInstance() 
{
    static IocContainer instance;
    return instance;
}