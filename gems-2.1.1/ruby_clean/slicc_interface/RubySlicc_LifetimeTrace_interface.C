#include "RubySlicc_LifetimeTrace_interface.h"
#include "System.h"

bool lifetimeTrace_enabled() {
    return LIFETIMETRACE_ENABLED;
}

void lifetimeTrace_lease(Time access_time, int address, NodeID core_id, int pc, Time lifetime) {
    g_system_ptr->getLifetimeTrace()->addLeaseAccess(access_time, address, core_id, pc, lifetime);
}

void lifetimeTrace_read(Time access_time, int address, NodeID core_id, int pc) {
    g_system_ptr->getLifetimeTrace()->addLoadAccess(access_time, address, core_id, pc);
}

void lifetimeTrace_write(Time current_time, int address, NodeID core_id) {
    g_system_ptr->getLifetimeTrace()->endLifetime(current_time, address, core_id, LifetimeEndReason_WRITE);
}

void lifetimeTrace_atomic(Time current_time, int address, NodeID core_id) {
    g_system_ptr->getLifetimeTrace()->endLifetime(current_time, address, core_id, LifetimeEndReason_ATOMIC);
}

void lifetimeTrace_l2evict(Time current_time, int address) {
    g_system_ptr->getLifetimeTrace()->endLifetime(current_time, address, 0, LifetimeEndReason_L2EVICT);
}
