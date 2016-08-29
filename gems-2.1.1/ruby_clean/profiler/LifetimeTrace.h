#ifndef LIFETIMETRACE_H
#define LIFETIMETRACE_H


#include "Global.h"
#include "GenericMachineType.h"
#include "Consumer.h"
#include <map>

class LifetimeAccess;

enum LifetimeEndReason_t {
    LifetimeEndReason_NONE = 0,
    LifetimeEndReason_L2EVICT,
    LifetimeEndReason_WRITE,
    LifetimeEndReason_ATOMIC,
    LifetimeEndReason_PROGRAMEND,
    LifetimeEndReason_EXPIRE   // for TC protocol
};

class LifetimeTrace : public Consumer {
public:
  // Constructors
    LifetimeTrace();

  // Destructor
  ~LifetimeTrace();

  void wakeup();

  void printStats(ostream& out, bool short_stats=false);
  void clearStats();
  void printConfig(ostream& out) const;
  void print(ostream& out) const;

  void addLoadAccess(Time access_time, physical_address_t address, int core_id, int pc);
  void endLifetime(Time current_time, physical_address_t address, int core_id, LifetimeEndReason_t reason);

  void addLeaseAccess(Time access_time, physical_address_t address, int core_id, int pc, Time lifetime);

private:

  void addAccessToTrace(LifetimeAccess& access);

  void programEndTrace();

  typedef std::map<int,LifetimeAccess> map_core_access_t;                             // map cores to access struct for given address
  typedef std::map<physical_address_t, map_core_access_t> map_address_cores_t;        // map addresses to list of cores' accesses

  map_address_cores_t m_map_address_cores;

  FILE * m_trace_file;      // output file for trace

};

// Output operator declaration
ostream& operator<<(ostream& out, const LifetimeTrace& obj);


struct LifetimeAccess {
    Time m_access_time;             // time at which access occurred
    Time m_lifetime;                // lifetime of access
    int m_core_id;                  // Id of L1
    int m_pc;                       // program counter
    physical_address_t  m_address;  // physical address
    LifetimeEndReason_t m_reason;   // reason that lifetime ended

    LifetimeAccess()
        : m_access_time(0), m_lifetime(0), m_core_id(-1), m_pc(-1), m_address(0), m_reason(LifetimeEndReason_NONE) { }

    LifetimeAccess(Time access_time, int core_id, int pc, physical_address_t address)
        : m_access_time(access_time), m_lifetime(0), m_core_id(core_id), m_pc(pc), m_address(address),
          m_reason(LifetimeEndReason_NONE) {}

    LifetimeAccess(Time access_time, int core_id, int pc, physical_address_t address, Time lifetime, LifetimeEndReason_t reason)
        : m_access_time(access_time), m_lifetime(lifetime), m_core_id(core_id), m_pc(pc), m_address(address),
          m_reason(reason) {}

    void setEndOfLifetime(Time lifetime, LifetimeEndReason_t reason) {
        m_lifetime = lifetime;
        m_reason = reason;
    }
};

#endif //LIFETIMETRACE_H
