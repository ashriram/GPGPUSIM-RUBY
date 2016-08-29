#ifndef MEMFETCHSET_H
#define MEMFETCHSET_H

#include "Global.h"
#include "Vector.h"
#include "NodeID.h"
#include "MachineID.h"
#include "RubyConfig.h"
#include "Set.h"
#include "MachineType.h"
#include <list>

// Keeps a set of memfetch pointers for each Core/L1
class MemfetchSet {
public:
   MemfetchSet();

   void insert(MachineID machine, uint64 memfetch);
   std::list<uint64>& get_list(int core_id);

   void print(ostream& out) const { }
private:

   typedef std::list<uint64> memfetch_list_t;
   std::vector<memfetch_list_t> m_memfetch_set;

};

// Output operator declaration
ostream& operator<<(ostream& out, const MemfetchSet& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const MemfetchSet& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //MEMFETCHSET_H
