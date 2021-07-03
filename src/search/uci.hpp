#pragma once
#include "search.hpp"
// Start the uci interface
namespace UCI {
void uci();
void uci_info(depth_t depth, score_t eval, unsigned long nodes, unsigned long nps, PrincipleLine principle,
              unsigned int time, ply_t root_ply);
void uci_info(depth_t depth, unsigned long nodes, unsigned long nps, unsigned int time);
void uci_info_nodes(unsigned long nodes, unsigned long nps);
inline bool uci_enabled = false; // So that info strings aren't pprinted to stdout during tests.
} // namespace UCI