#include "posi.h"

#include <iostream>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

lua_State* L;

void printLuaError(lua_State *L) {
    // Get the error message from the top of the stack
    const char *error_message = lua_tostring(L, -1);
    if (error_message) {
        fprintf(stderr, "Lua Error: %s\n", error_message);
    } else {
        fprintf(stderr, "Lua Error occurred, but could not retrieve message.\n");
    }  

    // Get and print the stack trace (using debug library)
    luaL_traceback(L, L, error_message, 1); // 1 means get traceback from current call stack
    const char *stack_trace = lua_tostring(L, -1);
    if (stack_trace) {
        fprintf(stderr, "Stack Trace:\n%s\n", stack_trace);
        lua_pop(L, 1); // Pop the stack trace string
    } else {
        fprintf(stderr, "Stack trace exists, but couldn't retrieve its string.\n");
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

void luaInit() {
    L = luaL_newstate();   // Create a new Lua state
    luaL_openlibs(L);       // Open standard Lua libraries (optional, but often useful)

    // Register your C API functions with Lua's global environment
    lua_register(L, "API_cls", lua_api_cls);
    lua_register(L, "API_isPressed", lua_api_isPressed);
    lua_register(L, "API_isJustPressed", lua_api_isJustPressed);
    lua_register(L, "API_isJustReleased", lua_api_isJustReleased);
    lua_register(L, "API_putPixel", lua_api_putPixel);
    lua_register(L, "API_drawSprite", lua_api_drawSprite);

    // No need to explicitly get and free a global object in Lua like in QuickJS C API
    // because lua_register directly works with the global environment.
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
    int status = lua_pcall(L, 0, 0, 0); // 0 arguments, 0 expected return values, 0 error handler function
    if (status != LUA_OK) {
        printLuaError(L); // Call your Lua error printing function
        return false;
    }

    // If lua_pcall returns LUA_OK, the function executed without errors

    return true;
}

bool luaEvalMain(std::string code) {
    int status = luaL_loadstring(L, code.c_str()); // Load the Lua code string

    if (status == LUA_OK) {
        // Code loaded successfully, now execute it in protected mode
        status = lua_pcall(L, 0, 0, 0); // 0 args, 0 returns, 0 error handler
    }

    if (status != LUA_OK) {
        printLuaError(L); // Handle and print any Lua errors
        return false;
    } else {
        return true; // Code executed without errors
    }
}