#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <queue>

#include <boost/lockfree/queue.hpp>
#include <boost/algorithm/string/join.hpp>

namespace bulk {

typedef struct
{
    std::string _text;
    std::chrono::system_clock::time_point _inition_timestamp;
} command, *pcommand;

class ICommandProcessor
{
public:
    ICommandProcessor(ICommandProcessor* next_command_processor = nullptr) : _next_command_processor(next_command_processor) {}

    virtual ~ICommandProcessor() = default;

    virtual void start_block() {}
    virtual void finish_block() {}

    virtual void command_handler(const command& command) = 0;

protected:
    ICommandProcessor* _next_command_processor;
};

class ConsoleInput : public ICommandProcessor
{
public:
    ConsoleInput(ICommandProcessor* next_command_processor = nullptr) : ICommandProcessor(next_command_processor), _block_depth(0) {}

    void command_handler(const command& command) override
    {
        if (_next_command_processor)
        {
            if (command._text == "{")
            {
                if (_block_depth++ == 0) _next_command_processor->start_block();
            }
            else if (command._text == "}")
            {
                if (--_block_depth == 0) _next_command_processor->finish_block();
            }
            else if (command._text == "EOF")
            {
                _block_depth = 0;
                _next_command_processor->command_handler(command);
            }
            else _next_command_processor->command_handler(command);
        }
    }

private:
    size_t _block_depth;
};

class ConsoleOutput : public ICommandProcessor
{
public:
    // standart queue
    std::queue<command> log_queue;

    ConsoleOutput(ICommandProcessor* next_command_processor = nullptr) : ICommandProcessor(next_command_processor) {}

    void command_handler(const command& command) override
    {
        log_queue.push(command);
        if (_next_command_processor) _next_command_processor->command_handler(command);
    }
};

class FileOutput : public ICommandProcessor
{
public:
    // lock-free queue
    boost::lockfree::queue<pcommand> file_queue{0};

    FileOutput(ICommandProcessor* next_command_processor = nullptr) : ICommandProcessor(next_command_processor) {}

    void command_handler(const command& command) override
    {
        if(!command._text.empty())
        {
            file_queue.push(new bulk::command(
                {
                    command._text,
                    command._inition_timestamp
                }
            ));
            if (_next_command_processor) _next_command_processor->command_handler(command);
        }
    }
};

class CommandProcessor : public ICommandProcessor
{
public:
    CommandProcessor(size_t bulk_max_depth, ICommandProcessor* next_command_processor) : ICommandProcessor(next_command_processor), _bulk_max_depth(bulk_max_depth), _used_block_command(false) {}

    ~CommandProcessor()
    {
        if (!_used_block_command) dump_commands();
    }

    void start_block() override
    {
        _used_block_command = true;
        dump_commands();
    }

    void finish_block() override
    {
        _used_block_command = false;
        dump_commands();
    }

    void command_handler(const command& command) override
    {
        _commands_pool.push_back(command);

        if (_used_block_command)
        {
            if (command._text == "EOF")
            {
                _used_block_command = false;
                _commands_pool.clear();
                return;
            }
        }
        else
        {
            if (command._text == "EOF" || _commands_pool.size() >= _bulk_max_depth)
            {
                dump_commands();
                return;
            }
        }
    }
private:
    void clear_commands_pool()
    {
        _commands_pool.clear();
    }

    void dump_commands()
    {
        if (_next_command_processor && !_commands_pool.empty())
        {
            auto commands_concat = concatenate_commands_pool();
            auto output =  !commands_concat.empty() ? commands_concat : "";
            _next_command_processor->command_handler(command{output, _commands_pool[0]._inition_timestamp});
        }
        clear_commands_pool();
    }

    std::string concatenate_commands_pool()
    {
        std::vector<std::string> command_vector;
        for(auto command: _commands_pool)
        {
            if (!command._text.empty()) command_vector.push_back(command._text);
        }
        return boost::algorithm::join(command_vector, ", ");
    }

    size_t _bulk_max_depth;
    bool _used_block_command;
    std::vector<command> _commands_pool;
};

class ThreadManager
{   
    FileOutput* file_stream{nullptr};
    ConsoleOutput* console_stream{nullptr};
    size_t context{0};

    std::string create_filename(pcommand command, int thread_id)
    {
        return 
            "bulk_" + 
            std::to_string(context) + 
            "_" + 
            std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(command->_inition_timestamp.time_since_epoch()).count()) + 
            "_" + 
            std::to_string(thread_id) + 
            ".log";
    }

    void file_write(int thread_id)
    {
        while (true) 
        {
            pcommand current_command;
            if (file_stream->file_queue.pop(current_command)) 
            {
                std::ofstream file(create_filename(current_command, thread_id), std::ofstream::out | std::ios::binary);
                file << current_command->_text;
                delete current_command;
            }
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(1000));    
        }
    }

    void console_write() 
    {
        while (true) 
        {
            if (!console_stream->log_queue.empty()) 
            {
                auto commands = console_stream->log_queue.front()._text;
                auto output =  !commands.empty() ? "bulk_" + std::to_string(context) + ": " + commands + "\n" : "";
                std::cout << output;
                console_stream->log_queue.pop();
            }
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(1000));
        }
    }

public:
    ThreadManager(FileOutput* fo, ConsoleOutput* co, size_t _context) : file_stream(fo), console_stream(co), context(_context)
    {
        std::thread log([&] { console_write();});
        log.detach();
        std::thread file1([&] { file_write(1);});
        file1.detach();
        std::thread file2([&] { file_write(2);});
        file2.detach();
    }
};

};
