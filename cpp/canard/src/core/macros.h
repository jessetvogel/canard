//
// Created by Jesse Vogel on 07/09/2021.
//

#pragma once

#include <iostream>
#include <cassert>

//#define DEBUG

#ifdef DEBUG
#define CANARD_DEBUG(m) do { std::cerr << "[🦆 DEBUG] " << m << std::endl; } while(false)
#else
#define CANARD_DEBUG(m) do {} while(false)
#endif

#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RESET "\033[0m"

#define CANARD_LOG(m) do { std::cerr << COLOR_GREEN "[🦆 LOG] " << m << COLOR_RESET << std::endl; } while(false)
#define CANARD_ERROR(m) do { std::cerr << COLOR_RED "[🦆 ERROR] " << m << COLOR_RESET << std::endl; } while(false)
#define CANARD_ASSERT(expr, m) do { if(!(expr)) { std::cerr << COLOR_RED "[🦆 ASSERT FAILED] " << m << COLOR_RESET << std::endl; assert(false); } } while(false)
