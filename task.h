#pragma once

#include <vector>
#include <functional>


/* Task is defined using its ID, the functions it works on
    and all other tasks it is depends upon(the prerequisites)*/
    
struct Task{
    int id;
    std::function <void()> work;
    std::vector <int> dependencies;
};