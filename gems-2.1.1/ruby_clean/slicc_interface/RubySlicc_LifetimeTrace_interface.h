#ifndef RUBYSLICC_LIFETIMETRACE_INTERFACE_H
#define RUBYSLICC_LIFETIMETRACE_INTERFACE_H

#include "Global.h"
#include "LifetimeTrace.h"
#include "NodeID.h"

bool lifetimeTrace_enabled();

void lifetimeTrace_lease(Time access_time, int address, NodeID core_id, int pc, Time lifetime);
void lifetimeTrace_read(Time access_time, int address, NodeID core_id, int pc);
void lifetimeTrace_write(Time current_time, int address, NodeID core_id);
void lifetimeTrace_atomic(Time current_time, int address, NodeID core_id);
void lifetimeTrace_l2evict(Time current_time, int address);


#endif // RUBYSLICC_LIFETIMETRACE_INTERFACE_H
