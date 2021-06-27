#pragma once
#include "search.hpp"
// Start the uci interface
void uci();
void uci_info(depth_t depth, score_t eval, unsigned long nodes, unsigned long nps, PrincipleLine principle, unsigned int time, ply_t root_ply);