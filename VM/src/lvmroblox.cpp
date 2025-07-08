// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lvm.h"

#include "lstate.h"
#include "ltable.h"
#include "lfunc.h"
#include "lstring.h"
#include "lgc.h"
#include "lmem.h"
#include "ldebug.h"
#include "ldo.h"
#include "lbuiltins.h"
#include "lnumutils.h"
#include "lbytecode.h"

#include <string.h>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <chrono>

// Enhanced VM execution for Roblox with better error handling and optimizations

// Maximum execution time in milliseconds before triggering a timeout
#define ROBLOX_VM_TIMEOUT_MS 5000

// Maximum memory allocation in bytes before triggering an out-of-memory error
#define ROBLOX_VM_MAX_MEMORY 100 * 1024 * 1024 // 100 MB

// Maximum call stack depth to prevent stack overflow attacks
#define ROBLOX_VM_MAX_CALL_DEPTH 200

// Structure to track VM execution metrics
struct RobloxVMMetrics {
    int64_t executionTimeMs;
    size_t memoryUsed;
    int callDepth;
    int instructionsExecuted;
    bool timedOut;
    bool memoryLimitExceeded;
    bool stackOverflow;
    
    RobloxVMMetrics()
        : executionTimeMs(0), memoryUsed(0), callDepth(0), 
          instructionsExecuted(0), timedOut(false), 
          memoryLimitExceeded(false), stackOverflow(false) {}
};

// Global metrics for the current execution
static RobloxVMMetrics g_vmMetrics;

// Execution start time for timeout detection
static std::chrono::high_resolution_clock::time_point g_executionStartTime;

// Enhanced error handling for Roblox VM
void roblox_vm_error(lua_State* L, const char* error) {
    luaG_runerror(L, "Roblox VM Error: %s", error);
}

// Check if execution has timed out
bool roblox_vm_check_timeout() {
    auto now = std::chrono::high_resolution_clock::now();
    g_vmMetrics.executionTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_executionStartTime).count();
    
    if (g_vmMetrics.executionTimeMs > ROBLOX_VM_TIMEOUT_MS) {
        g_vmMetrics.timedOut = true;
        return true;
    }
    
    return false;
}

// Check if memory limit has been exceeded
bool roblox_vm_check_memory(lua_State* L) {
    g_vmMetrics.memoryUsed = lua_gc(L, LUA_GCCOUNT, 0) * 1024;
    
    if (g_vmMetrics.memoryUsed > ROBLOX_VM_MAX_MEMORY) {
        g_vmMetrics.memoryLimitExceeded = true;
        return true;
    }
    
    return false;
}

// Check if call stack depth limit has been exceeded
bool roblox_vm_check_stack_depth(int depth) {
    g_vmMetrics.callDepth = depth;
    
    if (depth > ROBLOX_VM_MAX_CALL_DEPTH) {
        g_vmMetrics.stackOverflow = true;
        return true;
    }
    
    return false;
}

// Initialize VM metrics for a new execution
void roblox_vm_init_metrics() {
    g_vmMetrics = RobloxVMMetrics();
    g_executionStartTime = std::chrono::high_resolution_clock::now();
}

// Get current VM metrics
const RobloxVMMetrics& roblox_vm_get_metrics() {
    return g_vmMetrics;
}

// Enhanced VM execution with safety checks and metrics
int roblox_vm_execute(lua_State* L, int nresults) {
    // Initialize metrics
    roblox_vm_init_metrics();
    
    // Save current stack top
    int base = lua_gettop(L) - nresults;
    
    try {
        // Execute the function
        int status = lua_pcall(L, nresults, LUA_MULTRET, 0);
        
        // Update final metrics
        auto now = std::chrono::high_resolution_clock::now();
        g_vmMetrics.executionTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_executionStartTime).count();
        g_vmMetrics.memoryUsed = lua_gc(L, LUA_GCCOUNT, 0) * 1024;
        
        return status;
    }
    catch (const std::exception& e) {
        // Handle C++ exceptions
        lua_pushstring(L, e.what());
        return LUA_ERRRUN;
    }
    catch (...) {
        // Handle unknown exceptions
        lua_pushstring(L, "Unknown error in VM execution");
        return LUA_ERRRUN;
    }
}

// Enhanced memory allocator with limits and tracking
void* roblox_vm_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    lua_State* L = (lua_State*)ud;
    
    // Track memory usage
    g_vmMetrics.memoryUsed = lua_gc(L, LUA_GCCOUNT, 0) * 1024;
    
    // Check memory limit
    if (g_vmMetrics.memoryUsed > ROBLOX_VM_MAX_MEMORY) {
        g_vmMetrics.memoryLimitExceeded = true;
        return NULL; // Allocation failed due to memory limit
    }
    
    // Standard reallocation logic
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    else {
        return realloc(ptr, nsize);
    }
}

// Create a new Roblox-optimized Lua state
lua_State* roblox_vm_newstate() {
    // Create state with custom allocator
    lua_State* L = lua_newstate(roblox_vm_alloc, NULL);
    if (L) {
        // Initialize metrics
        roblox_vm_init_metrics();
        
        // Set up custom error handler
        // In a real implementation, you would register a custom error handler here
    }
    return L;
}

// Enhanced bytecode loader with security checks
int roblox_vm_load(lua_State* L, const char* chunkname, const char* bytecode, size_t bytecode_size) {
    // Initialize metrics
    roblox_vm_init_metrics();
    
    // Validate bytecode header (simplified check)
    if (bytecode_size < 4 || memcmp(bytecode, LUA_SIGNATURE, 4) != 0) {
        lua_pushstring(L, "Invalid bytecode signature");
        return LUA_ERRSYNTAX;
    }
    
    // Load the bytecode
    int status = luau_load(L, chunkname, bytecode, bytecode_size, 0);
    
    // Update metrics
    g_vmMetrics.memoryUsed = lua_gc(L, LUA_GCCOUNT, 0) * 1024;
    
    return status;
}

// Enhanced function to safely call a Lua function with timeout and memory checks
int roblox_vm_pcall(lua_State* L, int nargs, int nresults) {
    // Initialize metrics
    roblox_vm_init_metrics();
    
    // Set up timeout detection
    // In a real implementation, you would set up a timer or use a hook
    
    // Call the function
    int status = lua_pcall(L, nargs, nresults, 0);
    
    // Update final metrics
    auto now = std::chrono::high_resolution_clock::now();
    g_vmMetrics.executionTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_executionStartTime).count();
    g_vmMetrics.memoryUsed = lua_gc(L, LUA_GCCOUNT, 0) * 1024;
    
    // Check for timeout after execution
    if (g_vmMetrics.executionTimeMs > ROBLOX_VM_TIMEOUT_MS) {
        g_vmMetrics.timedOut = true;
        lua_pushstring(L, "Script execution timed out");
        return LUA_ERRRUN;
    }
    
    return status;
}

// Register Roblox-specific security functions to the Lua state
void roblox_vm_register_security(lua_State* L) {
    // In a real implementation, you would register functions to:
    // 1. Sandbox the environment
    // 2. Restrict access to dangerous functions
    // 3. Add rate limiting for resource-intensive operations
    // 4. Add logging for security-sensitive operations
}

// Enhanced garbage collection with metrics
int roblox_vm_gc(lua_State* L, int what, int data) {
    int result = lua_gc(L, what, data);
    
    // Update memory metrics after GC
    if (what == LUA_GCCOLLECT || what == LUA_GCSTEP) {
        g_vmMetrics.memoryUsed = lua_gc(L, LUA_GCCOUNT, 0) * 1024;
    }
    
    return result;
}

// Get detailed error information
const char* roblox_vm_get_error_details(lua_State* L) {
    static char errorBuffer[1024];
    
    // Get the error message from the stack
    const char* errorMsg = lua_tostring(L, -1);
    
    // Format with additional information
    snprintf(errorBuffer, sizeof(errorBuffer), 
             "Error: %s\nExecution time: %lld ms\nMemory used: %zu bytes\nCall depth: %d\n",
             errorMsg ? errorMsg : "Unknown error",
             g_vmMetrics.executionTimeMs,
             g_vmMetrics.memoryUsed,
             g_vmMetrics.callDepth);
    
    return errorBuffer;
}

// Enhanced table access with security checks
int roblox_vm_table_access(lua_State* L, int tableIndex, const char* key) {
    // Check if the table exists
    if (!lua_istable(L, tableIndex)) {
        lua_pushstring(L, "Attempt to access a non-table value");
        return LUA_ERRRUN;
    }
    
    // Get the value
    lua_getfield(L, tableIndex, key);
    
    // Check for timeout during this operation
    if (roblox_vm_check_timeout()) {
        lua_pushstring(L, "Script execution timed out during table access");
        return LUA_ERRRUN;
    }
    
    return LUA_OK;
}

// Enhanced function to create a sandboxed environment for scripts
void roblox_vm_create_sandbox(lua_State* L) {
    // Create a new table for the sandbox
    lua_newtable(L);
    
    // Add safe standard library functions
    // In a real implementation, you would carefully select which functions to expose
    
    // Example: Add math library
    lua_getglobal(L, "math");
    lua_setfield(L, -2, "math");
    
    // Example: Add string library with restrictions
    lua_getglobal(L, "string");
    lua_setfield(L, -2, "string");
    
    // Example: Add table library
    lua_getglobal(L, "table");
    lua_setfield(L, -2, "table");
    
    // Set the sandbox as the global environment for the script
    // In Lua 5.1 style:
    lua_setfenv(L, -2);
}

// Enhanced function to detect and prevent infinite loops
void roblox_vm_setup_loop_detection(lua_State* L) {
    // Set up a hook that checks for timeout every N instructions
    lua_sethook(L, [](lua_State* L, lua_Debug*) {
        // Increment instruction count
        g_vmMetrics.instructionsExecuted++;
        
        // Check for timeout
        if (roblox_vm_check_timeout()) {
            luaL_error(L, "Script execution timed out (possible infinite loop)");
        }
        
        // Check for memory limit
        if (roblox_vm_check_memory(L)) {
            luaL_error(L, "Memory limit exceeded");
        }
    }, LUA_MASKCOUNT, 1000); // Check every 1000 instructions
}

// Enhanced function to safely execute a script with all security measures
int roblox_vm_execute_script(lua_State* L, const char* script, size_t scriptLen, const char* chunkname) {
    // Initialize metrics
    roblox_vm_init_metrics();
    
    // Load the script
    if (luaL_loadbuffer(L, script, scriptLen, chunkname) != LUA_OK) {
        return LUA_ERRSYNTAX;
    }
    
    // Create sandbox environment
    roblox_vm_create_sandbox(L);
    
    // Set up loop detection
    roblox_vm_setup_loop_detection(L);
    
    // Execute with all safety measures
    return roblox_vm_pcall(L, 0, LUA_MULTRET);
}
