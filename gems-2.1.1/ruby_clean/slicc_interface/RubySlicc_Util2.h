#include "Global.h"
#include "Address.h"
#include "NodeID.h"
#include "Sequencer.h"
#include "RubyConfig.h"
#include "NetDest.h"

class Chip;


// Ideally invalidate address in all L1s
void ideal_inv_all_L1s(NodeID chip_id, Address addr);

// Ideally invalidate address in sharer L1s
void ideal_inv_sharer_L1s(NodeID chip_id, Address addr, NetDest Sharers);

// Ideally invalidate address in all sharers except requester
void ideal_inv_sharer_L1s_minus_requester(NodeID chip_id, Address addr, NetDest Sharers, MachineID requester);

// Check if protocol is ideal coherence
bool protocol_is_ideal_coherence();

// Check if benchmark contains memory fences
bool benchmark_contains_membar();

// Get a list of true L1 sharers (by probing the L1s)
NetDest get_true_sharers(NodeID chip_id, Address addr);
