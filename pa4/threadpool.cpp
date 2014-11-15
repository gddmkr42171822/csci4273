#include "threadpool.h"
using namespace std;

ThreadPool::ThreadPool(){
    ThreadPool(DEFAULT_SIZE);
}

ThreadPool::ThreadPool(size_t threadCount){
    this->threadCount = threadCount;
    this->availableThreads = threadCount;
    threads = new pthread_t[threadCount];
    for(unsigned int i = 0;i < threadCount;i++){
        pthread_create(&threads[i], NULL, &execute, (void*)this);
    }
}

ThreadPool::~ThreadPool(){
    cvLock.lock();
    terminate = true;
    cv.notify_all();
    cvLock.unlock();
    for(unsigned int i = 0;i < threadCount;i++){
        pthread_join(threads[i], NULL);
    }
    delete[] threads;
}

int ThreadPool::dispatch_thread(void dispatch_function(void*), void* arg){
    cvLock.lock();
    jobs.push_back(new job(dispatch_function, arg));
    cv.notify_one();
    cvLock.unlock();
    return 0;
}

bool ThreadPool::thread_avail(){
    bool result;
    cvLock.lock();
    result = availableThreads > 0;
    cvLock.unlock();
    return result;
}

void* ThreadPool::execute(void* tp){
    ThreadPool* pool = (ThreadPool*) tp;
    list<job*> *jobQueue = &(pool->jobs);
    while(!pool->terminate){
        job* myJob;
        unique_lock<mutex> uLock(pool->cvLock);
        bool empty = jobQueue->empty();
        if(empty){
            pool->cv.wait(uLock);
        }else{
            pool->availableThreads--;
            myJob = jobQueue->front();
            jobQueue->pop_front();
            uLock.unlock();
            myJob->function(myJob->arg);
            pool->cvLock.lock();
            pool->availableThreads++;
            pool->cvLock.unlock();
            delete(myJob);
        }
    }
    return (void*) 0;
}
