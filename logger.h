#pragma once

#include <iostream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <thread>
#include <iomanip>

std::mutex print_lock;

inline void logMessage(const std::string& msg);
inline std::string currentTimestamp();

inline std::string currentTimestamp() {

    auto now = std::chrono::system_clock::now();
    
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream curr_time;
    curr_time << std::put_time(std::localtime(&in_time_t), "%H:%M:%S ");
    return curr_time.str();
}

inline void logMessage(const std::string& msg){
    {
        std::lock_guard <std::mutex> lk(print_lock);
        std::cout << currentTimestamp() << "\tThread Id:" << std::this_thread::get_id() << "\t" << msg << std::endl;
    }
}