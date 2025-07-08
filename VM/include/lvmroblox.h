// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lua.h"

// Structure to track VM execution metrics
struct RobloxVMMetrics {
    int64_t executionTimeMs;
    size_t memoryUsed;
    int callDepth;
    int instructionsExecuted;
    bool timedOut;
    bool memoryLimitExceeded;
    bool stackOverflow;
};

// Enhanced error handling for Roblox VM
void roblox_vm_error(lua_State* L, const char* error);

// Check if execution has timed out
bool roblox_vm_check_timeout();

// Check if memory limit has been exceeded
bool roblox_vm_check_memory(lua_State* L);

// Check if call stack depth limit has been exceeded
bool roblox_vm_check_stack_depth(int depth);

// Initialize VM metrics for a new execution
void roblox_vm_init_metrics();

// Get current VM metrics
const RobloxVMMetrics& roblox_vm_get_metrics();

// Enhanced VM execution with safety checks and metrics
int roblox_vm_execute(lua_State* L, int nresults);

// Enhanced memory allocator with limits and tracking
void* roblox_vm_alloc(void* ud, void* ptr, size_t osize, size_t nsize);

// Create a new Roblox-optimized Lua state
lua_State* roblox_vm_newstate();

// Enhanced bytecode loader with security checks
int roblox_vm_load(lua_State* L, const char* chunkname, const char* bytecode, size_t bytecode_size);

// Enhanced function to safely call a Lua function with timeout and memory checks
int roblox_vm_pcall(lua_State* L, int nargs, int nresults);

// Register Roblox-specific security functions to the Lua state
void roblox_vm_register_security(lua_State* L);

// Enhanced garbage collection with metrics
int roblox_vm_gc(lua_State* L, int what, int data);

// Get detailed error information
const char* roblox_vm_get_error_details(lua_State* L);

// Enhanced table access with security checks
int roblox_vm_table_access(lua_State* L, int tableIndex, const char* key);

// Enhanced function to create a sandboxed environment for scripts
void roblox_vm_create_sandbox(lua_State* L);

// Enhanced function to detect and prevent infinite loops
void roblox_vm_setup_loop_detection(lua_State* L);

// Enhanced function to safely execute a script with all security measures
int roblox_vm_execute_script(lua_State* L, const char* script, size_t scriptLen, const char* chunkname);
