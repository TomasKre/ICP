#pragma once

#include <string>

// ICP project functions

int lua_init(const std::string& filename);
int lua_pre_run(void);
int lua_post_run(void);