
#ifndef GPUSIM_INTERFACE_H
#define GPUSIM_INTERFACE_H

#include "RubyConfig.h"
#include "Global.h"
#include "Driver.h"
#include "mf_api.h"
#include "CacheRequestType.h"
#include "MemorySpaceType.h"
#include "Message.h"

#include <vector>
#include <list>

class System;
class Chip;
class Sequencer;

class GpusimInterface : public Driver {
public:
  // Constructors
	GpusimInterface(System* sys_ptr);

  // Destructor
  // ~OpalInterface();

  void hitCallback( NodeID proc, SubBlock& data, CacheRequestType type, MemorySpaceType space, int thread, uint64 memfetch );
  void memfenceCallback( NodeID proc, int thread );
  void printStats(ostream& out) const;
  void clearStats();
  void printConfig(ostream& out) const;

  integer_t readPhysicalMemory(int procID, physical_address_t address,
                               int len );

  void writePhysicalMemory( int procID, physical_address_t address,
                            integer_t value, int len );


  void printDebug();

  /// The static GpusimInterface instance
  static GpusimInterface *inst;

  /// static methods

  /* returns true if the sequencer is able to handle more requests.
     This implements "back-pressure" by which the processor knows
     not to issue more requests if the network or cache's limits are reached.
   */
  static int isReady( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, 
                      CacheRequestType type, MemorySpaceType space);

  /*
  makeRequest performs the coherence transactions necessary to get the
  physical address in the cache with the correct permissions. More than
  one request can be outstanding to ruby, but only one per block address.
  The size of the cache line is defined to Intf_CacheLineSize.
  When a request is finished (e.g. the cache contains physical address),
  ruby calls completedRequest(). No request can be bigger than
  Opal_CacheLineSize. It is illegal to request non-aligned memory
  locations. A request of size 2 must be at an even byte, a size 4 must
  be at a byte address half-word aligned, etc. Requests also can't cross a
  cache-line boundaries.
  */
  static void makeRequest( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                           CacheRequestType req_type, bool isPriv, MemorySpaceType space, ruby_mem_access_byte_mask_t access_mask,
                           void * mf);

  static void makeMemfenceRequest( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                           CacheRequestType req_type, bool isPriv, MemorySpaceType space, ruby_mem_access_byte_mask_t access_mask);

  /* notify ruby of opal's status
   */
  static void notify( int status );

  /*
   * advance ruby one cycle
   */
  static void advanceTime( void );

  /*
   * return ruby's cycle count.
   */
  static unsigned long long getTime( void );

  /* prints Ruby's outstanding request table */
  static void printProgress(int cpuNumber);

  // interaction with the callback queue - empty(), top(), pop()
  bool callbackQueueEmpty(unsigned core_id) {
    assert(core_id < m_callback_queue.size());
    return m_callback_queue[core_id].empty(); 
  }
  unsigned long long callbackQueueTop(unsigned core_id) { 
    assert(core_id < m_callback_queue.size());
    return m_callback_queue[core_id].front(); 
  }
  void callbackQueuePop(unsigned core_id) { 
    assert(core_id < m_callback_queue.size());
    return m_callback_queue[core_id].pop_front(); 
  }
  void callbackQueuePrint(unsigned core_id) const; 

  // interaction with the memfence callback queue - empty(), top(), pop()
  bool memfenceCallbackQueueEmpty(unsigned core_id) {
    assert(core_id < m_memfence_callback_queue.size());
    return m_memfence_callback_queue[core_id].empty();
  }
  unsigned long long memfenceCallbackQueueTop(unsigned core_id) {
    assert(core_id < m_memfence_callback_queue.size());
    return m_memfence_callback_queue[core_id].front();
  }
  void memfenceCallbackQueuePop(unsigned core_id) {
    assert(core_id < m_memfence_callback_queue.size());
    return m_memfence_callback_queue[core_id].pop_front();
  }

  // interaction with the skipL1 callback queue - empty(), top(), pop()
  bool skipL1CallbackQueueEmpty(unsigned core_id) {
    assert(core_id < m_skipL1_callback_queue.size());
    return m_skipL1_callback_queue[core_id].empty();
  }
  void skipL1CallbackQueueTop(unsigned core_id, unsigned long long& addr, void** memfetch) {
    assert(core_id < m_skipL1_callback_queue.size());
    addr = m_skipL1_callback_queue[core_id].front().first;
    *memfetch = m_skipL1_callback_queue[core_id].front().second;
  }
  void skipL1CallbackQueuePop(unsigned core_id) {
    assert(core_id < m_skipL1_callback_queue.size());
    return m_skipL1_callback_queue[core_id].pop_front();
  }


  // interaction with GPGPU-Sim's DRAM
  bool ToGpusimDram_empty(unsigned partition_id);
  MsgPtr* ToGpusimDram_top(unsigned partition_id);
  void ToGpusimDram_pop(unsigned partition_id);

  void FromGpusimDram_push(unsigned partition_id, MsgPtr* msg_ptr);

  // Flush all L1 caches in Ruby
  void flushAllL1DCaches(ostream& out);

private:
  // Private Methods


  // Data Members (m_ prefix)

  // queue storing the address assocated with each callback 
  typedef std::list<unsigned long long> callback_queue; 
  std::vector<callback_queue> m_callback_queue; 

  // Queue for storing mem fence callbacks. One queue per core, each queue stores a list of warp ids
  typedef std::list<unsigned> memfence_callback_queue;
  std::vector<memfence_callback_queue> m_memfence_callback_queue;

  // Queue for storing L1-skipped or MSHR-skipped requests. One queue per core, each queue stores a (addr, memfetch) pair
  typedef std::pair<unsigned long long, void*> skipL1_request_t;
  typedef std::list<skipL1_request_t> skipL1_callback_queue;
  std::vector<skipL1_callback_queue> m_skipL1_callback_queue;

};

#endif // GPUSIM_INTERFACE_H

