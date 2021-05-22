/*
Written by amoy @ v3rmillion.net

You need static lua 5.1.5 for this to work
*/

#include <iostream>
#include <windows.h>
#include <thread>
#include <vector>
#include <string>
#include <cstdint>

extern "C" {
#include "lua-5.1.5/lua.h"
#include "lua-5.1.5/lapi.h"
#include "lua-5.1.5/lauxlib.h"
#include "lua-5.1.5/lualib.h"
}

std::vector<lua_State*> yielded_states = {};
constexpr const bool IS_DEBUGGING = false;

template <class... arguments>
int dprintf(const char* const fmt, arguments... args) {
    if constexpr (!IS_DEBUGGING)
        return 0;

    return printf(fmt, args...);
}

static int wait_for_tick(lua_State* ls) {
    yielded_states.push_back(ls);   
    return lua_yield(ls, 0);
}

void handle_script(lua_State* ls, int narg) {    
    int res = lua_resume(ls, narg);
    if (res == 0)
        dprintf("[*] state %p finished execution.\n", ls), lua_close(ls);
    else if (res == LUA_YIELD)
        dprintf("[*] yielded state %p\n", ls);
    else if (res > LUA_YIELD)
        dprintf("[-] state %p error: %s\n", ls, lua_tostring(ls, -1)), lua_close(ls);
}

void execute_script(lua_State* ls, const std::string& src) {
    luaL_loadstring(ls, src.c_str());
    std::thread(handle_script, ls, 0).detach();
}

void on_tick() {
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        while (!yielded_states.empty()) {
            lua_State* ls = yielded_states[yielded_states.size()-1];
            dprintf("[+] resuming closure at state %p\n", ls);
            std::thread(handle_script, ls, 0).detach();
            yielded_states.pop_back();
        }
    }
}

int main()
{
    std::thread(on_tick).detach();

    while (true) {
        std::string uin;
        std::getline(std::cin, uin);
        if (uin.empty()) continue;

        lua_State* ls = luaL_newstate();
        luaL_openlibs(ls);
        lua_register(ls, "wait_for_tick", wait_for_tick);

        dprintf("[+] starting closure at state %p\n", ls);
        execute_script(lua_newthread(ls), uin);
    }

    return EXIT_SUCCESS;
}
