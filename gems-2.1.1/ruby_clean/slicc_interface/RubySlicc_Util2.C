#include "RubySlicc_Util2.h"
#include "Chip.h"
#include "protocol_name.h"
#include "RubySlicc_ComponentMapping.h"

// Ideally invalidate address in all L1s
void ideal_inv_all_L1s(NodeID chip_id, Address addr) {
    assert(chip_id < RubyConfig::numberOfChips());
    for(int i=0; i<RubyConfig::numberOfProcsPerChip(); i++) {
        Sequencer* targetSequencer_ptr = g_system_ptr->getChip(chip_id)->getSequencer(i);
        targetSequencer_ptr->makeIdealInvRequest(
                CacheMsg( addr,
                   addr,
                   CacheRequestType_IDEAL_INV,
                   Address(0),    // PC
                   AccessModeType_UserMode,   // User/supervisor mode
                   0,   // Size in bytes of request
                   PrefetchBit_No, // Not a prefetch
                   0,              // Version number
                   addr,   // Virtual Address
                   0,              // SMT thread
                   0,          // TM specific - timestamp of memory request
                   false,      // TM specific - whether request is part of escape action
                   MemorySpaceType_NULL,    // memory space
                   false,                    // profiled yet?
                   0,         // access mask
                   0                  // memfetch pointer
                   )
                );
    }
}


// Ideally invalidate address in sharer L1s
void ideal_inv_sharer_L1s(NodeID chip_id, Address addr, NetDest Sharers) {
    assert(chip_id < RubyConfig::numberOfChips());
    while(Sharers.count() > 0) {
        MachineID L1Machine = Sharers.smallestElement(MachineType_L1Cache);
        int core_id = machineIDToVersion(L1Machine);
        assert(core_id < RubyConfig::numberOfL1CachePerChip());
        Sequencer* targetSequencer_ptr = g_system_ptr->getChip(chip_id)->getSequencer(core_id);
        targetSequencer_ptr->makeIdealInvRequest(
            CacheMsg( addr,
               addr,
               CacheRequestType_IDEAL_INV,
               Address(0),    // PC
               AccessModeType_UserMode,   // User/supervisor mode
               0,   // Size in bytes of request
               PrefetchBit_No, // Not a prefetch
               0,              // Version number
               addr,   // Virtual Address
               0,              // SMT thread
               0,          // TM specific - timestamp of memory request
               false,      // TM specific - whether request is part of escape action
               MemorySpaceType_NULL,    // memory space
               false,                    // profiled yet?
               0,         // access mask
               0                  // memfetch pointer
               )
        );

        Sharers.remove(L1Machine);
    }
}


// Ideally invalidate address in all sharers except requester
void ideal_inv_sharer_L1s_minus_requester(NodeID chip_id, Address addr, NetDest Sharers, MachineID requester) {
    Sharers.remove(requester);
    ideal_inv_sharer_L1s(chip_id, addr, Sharers);
}


// Check if protocol is ideal coherence
bool protocol_is_ideal_coherence() {
    // Check that first part of protocol name is 'ideal_coherence'
    return (strncmp(CURRENT_PROTOCOL, "ideal_coherence", 15) == 0);
}

// Check if benchmark contains memory fences
bool benchmark_contains_membar() { return BENCHMARK_CONTAINS_MEMBAR; }

// Get a list of true L1 sharers (by probing the L1s)
NetDest get_true_sharers(NodeID chip_id, Address addr) {
    NetDest true_sharers;
    assert(chip_id < RubyConfig::numberOfChips());
    for(int i=0; i<RubyConfig::numberOfProcsPerChip(); i++) {
        if(g_system_ptr->getChip(chip_id)->getL1DCache(i)->isTagPresent(addr))
            true_sharers.add(getL1MachineID(i));
    }
    return true_sharers;
}
