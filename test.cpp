#include <iostream>
#include <thread>
#include <string>
#include "async.h"

void tasks(std::string info)
{
    auto context = connect(2);

    std::string message = info + " context: " + std::to_string(context) + "\n";
    std::cout << message;

    receive("cmd1", 4, context);
    receive("cmd2", 4, context);
    receive("cmd3", 4, context);
    receive("", 0, context);

    receive("{", 1, context);
    receive("cmd1", 4, context);
    receive("cmd2", 4, context);
    receive("cmd3", 4, context);
    receive("", 0, context);
    receive("}", 1, context);

    receive("{", 1, context);
    receive("cmd2", 4, context);
    receive("EOF", 3, context);

    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(5000));
    disconnect(context);
    
    message = info + " exit" + "\n";
    std::cout << message;
}

int main(int argc, char* argv[])
{
    std::thread thread1([] { tasks("thread1");});
    std::thread thread2([] { tasks("thread2");});
    std::thread thread3([] { tasks("thread3");});

    thread1.join();
    thread2.join();
    thread3.join();

    return 0;
}