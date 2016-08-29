#include "GlobalWriteVector.h"

GlobalWriteVector::GlobalWriteVector(Chip* chip_ptr, int num_threads_per_proc)
   : m_globalWriteTimes(num_threads_per_proc)
{
   assert(chip_ptr != NULL);
   m_chip_ptr = chip_ptr;
   m_num_threads_per_proc = num_threads_per_proc;

   for(int t=0; t<m_num_threads_per_proc; t++) {
      m_globalWriteTimes[t] = 0;
   }
}

void GlobalWriteVector::set(int threadId, Time writeTime) {
   assert(threadId < m_num_threads_per_proc);
   m_globalWriteTimes[threadId] = writeTime;
}

Time GlobalWriteVector::get(int threadId) {
   assert(threadId < m_num_threads_per_proc);
   return m_globalWriteTimes[threadId];
}

void GlobalWriteVector::print(ostream& out) const {

}
