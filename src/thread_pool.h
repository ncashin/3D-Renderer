#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace ThreadPool{
enum class ReturnState{
    COMPLETE,
    REDISPATCH,
};

void Initialize(uint32_t thread_count);
void Terminate();

void HandleDispatch();

void Dispatch(std::function<ReturnState()> function);

extern bool active;

extern std::vector<std::thread> thread_vector;

extern std::mutex dispatch_mutex;
extern std::condition_variable dispatch_condition_variable;
extern std::deque<std::function<ReturnState()>> dispatch_queue;
}
