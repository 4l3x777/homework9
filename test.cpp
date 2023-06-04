#include <iostream>
#include <thread>
#include "async.h"

void tasks(std::string info)
{
    auto context = connect(2);

    std::cout << info << " context: " << std::to_string(context)  << std::endl;

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
    receive("EOF", 1, context);

    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(5000));
    disconnect(context);
    std::cout << info << " exit" << std::endl;
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