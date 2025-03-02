// src/framework/ioc_container.cpp
#include "framework/ioc_container.h"

// 初始化静态成员变量
pthread_mutex_t IocContainer::mutex_ = PTHREAD_MUTEX_INITIALIZER;