#include "thread_pool.h"

namespace core{
Threadpool threadpool{};
void Threadpool::Initialize(uint32_t thread_count){
    active = true;
    
    thread_vector.resize(thread_count);
    for(std::thread& thread : thread_vector){
        thread = std::thread(&Threadpool::HandleDispatch, this);
    }
}
void Threadpool::Terminate(){
    active = false;
    dispatch_mutex.lock();
    for(uint32_t i = 0; i < thread_vector.size(); i++){
        dispatch_queue.emplace_back([]{ return TASK_COMPLETE; });
    }
    dispatch_mutex.unlock();
    dispatch_condition_variable.notify_all();
    
    for(std::thread& thread : thread_vector){
        thread.join();
    }
}

void Threadpool::Dispatch(std::function<TaskState()> function){
    dispatch_mutex.lock();
    
    dispatch_queue.emplace_back(function);
    
    dispatch_mutex.unlock();
    
    dispatch_condition_variable.notify_one();
}


void Threadpool::HandleDispatch(){
    while(active){
        std::unique_lock<std::mutex> lock(dispatch_mutex);
        dispatch_condition_variable.wait(lock, [this]{ return dispatch_queue.size() > 0; });
        
        std::function<TaskState()> function;
        function = dispatch_queue.front();
        dispatch_queue.pop_front();
        
        lock.unlock();
        
        switch(function()){
            case TASK_COMPLETE:{
                break;
            }
            case TASK_NOT_READY:{
                lock.lock();
                dispatch_queue.emplace_back(function);
                lock.unlock();
                break;
            }
        }
    }
}
}
