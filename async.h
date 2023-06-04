#pragma once

#include <iostream>

#if defined(_WIN32)
    //  Microsoft 
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#elif defined(__linux)
    //  GCC
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    //  do nothing and hope for the best?
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif

// error types
enum message
{
	OK,
	CONTEXT_NOT_FOUND
};

// input - bulk block size 
// return - context
EXPORT size_t connect(size_t block_size);

// input - buff (command buffer), buff_size - size of command, context
// return - message code
EXPORT uint8_t receive(const char* buff, size_t buff_size, size_t context);

// input - context
// return - message code
EXPORT uint8_t disconnect(size_t context);