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


    bool result = API_isJustReleased((int32_t)buttonNumber);

    lua_pushboolean(L, result);
    return 1;
}

static int lua_api_pixel(lua_State *L) {
    int num_args = lua_gettop(L);
    lua_Integer x, y;
    lua_Unsigned color; // Use lua_Unsigned for uint32_t (color)

    if (num_args == 3) {
        if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "x must be a number");
        if (!lua_isnumber(L, 2)) return luaL_argerror(L, 2, "y must be a number");
        if (!lua_isnumber(L, 3)) return luaL_argerror(L, 3, "color must be a number");

        x = luaL_checkinteger(L, 1);
        y = luaL_checkinteger(L, 2);
        color = (lua_Unsigned)luaL_checkinteger(L, 3); // Use luaL_checkunsigned for uint32_t

        posiAPIPutPixel((int32_t)x, (int32_t)y, (uint32_t)color); // Call native C function

        return 0; // No return value to Lua
    } else if (num_args == 2) {
        if (!lua_isnumber(L, 1)) return luaL_argerror(L, 1, "x must be a number");
        if (!lua_isnumber(L, 2)) return luaL_argerror(L, 2, "y must be a number");

        x = luaL_checkinteger(L, 1);
        y = luaL_checkinteger(L, 2);

        uint32_t retrieved_color = posiAPIGetPixel((int32_t)x, (int32_t)y);
        lua_pushinteger(L, (lua_Integer)retrieved_color); // Push the color value onto the Lua stack
        return 1; // Return one value to Lua
    } else {
        return luaL_error(L, "API_putPixel expects 2 or 3 arguments: x, y, [color].");
    }
}

// Lua C function for the posiAPITilePagePixel pair
static int l_posiAPITilePagePixel(lua_State *L) {
  int n = lua_gettop(L);

  if (n == 3) {
    if (!lua_isinteger(L, 1) || !lua_isinteger(L, 2) || !lua_isinteger(L, 3)) {
      return luaL_error(L, "Expected three integer arguments for posiAPITilePagePixel (get)");
    }
    int pageNum = lua_tointeger(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    uint32_t result = gpuGetTilePagePixel(pageNum, x, y);
    lua_pushinteger(L, result);
    return 1;
  } else if (n == 4) {
    if (!lua_isinteger(L, 1) || !lua_isinteger(L, 2) || !lua_isinteger(L, 3) || !lua_isinteger(L, 4)) {
      return luaL_error(L, "Expected four integer arguments for posiAPITilePagePixel (set)");
    }
    int pageNum = lua_tointeger(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    uint32_t color = lua_tointeger(L, 4);
    gpuSetTilePagePixel(pageNum, x, y, color);
    return 0;
  } else {
    return luaL_error(L, "Wrong number of arguments for posiAPITilePagePixel. Expected 3 (get) or 4 (set), got %d", n);
  }
}

// Lua C function for the posiAPITilePixel pair
static int l_posiAPITilePixel(lua_State *L) {
  int n = lua_gettop(L);

  if (n == 3) {
    if (!lua_isinteger(L, 1) || !lua_isinteger(L, 2) || !lua_isinteger(L, 3)) {
      return luaL_error(L, "Expected three integer arguments for posiAPITilePixel (get)");
    }
    int tileNum = lua_tointeger(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    uint32_t result = gpuGetTilePixel(tileNum, x, y);
    lua_pushinteger(L, result);
    return 1;
  } else if (n == 4) {
    if (!lua_isinteger(L, 1) || !lua_isinteger(L, 2) || !lua_isinteger(L, 3) || !lua_isinteger(L, 4)) {
      return luaL_error(L, "Expected four integer arguments for posiAPITilePixel (set)");
    }
    int tileNum = lua_tointeger(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);
    uint32_t color = lua_tointeger(L, 4);
    gpuSetTilePixel(tileNum, x, y, color);
    return 0;
  } else {
    return luaL_error(L, "Wrong number of arguments for posiAPITilePixel. Expected 3 (get) or 4 (set), got %d", n);
  }
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

static int l_posiAPIDrawLine(lua_State *L) {
    // Check the number of arguments. Expected 5: x1, y1, x2, y2, color
    if (lua_gettop(L) != 5) {
        luaL_error(L, "Expected 5 arguments: x1, y1, x2, y2, color");
        return 0;
    }

    // Get the arguments from Lua stack
    int x1 = luaL_checkinteger(L, 1);
    int y1 = luaL_checkinteger(L, 2);
    int x2 = luaL_checkinteger(L, 3);
    int y2 = luaL_checkinteger(L, 4);
    uint32_t color = luaL_checkinteger(L, 5); // Lua numbers are doubles by default

    // Call the original C function
    posiAPIDrawLine(x1, y1, x2, y2, color);

    return 0; // No return values to Lua
}

// Wrapper for posiAPIDrawRect
static int l_posiAPIDrawRect(lua_State *L) {
    if (lua_gettop(L) != 5) {
        luaL_error(L, "Expected 5 arguments: x1, y1, x2, y2, color");
        return 0;
    }
    int x1 = luaL_checkinteger(L, 1);
    int y1 = luaL_checkinteger(L, 2);
    int x2 = luaL_checkinteger(L, 3);
    int y2 = luaL_checkinteger(L, 4);
    uint32_t color = luaL_checkinteger(L, 5);
    posiAPIDrawRect(x1, y1, x2, y2, color);
    return 0;
}

// Wrapper for posiAPIDrawFilledRect
static int l_posiAPIDrawFilledRect(lua_State *L) {
    if (lua_gettop(L) != 5) {
        luaL_error(L, "Expected 5 arguments: x1, y1, x2, y2, color");
        return 0;
    }
    int x1 = luaL_checkinteger(L, 1);
    int y1 = luaL_checkinteger(L, 2);
    int x2 = luaL_checkinteger(L, 3);
    int y2 = luaL_checkinteger(L, 4);
    uint32_t color = luaL_checkinteger(L, 5);
    posiAPIDrawFilledRect(x1, y1, x2, y2, color);
    return 0;
}

// Wrapper for posiAPIDrawCircle
static int l_posiAPIDrawCircle(lua_State *L) {
    if (lua_gettop(L) != 4) {
        luaL_error(L, "Expected 4 arguments: centerX, centerY, radius, color");
        return 0;
    }
    int centerX = luaL_checkinteger(L, 1);
    int centerY = luaL_checkinteger(L, 2);
    int radius = luaL_checkinteger(L, 3);
    uint32_t color = luaL_checkinteger(L, 4);
    posiAPIDrawCircle(centerX, centerY, radius, color);
    return 0;
}

// Wrapper for posiAPIDrawFilledCircle
static int l_posiAPIDrawFilledCircle(lua_State *L) {
    if (lua_gettop(L) != 4) {
        luaL_error(L, "Expected 4 arguments: centerX, centerY, radius, color");
        return 0;
    }
    int centerX = luaL_checkinteger(L, 1);
    int centerY = luaL_checkinteger(L, 2);
    int radius = luaL_checkinteger(L, 3);
    uint32_t color = luaL_checkinteger(L, 4);
    posiAPIDrawFilledCircle(centerX, centerY, radius, color);
    return 0;
}

// Wrapper for posiAPIDrawTriangle
static int l_posiAPIDrawTriangle(lua_State *L) {
    if (lua_gettop(L) != 7) {
        luaL_error(L, "Expected 7 arguments: x1, y1, x2, y2, x3, y3, color");
        return 0;
    }
    int x1 = luaL_checkinteger(L, 1);
    int y1 = luaL_checkinteger(L, 2);
    int x2 = luaL_checkinteger(L, 3);
    int y2 = luaL_checkinteger(L, 4);
    int x3 = luaL_checkinteger(L, 5);
    int y3 = luaL_checkinteger(L, 6);
    uint32_t color = luaL_checkinteger(L, 7);
    posiAPIDrawTriangle(x1, y1, x2, y2, x3, y3, color);
    return 0;
}

// Wrapper for posiAPIDrawFilledTriangle
static int l_posiAPIDrawFilledTriangle(lua_State *L) {
    if (lua_gettop(L) != 7) {
        luaL_error(L, "Expected 7 arguments: x1, y1, x2, y2, x3, y3, color");
        return 0;
    }
    int x1 = luaL_checkinteger(L, 1);
    int y1 = luaL_checkinteger(L, 2);
    int x2 = luaL_checkinteger(L, 3);
    int y2 = luaL_checkinteger(L, 4);
    int x3 = luaL_checkinteger(L, 5);
    int y3 = luaL_checkinteger(L, 6);
    uint32_t color = luaL_checkinteger(L, 7);
    posiAPIDrawFilledTriangle(x1, y1, x2, y2, x3, y3, color);
    return 0;
}

static int lua_posiAPIDrawText(lua_State *L) {
    // Check the number of arguments
    int argc = lua_gettop(L);
    if (argc != 6) {
        luaL_error(L, "Expected 6 arguments, but got %d", argc);
    }

    // Get arguments from the Lua stack
    // Argument 1: text (string)
    size_t text_len;
    const char* text_c_str = luaL_checklstring(L, 1, &text_len);
    std::string text(text_c_str, text_len);

    // Argument 2: x (integer)
    int x = luaL_checkinteger(L, 2);

    // Argument 3: y (integer)
    int y = luaL_checkinteger(L, 3);

    // Argument 6: proportional (boolean)
    // lua_toboolean returns 0 for false, 1 for true, and 0 for non-boolean types (but luaL_checktype could be used for stricter checking)
    bool proportional = lua_toboolean(L, 4);

    // Argument 7: color (uint32_t) - Lua numbers are typically doubles, but can represent integers
    uint32_t color = static_cast<uint32_t>(luaL_checkinteger(L, 5));

    // Argument 8: start (integer)
    int start = luaL_checkinteger(L, 6);

    // Call the original C++ function
    int result = posiAPIDrawText(text, x, y, proportional, color, start);

    // Push the result back to the Lua stack
    lua_pushinteger(L, result);

    // Return the number of results pushed onto the stack
    return 1;
}

static int l_posiAPITilemapEntry(lua_State *L) {
  int num_args = lua_gettop(L);

  if (num_args == 3) {
    // Assume it's a call to posiAPIGetTilemapEntry (3 arguments: tilemapNum, tmx, tmy)
    int tilemapNum = luaL_checkinteger(L, 1);
    int tmx = luaL_checkinteger(L, 2);
    int tmy = luaL_checkinteger(L, 3);

    auto result = posiAPIGetTilemapEntry(tilemapNum, tmx, tmy);
    lua_pushinteger(L, (lua_Integer)result);
    return 1; // Return 1 value (the tilemap entry)

  } else if (num_args == 4) {
    
    int tilemapNum = luaL_checkinteger(L, 1);
    int tmx = luaL_checkinteger(L, 2);
    int tmy = luaL_checkinteger(L, 3);
    uint16_t entry = (uint16_t)luaL_checkinteger(L, 4);

    posiAPISetTilemapEntry(tilemapNum, tmx, tmy, entry);
    return 0; // Return no value (void function)

  } else {
    // Incorrect number of arguments
    return luaL_error(L, "API_tilemapEntry: Incorrect number of arguments.");
  }
}

static int l_posiAPIOperatorParameter(lua_State *L) {
  int num_args = lua_gettop(L);

  if (num_args == 4) {
    // Setter: channelNumber, operatorNumber, parameter, value
    int channelNumber = luaL_checkinteger(L, 1);
    uint8_t operatorNumber = (uint8_t)luaL_checkinteger(L, 2);
    uint8_t parameter = (uint8_t)luaL_checkinteger(L, 3);
    float value = (float)luaL_checknumber(L, 4);

    posiAPISetOperatorParameter(channelNumber, operatorNumber, parameter, value);
    return 0; // No return values
  } else if (num_args == 3) {
    // Getter: channelNumber, operatorNumber, parameter
    int channelNumber = luaL_checkinteger(L, 1);
    uint8_t operatorNumber = (uint8_t)luaL_checkinteger(L, 2);
    uint8_t parameter = (uint8_t)luaL_checkinteger(L, 3);

    float result = posiAPIGetOperatorParameter(channelNumber, operatorNumber, parameter);
    lua_pushnumber(L, (lua_Number)result);
    return 1; // One return value
  } else {
    // Invalid number of arguments
    luaL_error(L, "Wrong number of arguments. Expected 3 (get) or 4 (set).");
    return 0; // Should not reach here
  }
}

// Combined Lua binding for setting or getting global parameter
static int l_posiAPIGlobalParameter(lua_State *L) {
  int num_args = lua_gettop(L);

  if (num_args == 3) {
    // Setter: channelNumber, parameter, value
    int channelNumber = luaL_checkinteger(L, 1);
    uint8_t parameter = (uint8_t)luaL_checkinteger(L, 2);
    float value = (float)luaL_checknumber(L, 3);

    posiAPISetGlobalParameter(channelNumber, parameter, value);
    return 0; // No return values
  } else if (num_args == 2) {
    // Getter: channelNumber, parameter
    int channelNumber = luaL_checkinteger(L, 1);
    uint8_t parameter = (uint8_t)luaL_checkinteger(L, 2);

    float result = posiAPIGetGlobalParameter(channelNumber, parameter);
    lua_pushnumber(L, (lua_Number)result);
    return 1; // One return value
  } else {
    // Invalid number of arguments
    luaL_error(L, "Wrong number of arguments. Expected 2 (get) or 3 (set).");
    return 0; // Should not reach here
  }
}

// Combined Lua binding for noteOff or releaseAll
static int l_posiAPINoteOff(lua_State *L) {
  int num_args = lua_gettop(L);
  int channelNumber = luaL_checkinteger(L, 1);

  if (num_args == 2) {
    // Note Off: channelNumber, note
    uint8_t note = (uint8_t)luaL_checkinteger(L, 2);
    posiAPINoteOff(channelNumber, note);
    return 0;
  } else if (num_args == 1) {
    // Release All: channelNumber
    posiAPIReleaseAll(channelNumber);
    return 0;
  } else {
    // Invalid number of arguments
    luaL_error(L, "Wrong number of arguments. Expected 1 (releaseAll) or 2 (noteOff).");
    return 0; // Should not reach here
  }
}

// Lua binding for posiAPINoteOn
static int l_posiAPINoteOn(lua_State *L) {
  int channelNumber = luaL_checkinteger(L, 1);
  uint8_t note = (uint8_t)luaL_checkinteger(L, 2);
  uint8_t velocity = (uint8_t)luaL_checkinteger(L, 3);

  posiAPINoteOn(channelNumber, note, velocity);
  return 0;
}

// Lua binding for posiAPISetSustain
static int l_posiAPISetSustain(lua_State *L) {
  int channelNumber = luaL_checkinteger(L, 1);
  bool enable = lua_toboolean(L, 2); // Lua booleans are 0 or 1

  posiAPISetSustain(channelNumber, enable);
  return 0;
}

// Lua binding for posiAPISetModWheel
static int l_posiAPISetModWheel(lua_State *L) {
  int channelNumber = luaL_checkinteger(L, 1);
  uint8_t wheel = (uint8_t)luaL_checkinteger(L, 2);

  posiAPISetModWheel(channelNumber, wheel);
  return 0;
}

// Lua binding for posiAPISetPitchBend
static int l_posiAPISetPitchBend(lua_State *L) {
  int channelNumber = luaL_checkinteger(L, 1);
  uint16_t value = (uint16_t)luaL_checkinteger(L, 2);

  posiAPISetPitchBend(channelNumber, value);
  return 0;
}

static int lua_posiAPIIsSlotPresent(lua_State *L) {
    // Check and get the integer argument from the stack.
    // luaL_checkinteger is safer than lua_tointeger as it throws an error
    // if the argument is not an integer or convertible to one.
    int slotNum = luaL_checkinteger(L, 1); // 1 is the index of the first argument

    // Call the original C function
    bool is_present = posiAPIIsSlotPresent(slotNum);

    // Push the boolean result onto the Lua stack
    lua_pushboolean(L, is_present);

    // Return the number of results pushed onto the stack (1 in this case)
    return 1;
}

// Function to save a string from Lua to storage
// Lua signature: success = slotSaveString(slotNum, dataString)
// Returns: boolean
int lua_slotSave(lua_State* L) {
    // Check arguments: 1 number (slotNum), 1 string (dataString)
    if (lua_gettop(L) != 2 || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        // Use luaL_check* functions for more specific errors if preferred
        luaL_error(L, "Invalid arguments for slotSaveString(slotNum, dataString). Expected number, string.");
        return 0; // luaL_error jumps out, doesn't actually return
    }

    int slotNum = static_cast<int>(lua_tointeger(L, 1));
    size_t data_len;
    const char* dataString = lua_tolstring(L, 2, &data_len); // Get the string and its length

    // Convert the Lua string data (const char*, size_t) to std::vector<uint8_t>
    std::vector<uint8_t> data_to_save(dataString, dataString + data_len);

    // Call the external C++ function to save the vector
    bool storage_save_success = dbSlotSave(slotNum, data_to_save);

    // Push the result onto the Lua stack
    lua_pushboolean(L, storage_save_success);

    // Return the number of results pushed onto the stack
    return 1; // Pushed 1 result (the boolean)
}

// Function to load a string from storage for Lua
// Lua signature: dataString = slotLoadString(slotNum)
// Returns: string or nil
int lua_slotLoad(lua_State* L) {
    // Check arguments: 1 number (slotNum)
    if (lua_gettop(L) != 1 || !lua_isnumber(L, 1)) {
        luaL_error(L, "Invalid argument for slotLoadString(slotNum). Expected number.");
        return 0; // luaL_error jumps out
    }

    int slotNum = static_cast<int>(lua_tointeger(L, 1));

    // Call the external C++ function to load the vector<uint8_t>
    std::optional<std::vector<uint8_t>> loaded_data = dbSlotLoad(slotNum);

    // Check if loading succeeded (empty vector indicates no data or error)
    if (!loaded_data) {
        std::cerr << "C++ Load String: No save data found or failed to load data for slot " << slotNum << std::endl;
        // Push nil onto the Lua stack to indicate no data
        lua_pushnil(L);
    } else {
        // Convert the std::vector<uint8_t> data back to a Lua string
        // Use lua_pushlstring to handle potential embedded nulls correctly
        lua_pushlstring(L, reinterpret_cast<const char*>(loaded_data->data()), loaded_data->size());
    }

    // Return the number of results pushed onto the stack (either string or nil)
    return 1; // Pushed 1 result
}

static int lua_posiAPISlotDelete(lua_State *L) {
    // Get the number of arguments on the stack
    int num_args = lua_gettop(L);
    bool success = false;

    switch (num_args) {
        case 0:
            // No arguments: call posiAPISlotDeleteAll
            success = posiAPISlotDeleteAll();
            lua_pushboolean(L, success);
            return 1; // Return 1 result (the boolean)

        case 1: {
            // One argument: check if it's an integer and call posiAPISlotDelete
            // luaL_checkinteger is safer as it validates the type
            int slotNum = luaL_checkinteger(L, 1); // Get the first argument

            success = posiAPISlotDelete(slotNum);
            lua_pushboolean(L, success);
            return 1; // Return 1 result (the boolean)
        }

        default:
            // Incorrect number of arguments
            luaL_error(L, "Expected 0 or 1 argument, but received %d", num_args);
            return 0; // luaL_error does not return, but for completeness
    }
}

// Error handler function to be used with lua_pcall, using luaL_traceback
static int tracebackErrorHandler(lua_State *L) {
    // 'luaL_traceback' expects the error message to be at the top of the stack (index -1)
    // and the lua_State in which the error occurred (which is 'L' itself).
    luaL_traceback(L, L, lua_tostring(L, -1), 1);  // level 1 to skip tracebackErrorHandler itself
    return 1; // Return 1 value: the traceback string (now at top of stack)
}

// Define the API functions registration table
static const struct luaL_Reg api_funcs[] = {
    {"cls", lua_api_cls},
    {"isPressed", lua_api_isPressed},
    {"isJustPressed", lua_api_isJustPressed},
    {"isJustReleased", lua_api_isJustReleased},
    {"pixel", lua_api_pixel},
	{"tilePagePixel",l_posiAPITilePagePixel},
	{"tilePixel",l_posiAPITilePixel},
    {"drawSprite", lua_api_drawSprite},
    {"drawTilemap", l_posiAPIDrawTilemap},
	{"drawLine", l_posiAPIDrawLine},
	{"drawRect", l_posiAPIDrawRect},
	{"drawFilledRect", l_posiAPIDrawFilledRect},
	{"drawTri", l_posiAPIDrawTriangle},
	{"drawFilledTri", l_posiAPIDrawFilledTriangle},
	{"drawCircle", l_posiAPIDrawCircle},
	{"drawFilledCircle", l_posiAPIDrawFilledCircle},
	{"drawText", lua_posiAPIDrawText},
    {"tilemapEntry", l_posiAPITilemapEntry},
    {"operatorParameter", l_posiAPIOperatorParameter},
    {"globalParameter", l_posiAPIGlobalParameter},
    {"noteOn", l_posiAPINoteOn},
    {"noteOff", l_posiAPINoteOff},
    {"setSustain", l_posiAPISetSustain},
    {"setModWheel", l_posiAPISetModWheel},
    {"setPitchBend", l_posiAPISetPitchBend},
	{"isSlotPresent", lua_posiAPIIsSlotPresent},
	{"slotSave", lua_slotSave},
	{"slotLoad", lua_slotLoad},
	{"slotDelete", lua_posiAPISlotDelete},
    {NULL, NULL} // Sentinel value to mark the end of the array
};

// Function to open the API library
int luaopen_API(lua_State *L) {
    luaL_newlib(L, api_funcs);
    return 1; // Return the number of values pushed onto the stack (the table)
}

// Function to remove a Lua function (global or within a table),
// or remove an entire table if functionName is NULL
int removeLuaFunction(lua_State *L, const char *functionName, const char *tableName) {
    if (L == NULL) {
        fprintf(stderr, "Error: Invalid Lua state.\n");
        return 0; // Indicate failure
    }

    if (tableName != NULL && strlen(tableName) > 0 && functionName == NULL) {
        // Remove the entire table
        lua_pushnil(L);
        lua_setglobal(L, tableName);
        return 1; // Indicate success
    }

    if (functionName == NULL) {
        fprintf(stderr, "Error: Invalid function name (NULL) for global removal.\n");
        return 0; // Indicate failure
    }

    if (tableName == NULL || strlen(tableName) == 0) {
        // Remove a global function
        lua_pushnil(L);
        lua_setglobal(L, functionName);
        return 1; // Indicate success
    } else {
        // Remove a function from a table
        lua_getglobal(L, tableName);
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            lua_setfield(L, -2, functionName);
            lua_pop(L, 1); // Pop the table from the stack
            return 1; // Indicate success
        } else {
            fprintf(stderr, "Error: Table '%s' not found.\n", tableName);
            lua_pop(L, 1); // Clean up the stack
            return 0; // Indicate failure
        }
    }
}

void luaInit() {
    L = luaL_newstate();   // Create a new Lua stat
	// Create the _MODULE_CACHE table in Lua (global table)
    lua_newtable(L);
    lua_setglobal(L, "_MODULE_CACHE");

	luaL_openlibs(L);
	
	lua_register(L, "require", my_require_cpp);
	
	// Register the API table. luaopen_API will be called when 'require "API"' is used.
    luaL_requiref(L, "API", luaopen_API, 1);
    lua_pop(L, 1); // Pop the returned table, as we've already set it up

	removeLuaFunction(L, "dofile",nullptr);
	removeLuaFunction(L, "loadfile",nullptr);
	removeLuaFunction(L, "debug","debug");
	removeLuaFunction(L, nullptr, "package");
	removeLuaFunction(L, nullptr, "io");
	removeLuaFunction(L, nullptr, "file");
	removeLuaFunction(L, nullptr, "os");
	
	lua_pushcfunction(L, tracebackErrorHandler);
    error_handler_index = lua_gettop(L); // Get the index of the error handler
}

bool luaLoad() {
	auto retrieved_data = dbLoadByName("code", "main");
	if(!retrieved_data) {
		std::cout<<"Error: Could not load main script."<<std::endl;
		return false;
	}
	auto retrieved_code = std::string((*retrieved_data).begin(), (*retrieved_data).end());

	if(!luaEvalMain(retrieved_code)){
		return false;
	}
	
	return true;
}

void luaDeinit() {
    if (L) {
        lua_close(L); // Close the Lua state, releasing resources
        L = NULL;      // Good practice to set the global state pointer to NULL
    }
}

bool luaReset() {
	luaDeinit();
	luaInit();
	return luaLoad();	
}

bool luaCallInit() {
	
    int isfunc;

    // Get the global function "API_Tick"
    //lua_getglobal(L, "API_Tick");
	
	lua_getglobal(L, "API");
    lua_getfield(L, -1, "init");

    // Check if it's a function
    isfunc = lua_isfunction(L, -1);
    if (!isfunc) {
        lua_pop(L, 2); // Pop the non-function value from the stack
        return false;
    }

    // Call the function (no arguments) in protected mode (pcall)
    int status = lua_pcall(L, 0, 0,error_handler_index); // 0 arguments, 0 expected return values, 0 error handler function

    if (status != LUA_OK) {
        printLuaError(L); // Call your Lua error printing function
        return false;
    }
	// Pop the "Tick" function (or the non-function value) from the stack
    lua_pop(L, 1);
    // If lua_pcall returns LUA_OK, the function executed without errors

    return true;
}

bool luaCallTick() {
	
    int isfunc;

    // Get the global function "API_Tick"
    //lua_getglobal(L, "API_Tick");
	
	lua_getglobal(L, "API");
    // Get the "Tick" function from the API table
    lua_getfield(L, -1, "tick");

    // Check if it's a function
    isfunc = lua_isfunction(L, -1);
    if (!isfunc) {
        fprintf(stderr, "Error: API.tick() is not defined in the Lua script.\n");
        lua_pop(L, 2); // Pop the non-function value from the stack
        return false;
    }

    // Call the function (no arguments) in protected mode (pcall)
    int status = lua_pcall(L, 0, 0,error_handler_index); // 0 arguments, 0 expected return values, 0 error handler function

    if (status != LUA_OK) {
        printLuaError(L); // Call your Lua error printing function
        return false;
    }
	// Pop the "Tick" function (or the non-function value) from the stack
    lua_pop(L, 1);
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
		luaCallInit();
        return true; // Code executed without errors
    }
	
}