#include "posi.h"

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <cstring>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

lua_State* L;

// Module cache
std::unordered_map<std::string, int> module_cache;

// Set to track modules currently being loaded
std::unordered_set<std::string> loading_modules;

int error_handler_index;

int my_require_cpp(lua_State* L) {
    const char* module_name_cstr = luaL_checkstring(L, 1);
    std::string module_name = module_name_cstr;

    // 1. Check Module Cache (_LOADED table in Lua registry, like standard require)
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE); // Push _LOADED table onto stack (at index -1)
    lua_getfield(L, -1, module_name.c_str()); // Push _LOADED[module_name] onto stack (at index -1, table at -2)
    if (lua_toboolean(L, -1)) { // Check if _LOADED[module_name] is truthy (module loaded)
        lua_remove(L, -2); // Remove _LOADED table from stack, keep only the loaded module value
        return 1; // Return 1 value (the cached module from _LOADED)
    }
    lua_pop(L, 1); // Pop the result of getfield (nil or false), keep _LOADED table on stack for later use


    // 2. Check for Recursive Inclusion (same as before)
    if (loading_modules.count(module_name)) {
        lua_remove(L, -1); // Clean up _LOADED table from stack before error
        return luaL_error(L, "recursive module inclusion detected for module '%s'", module_name.c_str());
    }

    // 3. Mark Module as Loading (same as before)
    loading_modules.insert(module_name);

    // 4. Module Retrieval (same as before)
    std::optional<std::vector<uint8_t>> module_bytes_opt = dbLoadByName("code", module_name);

    if (!module_bytes_opt.has_value()) {
        loading_modules.erase(module_name);
        lua_remove(L, -1); // Clean up _LOADED table from stack before error
        return luaL_error(L, "module '%s' not found", module_name.c_str());
    }

    std::vector<uint8_t> module_bytes = module_bytes_opt.value();

    // 5. Module Loading and Execution (same as before)
    if (luaL_loadbuffer(L, (const char*)module_bytes.data(), module_bytes.size(), module_name.c_str()) != LUA_OK) {
        loading_modules.erase(module_name);
        lua_remove(L, -2); // Clean up _LOADED table from stack before error
        return luaL_error(L, "%s", lua_tostring(L, -1));
    }

    if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
        loading_modules.erase(module_name);
        lua_remove(L, -2); // Clean up _LOADED table from stack before error
        return luaL_error(L, "%s", lua_tostring(L, -1));
    }

    // 6. Module Caching (_LOADED table, like standard require - modified caching logic)
    if (!lua_isnil(L, -1)) { // If module returned a non-nil value
        lua_pushvalue(L, -1); // Duplicate the module's return value (top of stack)
        lua_setfield(L, -3, module_name.c_str()); // _LOADED[module_name] = module's return value. Table is at -3, key at -2, value at -1 (_LOADED table is still on stack)
        loading_modules.erase(module_name); // Remove after successful load
        lua_remove(L, -2); // Pop the _LOADED table from stack (after setting the field)
        return 1; // Return 1 value (the module's return value)
    } else { // If module returned nil or nothing (lua_gettop(L) == 0 after pcall can also imply no returns, but nil is more explicit)
        lua_pop(L, 1); // Pop the nil value from stack (result of pcall)
        lua_pushboolean(L, 1); // Push 'true' onto stack (as per standard require when module returns nothing)
        lua_setfield(L, -3, module_name.c_str()); // _LOADED[module_name] = true. Table at -3, key at -2, value at -1 (_LOADED table still on stack)
        loading_modules.erase(module_name); // Remove after successful (but value-less) load
        lua_remove(L, -2); // Pop the _LOADED table from stack (after setting the field)
        lua_pushboolean(L, 1); // Push 'true' again as the return value (as per standard require when module returns nothing)
        return 1; // Return 1 value (true)
    }
}

void printLuaError(lua_State *L) {
    // Get the error message from the top of the stack
    const char *error_message = lua_tostring(L, -1);
    if (error_message) {
        fprintf(stderr, "Lua Error: \n%s\n", error_message);
    } else {
        fprintf(stderr, "Lua Error occurred, but could not retrieve message.\n");
    }  
	
    lua_pop(L, 1); // Pop the error message from the stack - crucial to clean up the stack
}

static int lua_api_cls(lua_State *L) {
    lua_Integer color; // Lua integer type (can hold 32-bit integers)

    if (lua_gettop(L) < 1) {
        return luaL_error(L, "API_cls requires at least one argument (color).");
    }

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "API_cls argument 'color' must be a number.");
    }

    color = luaL_checkinteger(L, 1); // Get integer, error if not an integer

    posiAPICls((int32_t)color); // Call your native C function, casting to int32_t

    return 0; // Indicate success, no return value to Lua (like JS_UNDEFINED)
}

static int lua_api_isPressed(lua_State *L) {
    lua_Integer buttonNumber;

    if (lua_gettop(L) != 1) {
        return luaL_error(L, "API_isPressed requires exactly one argument (buttonNumber).");
    }

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "API_isPressed argument 'buttonNumber' must be a number.");
    }

    buttonNumber = luaL_checkinteger(L, 1);

    if (buttonNumber < 0 || buttonNumber >= numInputButtons) { // Assuming numInputButtons is defined in your C code
        return luaL_argerror(L, 1, "buttonNumber is out of range"); // More specific arg error
    }

    bool result = API_isPressed((int32_t)buttonNumber); // Call your native C function

    lua_pushboolean(L, result); // Push the boolean result onto the Lua stack
    return 1; // Indicate that 1 value (boolean) is returned to Lua
}

// --- Lua Binding for API_isJustPressed ---
static int lua_api_isJustPressed(lua_State *L) {
    lua_Integer buttonNumber;

    if (lua_gettop(L) != 1) {
        return luaL_error(L, "API_isJustPressed requires exactly one argument (buttonNumber).");
    }

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "API_isJustPressed argument 'buttonNumber' must be a number.");
    }

    buttonNumber = luaL_checkinteger(L, 1);

    if (buttonNumber < 0 || buttonNumber >= numInputButtons) {
        return luaL_argerror(L, 1, "buttonNumber is out of range");
    }

    bool result = API_isJustPressed((int32_t)buttonNumber);

    lua_pushboolean(L, result);
    return 1;
}

// --- Lua Binding for API_isJustReleased ---
static int lua_api_isJustReleased(lua_State *L) {
    lua_Integer buttonNumber;

    if (lua_gettop(L) != 1) {
        return luaL_error(L, "API_isJustReleased requires exactly one argument (buttonNumber).");
    }

    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "API_isJustReleased argument 'buttonNumber' must be a number.");
    }

    buttonNumber = luaL_checkinteger(L, 1);

    if (buttonNumber < 0 || buttonNumber >= numInputButtons) {
        return luaL_argerror(L, 1, "buttonNumber is out of range");
    }

    bool result = API_isJustReleased((int32_t)buttonNumber);

    lua_pushboolean(L, result);
    return 1;
}

static int lua_api_putPixel(lua_State *L) {
    lua_Integer x, y;
    lua_Unsigned color; // Use lua_Unsigned for uint32_t (color)

    if (lua_gettop(L) != 3) {
        return luaL_error(L, "API_putPixel expects 3 arguments: x, y, color.");
    }

    if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "x must be a number");
    if (!lua_isnumber(L, 2)) return luaL_argerror(L, 2, "y must be a number");
    if (!lua_isnumber(L, 3)) return luaL_argerror(L, 3, "color must be a number");

    x = luaL_checkinteger(L, 1);
    y = luaL_checkinteger(L, 2);
    color = (lua_Unsigned)luaL_checkinteger(L, 3); // Use luaL_checkunsigned for uint32_t

    posiAPIPutPixel((int32_t)x, (int32_t)y, (uint32_t)color); // Call native C function

    return 0; // No return value to Lua
}

static int lua_api_drawSprite(lua_State *L) {
    lua_Integer id, w, h, x, y;
    bool flipHorz, flipVert;

    if (lua_gettop(L) != 7) {
        return luaL_error(L, "API_drawSprite expects 7 arguments.");
    }

    int arg_index = 1; // Lua stack indices are 1-based

    if (!lua_isnumber(L, arg_index)) return luaL_argerror(L, arg_index, "id must be a number");
    id = luaL_checkinteger(L, arg_index++);
    if (!lua_isnumber(L, arg_index)) return luaL_argerror(L, arg_index, "w must be a number");
    w = luaL_checkinteger(L, arg_index++);
    if (!lua_isnumber(L, arg_index)) return luaL_argerror(L, arg_index, "h must be a number");
    h = luaL_checkinteger(L, arg_index++);
    if (!lua_isnumber(L, arg_index)) return luaL_argerror(L, arg_index, "x must be a number");
    x = luaL_checkinteger(L, arg_index++);
    if (!lua_isnumber(L, arg_index)) return luaL_argerror(L, arg_index, "y must be a number");
    y = luaL_checkinteger(L, arg_index++);

    flipHorz = lua_toboolean(L, arg_index++); // lua_toboolean handles Lua booleans and truthy/falsy values
    flipVert = lua_toboolean(L, arg_index++);

    posiAPIDrawSprite((int32_t)id, (int32_t)w, (int32_t)h, (int32_t)x, (int32_t)y, flipHorz, flipVert);

    return 0; // No return value to Lua
}

static int l_posiAPIDrawTilemap(lua_State *L) {
    // 1. Get arguments from Lua stack and check their types.
    int tilemapNum = luaL_checkinteger(L, 1); // Get the 1st argument, ensure it's an integer
    int tmx = luaL_checkinteger(L, 2);        // Get the 2nd argument, ensure it's an integer
    int tmy = luaL_checkinteger(L, 3);        // Get the 3rd argument, ensure it's an integer
    int tmw = luaL_checkinteger(L, 4);        // Get the 4th argument, ensure it's an integer
    int tmh = luaL_checkinteger(L, 5);        // Get the 5th argument, ensure it's an integer
    int x = luaL_checkinteger(L, 6);          // Get the 6th argument, ensure it's an integer
    int y = luaL_checkinteger(L, 7);          // Get the 7th argument, ensure it's an integer

    // 2. Call the original C function posiAPIDrawTilemap.
    posiAPIDrawTilemap(tilemapNum, tmx, tmy, tmw, tmh, x, y);

    // 3. Return values to Lua (if any). In this case, posiAPIDrawTilemap is void, so we return nothing.
    return 0; // Indicate no return values to Lua.
}

static int l_posiAPITilemapEntry(lua_State *L) {
  int num_args = lua_gettop(L);

  if (num_args == 3) {
    // Assume it's a call to posiAPIGetTilemapEntry (3 arguments: tilemapNum, tmx, tmy)
    int tilemapNum = luaL_checkinteger(L, 1);
    int tmx = luaL_checkinteger(L, 2);
    int tmy = luaL_checkinteger(L, 3);

    uint16_t result = posiAPIGetTilemapEntry(tilemapNum, tmx, tmy);
    lua_pushinteger(L, (lua_Integer)result);
    return 1; // Return 1 value (the tilemap entry)

  } else if (num_args == 4) {
    // Assume it's a call to posiAPISetTilemapEntry (4 arguments: tilemapNum, tmx, tmy, entry)
    int tilemapNum = luaL_checkinteger(L, 1);
    int tmx = luaL_checkinteger(L, 2);
    int tmy = luaL_checkinteger(L, 3);
    uint16_t entry = (uint16_t)luaL_checkinteger(L, 4);

    posiAPISetTilemapEntry(tilemapNum, tmx, tmy, entry);
    return 0; // Return no value (void function)

  } else {
    // Incorrect number of arguments
    return luaL_error(L, "API_tilemapEntry: Incorrect number of arguments. Expected 3 (get) or 4 (set).");
  }
}

static int l_cppPrint(lua_State *L) {
    int nargs = lua_gettop(L); // Number of arguments passed from Lua
    std::string output = "";

    for (int i = 1; i <= nargs; ++i) {
        if (lua_isstring(L, i)) {
            output += lua_tostring(L, i);
        } else if (lua_isnumber(L, i)) {
            output += std::to_string(lua_tonumber(L, i));
        } else if (lua_isboolean(L, i)) {
            output += lua_toboolean(L, i) ? "true" : "false";
        } else if (lua_isnil(L, i)) {
            output += "nil";
        } else {
            output += lua_typename(L, lua_type(L, i)); // Type name if not string/number/bool/nil
        }
        if (i < nargs) {
            output += "\t"; // Add a tab separator between arguments, like standard Lua print
        }
    }
    std::cout << output << std::endl; // Print to C++ console (std::cout)
    return 0; // Number of return values to Lua (none in this case)
}

// Error handler function to be used with lua_pcall, using luaL_traceback
static int tracebackErrorHandler(lua_State *L) {
    // 'luaL_traceback' expects the error message to be at the top of the stack (index -1)
    // and the lua_State in which the error occurred (which is 'L' itself).
    luaL_traceback(L, L, lua_tostring(L, -1), 1);  // level 1 to skip tracebackErrorHandler itself
    return 1; // Return 1 value: the traceback string (now at top of stack)
}

void luaInit() {
    L = luaL_newstate();   // Create a new Lua stat
	// Create the _MODULE_CACHE table in Lua (global table)
    lua_newtable(L);
    lua_setglobal(L, "_MODULE_CACHE");

	luaopen_base(L);
	// 1. Base Library (basic functions like print, assert, error, etc.)
   // luaL_requiref(L, "", luaopen_base, 1);
   // lua_pop(L, 1); // Pop the library table from the stack. It's now in _G.

    // 2. Coroutine Library (coroutines and related functions)
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
    lua_pop(L, 1); // Pop coroutine library table.

    // 3. Table Library (table manipulation functions)
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1); // Pop table library table.

	// 3. Table Library (table manipulation functions)
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_pop(L, 1); // Pop table library table.

    // 6. String Library (string manipulation functions)
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L, 1); // Pop string library table.

    // 7. Math Library (mathematical functions)
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1); // Pop math library table.

    // Register your C API functions with Lua's global environment
    lua_register(L, "API_cls", lua_api_cls);
    lua_register(L, "API_isPressed", lua_api_isPressed);
    lua_register(L, "API_isJustPressed", lua_api_isJustPressed);
    lua_register(L, "API_isJustReleased", lua_api_isJustReleased);
    lua_register(L, "API_putPixel", lua_api_putPixel);
    lua_register(L, "API_drawSprite", lua_api_drawSprite);
	lua_register(L, "API_drawTilemap", l_posiAPIDrawTilemap);
	lua_register(L, "API_tilemapEntry", l_posiAPITilemapEntry);
	lua_register(L, "require", my_require_cpp);
	lua_register(L, "print", l_cppPrint);

    // No need to explicitly get and free a global object in Lua like in QuickJS C API
    // because lua_register directly works with the global environment.
	
	lua_pushcfunction(L, tracebackErrorHandler);
    error_handler_index = lua_gettop(L); // Get the index of the error handler
}

void luaDeinit() {
    if (L) {
        lua_close(L); // Close the Lua state, releasing resources
        L = NULL;      // Good practice to set the global state pointer to NULL
    }
}

bool luaCallTick() {
	
    int isfunc;

    // Get the global function "API_Tick"
    lua_getglobal(L, "API_Tick");

    // Check if it's a function
    isfunc = lua_isfunction(L, -1);
    if (!isfunc) {
        fprintf(stderr, "Error: API_Tick() is not defined in the Lua script.\n");
        lua_pop(L, 1); // Pop the non-function value from the stack
        return false;
    }

    // Call the function (no arguments) in protected mode (pcall)
    int status = lua_pcall(L, 0, 0,error_handler_index); // 0 arguments, 0 expected return values, 0 error handler function

    if (status != LUA_OK) {
        printLuaError(L); // Call your Lua error printing function
        return false;
    }

    // If lua_pcall returns LUA_OK, the function executed without errors

    return true;
}



bool luaEvalMain(std::string code) {
    int status = luaL_loadbuffer(L, code.c_str(), code.size(), "main"); // Load the Lua code string

    if (status == LUA_OK) {
        // **--- Call lua_pcall with our custom error handler (msgh argument) ---**
        status = lua_pcall(L, 0, 0, error_handler_index);

        
    }

    if (status != LUA_OK) {
        printLuaError(L); // Handle and print any Lua errors
        return false;
    } else {
        return true; // Code executed without errors
    }
	
}