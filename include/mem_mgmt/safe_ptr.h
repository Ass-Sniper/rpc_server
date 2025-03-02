#ifndef SAFE_PTR_H
#define SAFE_PTR_H

#include <pthread.h> // 引入pthread库以支持线程安全
#include <cassert>
#include "lock_guard.h"
#include "weak_ptr.h"

// 安全指针类，支持线程安全的引用计数
template<typename T>
class SafePtr {
public:
    // 基础构造/析构
    explicit SafePtr(T* ptr = NULL, void (*deleter)(T*) = NULL) : ptr_(ptr), refCount_(new int(1)), deleter_(deleter) {
        pthread_mutex_init(&mutex_, NULL);
    }
    ~SafePtr() {
        LockGuard lock(&mutex_);
        if (--(*refCount_) == 0) {
            if (deleter_) {
                deleter_(ptr_);
            } else {
                delete ptr_;
            }
            delete refCount_;
        }
        pthread_mutex_destroy(&mutex_);
    }

    // 转移构造函数（资源所有权转移）
    SafePtr(SafePtr&& other) : ptr_(other.ptr_), refCount_(other.refCount_), deleter_(other.deleter_) {
        LockGuard lock(&other.mutex_);
        other.ptr_ = NULL;
        other.refCount_ = new int(1);
        other.deleter_ = NULL;
    }

    // 拷贝赋值运算符（深拷贝实现）
    SafePtr& operator=(const SafePtr& other) {
        if (this != &other) {
            LockGuard lock1(&mutex_);
            LockGuard lock2(&other.mutex_);
            if (--(*refCount_) == 0) {
                if (deleter_) {
                    deleter_(ptr_);
                } else {
                    delete ptr_;
                }
                delete refCount_;
            }
            ptr_ = other.ptr_;
            refCount_ = other.refCount_;
            deleter_ = other.deleter_;
            (*refCount_)++;
        }
        return *this;
    }

    // 转移赋值运算符（资源所有权转移）
    SafePtr& operator=(SafePtr&& other) {
        if (this != &other) {
            LockGuard lock1(&mutex_);
            LockGuard lock2(&other.mutex_);
            if (--(*refCount_) == 0) {
                if (deleter_) {
                    deleter_(ptr_);
                } else {
                    delete ptr_;
                }
                delete refCount_;
            }
            ptr_ = other.ptr_;
            refCount_ = other.refCount_;
            deleter_ = other.deleter_;
            other.ptr_ = NULL;
            other.refCount_ = new int(1);
            other.deleter_ = NULL;
        }
        return *this;
    }

public:

    // 空值判断
    bool operator!() const { return !ptr_; }
    operator bool() const { return ptr_ != NULL; }

    // 访问器
    T* get() const { return ptr_; }
    T& operator*() const {
        assert(ptr_ && "Dereferencing null pointer");
        return *ptr_;
    }
    T* operator->() const {
        assert(ptr_ && "Accessing through null pointer");
        return ptr_;
    }

    // 资源管理
    T* release() {
        LockGuard lock(&mutex_);
        T* tmp = ptr_;
        ptr_ = NULL;
        return tmp;
    }
    void reset(T* ptr = NULL, void (*deleter)(T*) = NULL) {
        LockGuard lock(&mutex_);
        if (--(*refCount_) == 0) {
            if (deleter_) {
                deleter_(ptr_);
            } else {
                delete ptr_;
            }
            delete refCount_;
        }
        ptr_ = ptr;
        refCount_ = new int(1);
        deleter_ = deleter;
    }

    // 引用计数
    int use_count() const {
        LockGuard lock(&mutex_);
        int count = *refCount_;
        return count;
    }

private:
    T* ptr_;
    int* refCount_;
    void (*deleter_)(T*);
    mutable pthread_mutex_t mutex_;
};

#endif // SAFE_PTR_H