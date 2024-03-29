#include "thread_pool.h"

namespace ThreadPool{
bool active = false;

std::vector<std::thread> thread_vector;

std::mutex dispatch_mutex;
std::condition_variable dispatch_condition_variable;
std::deque<std::function<ReturnState()>> dispatch_queue;
}

void ThreadPool::Initialize(uint32_t thread_count){
    active = true;
    
    thread_vector.resize(thread_count);
    for(std::thread& thread : thread_vector){
        thread = std::thread(&ThreadPool::HandleDispatch);
    }
}
void ThreadPool::Terminate(){
    active = false;
    dispatch_mutex.lock();
    for(uint32_t i = 0; i < thread_vector.size(); i++){
        dispatch_queue.emplace_back([]{ return ReturnState::COMPLETE; });
    }
    dispatch_mutex.unlock();
    dispatch_condition_variable.notify_all();
    
    for(std::thread& thread : thread_vector){
        thread.join();
    }
}

void ThreadPool::Dispatch(std::function<ReturnState()> function){
    dispatch_mutex.lock();
    
    dispatch_queue.emplace_back(function);
    
    dispatch_mutex.unlock();
    
    dispatch_condition_variable.notify_one();
}


void ThreadPool::HandleDispatch(){
    while(active){
        std::unique_lock<std::mutex> lock(dispatch_mutex);
        dispatch_condition_variable.wait(lock, []{ return dispatch_queue.size() > 0; });
        
        std::function<ReturnState()> function;
        function = dispatch_queue.front();
        dispatch_queue.pop_front();
        
        lock.unlock();
        
        switch(function()){
            case ReturnState::COMPLETE:{
                break;
            }
            case ReturnState::REDISPATCH:{
                lock.lock();
                dispatch_queue.emplace_back(function);
                lock.unlock();
                break;
            }
        }
    }
}
