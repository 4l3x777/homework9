#include "bulk.h"
#include <memory>
#include <unordered_map>
#include <random>
#include <mutex>

#include "async.h"

namespace bulk_handler
{

class BulkHandler 
{
    std::unique_ptr<bulk::FileOutput> file_output;
    std::unique_ptr<bulk::ConsoleOutput> console_output;
    std::unique_ptr<bulk::CommandProcessor> command_processor;
    std::unique_ptr<bulk::ThreadManager> thread_manager;
    std::unique_ptr<bulk::ConsoleInput> console_input;

public:
    BulkHandler(std::size_t bulk_max_depth, std::size_t context = 0) 
    {
        file_output = std::make_unique<bulk::FileOutput>();
        console_output = std::make_unique<bulk::ConsoleOutput>(file_output.get());
        command_processor = std::make_unique<bulk::CommandProcessor>(bulk_max_depth, console_output.get());
        console_input = std::make_unique<bulk::ConsoleInput>(command_processor.get());
        thread_manager = std::make_unique<bulk::ThreadManager>(file_output.get(), console_output.get(), context);    
    }

    void job(std::string command_text)
    {
        console_input->command_handler(bulk::command{command_text, std::chrono::system_clock::now()});
    }
};

class ConnectionManager
{
    std::mutex g_mutex;
    std::unordered_map<std::size_t, std::unique_ptr<BulkHandler>> connections;

    int generate_id()
    {
        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<std::mt19937::result_type> distance(1,0xfffffff);
        return distance(generator);
    }

public:
    std::size_t connect(std::size_t block_size)
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        auto id = generate_id();
        while(connections.find(id) != connections.end())
        {
            id = generate_id();
        }

        connections.insert(
            std::pair<std::size_t, std::unique_ptr<BulkHandler>>(
            id,
            std::make_unique<BulkHandler>(block_size, id)
            )
        );

        return id;
    }

    uint8_t receive(std::string command_text, std::size_t context)
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        if(connections.find(context) != connections.end())
        {
            connections.at(context)->job(command_text);
            return message::OK;
        }
        else return message::CONTEXT_NOT_FOUND;
    }

    uint8_t disconnect(std::size_t context)
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        if(connections.find(context) != connections.end())
        {
            connections.erase(context);
            return message::OK;
        }
        else return message::CONTEXT_NOT_FOUND;
    }
      
};

};

// global instance
bulk_handler::ConnectionManager manager;

EXPORT size_t connect(size_t block_size)
{
    return manager.connect(block_size);
}

EXPORT uint8_t receive(const char* buff, size_t buff_size, size_t context)
{
    return manager.receive(std::string(buff, buff_size), context);
}

EXPORT uint8_t disconnect(size_t context)
{
    return manager.disconnect(context);
}