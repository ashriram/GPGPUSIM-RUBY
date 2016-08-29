
#ifndef GLOBALWRITEVECTOR_H
#define GLOBALWRITEVECTOR_H


#include "Global.h"
#include "Map.h"
#include "Address.h"
#include <vector>

class Chip;

class GlobalWriteVector {
public:

   GlobalWriteVector(Chip* chip_ptr, int num_threads_per_proc);

   // Class Methods
   static void printConfig(ostream& out) {}

   void set(int threadId, Time writeTime);
   Time get(int threadId);


   void print(ostream& out) const;

private:
   Chip* m_chip_ptr;
   int m_num_threads_per_proc;

   Vector<Time> m_globalWriteTimes;
};

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const GlobalWriteVector& obj)
{
  obj.print(out);
  out << flush;
  return out;
}



#endif // GLOBALWRITEVECTOR_H
