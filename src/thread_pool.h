#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace core{
class Threadpool{
public:
    enum TaskState{
        TASK_COMPLETE,
        TASK_NOT_READY,
    };
    
    void Initialize(uint32_t thread_count);
    void Terminate();
    
    void HandleDispatch();
    
    void Dispatch(std::function<TaskState()> function);
    
    bool active;
    
    std::vector<std::thread> thread_vector;
    
    std::mutex dispatch_mutex;
    std::condition_variable dispatch_condition_variable;
    std::deque<std::function<TaskState()>> dispatch_queue;
};
extern Threadpool threadpool;
}
