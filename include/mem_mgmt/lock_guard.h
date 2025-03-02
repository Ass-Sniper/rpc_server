#ifndef LOCK_GUARD_H
#define LOCK_GUARD_H

#include <pthread.h> // 引入pthread库以支持线程安全

class LockGuard {
public:
    explicit LockGuard(pthread_mutex_t* mutex) : mutex_(mutex) {
        pthread_mutex_lock(mutex_);
    }
    ~LockGuard() {
        pthread_mutex_unlock(mutex_);
    }
private:
    pthread_mutex_t* mutex_;
};

#endif // LOCK_GUARD_H