#include "LifetimeTrace.h"
#include "System.h"
#include "Debug.h"

/*******************************************************************
 * LifetimeTrace class
 *******************************************************************/
LifetimeTrace::LifetimeTrace() {
    if(LIFETIMETRACE_ENABLED) {
        m_trace_file = fopen(LIFETIMETRACE_OUTFILE, "w");
        if(m_trace_file == NULL) {
            ERROR_MSG("Unable to open output file for LifetimeTrace");
        }
    }
}

LifetimeTrace::~LifetimeTrace()
{
    if(LIFETIMETRACE_ENABLED) {
        programEndTrace();
        fclose(m_trace_file);
    }
}

void LifetimeTrace::print(ostream& out) const
{
    out << "[LifetimeTrace]";
}

void LifetimeTrace::printStats(ostream& out, bool short_stats)
{
}

void LifetimeTrace::clearStats()
{
}

void LifetimeTrace::printConfig(ostream& out) const
{
    out << "LifetimeTrace Configuration" << endl;
    out << "----------------------" << endl;
    out << "----------------------" << endl;
}

void LifetimeTrace::wakeup()
{
}


void LifetimeTrace::addLeaseAccess(Time access_time, physical_address_t address, int core_id, int pc, Time lifetime) {
    LifetimeAccess access(access_time, core_id, pc, address, lifetime, LifetimeEndReason_EXPIRE);
    addAccessToTrace(access);
}

void LifetimeTrace::addLoadAccess(Time access_time, physical_address_t address, int core_id, int pc) {
    assert(core_id>=0 and core_id<RubyConfig::numberOfL1CachePerChip());

    // Check if a pending access already exists
    if(m_map_address_cores.count(address) > 0) {
        if(m_map_address_cores[address].count(core_id) > 0) {
            // pending access to this address from this core, ignore this access
            return;
        }
    }

    // Track this access
    m_map_address_cores[address][core_id] = LifetimeAccess(access_time, core_id, pc, address);
}

// End lifetimes
// If reason is L2EVICT or ATOMIC, then end lifetimes of all cores for this address
// If reason is WRITE, then end lifetimes of all cores except the given core_id
void LifetimeTrace::endLifetime(Time current_time, physical_address_t address, int core_id, LifetimeEndReason_t reason) {
    assert(core_id>=0 and core_id<RubyConfig::numberOfL1CachePerChip());
    assert(reason==LifetimeEndReason_L2EVICT or reason==LifetimeEndReason_WRITE or reason==LifetimeEndReason_ATOMIC);

    // Check if any accesses to this address are being tracked
    if(m_map_address_cores.count(address) == 0) {
        // No accesses, ignore this
        return;
    }
    assert(m_map_address_cores[address].size() > 0);    // there must be some accesses being tracked for this address

    // End existing lifetimes and add them to trace
    map_core_access_t& accesses_to_address = m_map_address_cores[address];
    for(map_core_access_t::iterator it=accesses_to_address.begin();
        it!=accesses_to_address.end();
        it++)
    {
        LifetimeAccess& access = (*it).second;
        assert(access.m_address == address);
        assert(access.m_access_time < current_time);

        // If reason is WRITE, then skip ending lifetime for writing core
        if(reason==LifetimeEndReason_WRITE and access.m_core_id==core_id)
            continue;

        access.setEndOfLifetime(current_time-access.m_access_time, reason);
        addAccessToTrace(access);
    }

    // Remove existing lifetimes for this address based on reason policy
    if(reason==LifetimeEndReason_L2EVICT or reason==LifetimeEndReason_ATOMIC) {
        // Remove all lifetimes
        m_map_address_cores.erase(address);
    } else if(reason==LifetimeEndReason_WRITE) {
        // Remove all accesses except for the writing core
        if(accesses_to_address.count(core_id) > 0){
            // Access by writing core exists, save it and add it back
            LifetimeAccess writingCoresAccess = accesses_to_address[core_id];
            m_map_address_cores.erase(address);
            m_map_address_cores[address][core_id] = writingCoresAccess;
        } else {
            // Access by writing core doesn't exist, remove all
            m_map_address_cores.erase(address);
        }
    }
}

// End lifetimes of all remaining accesses and add them to trace
void LifetimeTrace::programEndTrace() {
    Time current_time = g_eventQueue_ptr->getTime();
    for(map_address_cores_t::iterator it=m_map_address_cores.begin();
        it!=m_map_address_cores.end();
        it++)
    {
        map_core_access_t& accesses = (*it).second;
        physical_address_t address = (*it).first;
        assert(accesses.size() > 0);
        for(map_core_access_t::iterator it2=accesses.begin();
            it2!=accesses.end();
            it2++)
        {
            LifetimeAccess& access = (*it2).second;
            assert(access.m_address==address);
            assert(access.m_access_time<current_time);
            access.setEndOfLifetime(current_time-access.m_access_time, LifetimeEndReason_PROGRAMEND);
            addAccessToTrace(access);
        }
    }
    // Remove all tracked accesses
    m_map_address_cores.clear();
}

void LifetimeTrace::addAccessToTrace(LifetimeAccess& access) {
    assert(LIFETIMETRACE_ENABLED);
    assert(access.m_reason != LifetimeEndReason_NONE);

    fprintf(m_trace_file, "%ld, %u, %d, %d, %ld, ",
            (long) access.m_access_time, (unsigned) access.m_address, access.m_core_id, access.m_pc, (long) access.m_lifetime);
    switch(access.m_reason) {
        case LifetimeEndReason_L2EVICT: fprintf(m_trace_file, "L2EVICT"); break;
        case LifetimeEndReason_WRITE: fprintf(m_trace_file, "WRITE"); break;
        case LifetimeEndReason_ATOMIC: fprintf(m_trace_file, "ATOMIC"); break;
        case LifetimeEndReason_PROGRAMEND: fprintf(m_trace_file, "PROGRAMEND"); break;
        case LifetimeEndReason_EXPIRE: fprintf(m_trace_file, "EXPIRE"); break;
        default: ERROR_MSG("Unrecognized LifetimeEndReason"); break;
    }
    fprintf(m_trace_file, "\n");
    fflush(m_trace_file);
}
