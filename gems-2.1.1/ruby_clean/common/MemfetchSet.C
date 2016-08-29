#include "MemfetchSet.h"
#include "RubyConfig.h"
#include "Protocol.h"

MemfetchSet::MemfetchSet() : m_memfetch_set(g_NUM_PROCESSORS)
{
}

void MemfetchSet::insert(MachineID machine, uint64 memfetch) {
   assert(machine.num < g_NUM_PROCESSORS);
   m_memfetch_set[machine.num].push_back(memfetch);
}

std::list<uint64>& MemfetchSet::get_list(int core_id) {
   assert(core_id < g_NUM_PROCESSORS);
   assert(!m_memfetch_set[core_id].empty());
   return m_memfetch_set[core_id];
}
