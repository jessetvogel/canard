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

#define CANARD_LOG(m) do { std::cerr << "[🦆 LOG] " << m << std::endl; } while(false)
#define CANARD_ERROR(m) do { std::cerr << "[🦆 ERROR] " << m << std::endl; } while(false)
#define CANARD_ASSERT(expr, m) do { if(!(expr)) { std::cerr << "[🦆 ASSERT FAILED] " << m << std::endl; assert(false); } } while(false)
