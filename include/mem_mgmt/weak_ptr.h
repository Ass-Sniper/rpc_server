#ifndef WEAK_PTR_H
#define WEAK_PTR_H

#include "safe_ptr.h" // 确保正确包含 safe_ptr.h

template<typename T>
class SafePtr;

// 弱指针类，用于管理SafePtr的生命周期，不增加引用计数
template<typename T>
class WeakPtr {
public:
    // 默认构造函数，初始化指针和引用计数为NULL
    WeakPtr() : ptr_(NULL), refCount_(NULL) {}

    // 从SafePtr构造弱指针，增加引用计数
    WeakPtr(const SafePtr<T>& other) : ptr_(other.ptr_), refCount_(other.refCount_) {
        LockGuard lock(&other.mutex_);
        (*refCount_)++;
    }

    // 析构函数，减少引用计数
    ~WeakPtr() {
        LockGuard lock(&mutex_);
        if (--(*refCount_) == 0) {
            delete refCount_;
        }
    }

    // 拷贝构造函数，增加引用计数
    WeakPtr(const WeakPtr& other) : ptr_(other.ptr_), refCount_(other.refCount_) {
        LockGuard lock(&other.mutex_);
        (*refCount_)++;
    }

    // 拷贝赋值运算符，增加引用计数
    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            LockGuard lock1(&mutex_);
            LockGuard lock2(&other.mutex_);
            if (--(*refCount_) == 0) {
                delete refCount_;
            }
            ptr_ = other.ptr_;
            refCount_ = other.refCount_;
            (*refCount_)++;
        }
        return *this;
    }

    // 尝试提升弱指针为强指针，如果对象仍然存在
    SafePtr<T> lock() const {
        LockGuard lock(&mutex_);
        if (*refCount_ > 0) {
            return SafePtr<T>(ptr_);
        }
        return SafePtr<T>();
    }

    // 检查弱指针是否失效
    bool expired() const {
        LockGuard lock(&mutex_);
        bool result = *refCount_ == 0;
        return result;
    }

private:
    T* ptr_; // 指向对象的指针
    int* refCount_; // 引用计数指针
    mutable pthread_mutex_t mutex_; // 互斥锁，保证线程安全
};

#endif // WEAK_PTR_H