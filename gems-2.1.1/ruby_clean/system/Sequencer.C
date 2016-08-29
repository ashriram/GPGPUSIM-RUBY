
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator,
    a component of the Multifacet GEMS (General Execution-driven
    Multiprocessor Simulator) software toolset originally developed at
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Andrew Phelps, Manoj Plakal, Daniel Sorin, Haris Volos,
    Min Xu, and Luke Yen.
    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/

/*
 * $Id: Sequencer.C 1.131 2006/11/06 17:41:01-06:00 bobba@gratiano.cs.wisc.edu $
 *
 */

#include "Global.h"
#include "Sequencer.h"
#include "System.h"
#include "Protocol.h"
#include "Profiler.h"
#include "CacheMemory.h"
#include "RubyConfig.h"
#include "Tracer.h"
#include "AbstractChip.h"
#include "Chip.h"
#include "Tester.h"
#include "SubBlock.h"
#include "Protocol.h"
#include "Map.h"
//#include "interface.h"
#include <list>

Sequencer::Sequencer(AbstractChip* chip_ptr, int version) {
  m_chip_ptr = chip_ptr;
  m_version = version;

  m_deadlock_check_scheduled = false;
  m_outstanding_count = 0;

  int smt_threads = RubyConfig::numberofSMTThreads();
  m_writeRequestTable_ptr = new Map<Address, CacheMsg>*[smt_threads];
  m_readRequestTable_ptr = new Map<Address, CacheMsg>*[smt_threads];

  m_skipL1RequestTable_ptr = new Map<uint64, CacheMsg>;

  for(int p=0; p < smt_threads; ++p){
    m_writeRequestTable_ptr[p] = new Map<Address, CacheMsg>;
    m_readRequestTable_ptr[p] = new Map<Address, CacheMsg>;
  }


}

Sequencer::~Sequencer() {
  int smt_threads = RubyConfig::numberofSMTThreads();
  for(int i=0; i < smt_threads; ++i){
    if(m_writeRequestTable_ptr[i]){
      delete m_writeRequestTable_ptr[i];
    }
    if(m_readRequestTable_ptr[i]){
      delete m_readRequestTable_ptr[i];
    }
  }
  if(m_writeRequestTable_ptr){
    delete [] m_writeRequestTable_ptr;
  }
  if(m_readRequestTable_ptr){
    delete [] m_readRequestTable_ptr;
  }
  if(m_skipL1RequestTable_ptr) {
     delete m_skipL1RequestTable_ptr;
  }
}

void Sequencer::wakeup() {
  // Check for deadlock of any of the requests
  Time current_time = g_eventQueue_ptr->getTime();
  bool deadlock = false;

  // Check across all outstanding requests
  int smt_threads = RubyConfig::numberofSMTThreads();
  int total_outstanding = 0;
  for(int p=0; p < smt_threads; ++p){
    Vector<Address> keys = m_readRequestTable_ptr[p]->keys();
    for (int i=0; i<keys.size(); i++) {
      CacheMsg& request = m_readRequestTable_ptr[p]->lookup(keys[i]);
      if (current_time - request.getTime() >= g_DEADLOCK_THRESHOLD) {
        WARN_MSG("Possible Deadlock detected");
        WARN_EXPR(request);
        WARN_EXPR(m_chip_ptr->getID());
        WARN_EXPR(m_version);
        WARN_EXPR(keys.size());
        WARN_EXPR(current_time);
        WARN_EXPR(request.getTime());
        WARN_EXPR(current_time - request.getTime());
        WARN_EXPR(*m_readRequestTable_ptr[p]);
        ERROR_MSG("Aborting");
        deadlock = true;
      }
    }

    keys = m_writeRequestTable_ptr[p]->keys();
    for (int i=0; i<keys.size(); i++) {
      CacheMsg& request = m_writeRequestTable_ptr[p]->lookup(keys[i]);
      if (current_time - request.getTime() >= g_DEADLOCK_THRESHOLD) {
        WARN_MSG("Possible Deadlock detected");
        WARN_EXPR(request);
        WARN_EXPR(m_chip_ptr->getID());
        WARN_EXPR(m_version);
        WARN_EXPR(current_time);
        WARN_EXPR(request.getTime());
        WARN_EXPR(current_time - request.getTime());
        WARN_EXPR(keys.size());
        WARN_EXPR(*m_writeRequestTable_ptr[p]);
        ERROR_MSG("Aborting");
        deadlock = true;
      }
    }
    total_outstanding += m_writeRequestTable_ptr[p]->size() + m_readRequestTable_ptr[p]->size();
  }  // across all request tables


  Vector<uint64> skipL1_keys = m_skipL1RequestTable_ptr->keys();
  for (int i=0; i<skipL1_keys.size(); i++) {
    CacheMsg& request = m_skipL1RequestTable_ptr->lookup(skipL1_keys[i]);
    if (current_time - request.getTime() >= g_DEADLOCK_THRESHOLD) {
      WARN_MSG("Possible Deadlock detected");
      WARN_EXPR(request);
      WARN_EXPR(m_chip_ptr->getID());
      WARN_EXPR(m_version);
      WARN_EXPR(skipL1_keys.size());
      WARN_EXPR(current_time);
      WARN_EXPR(request.getTime());
      WARN_EXPR(current_time - request.getTime());
      WARN_EXPR(m_skipL1RequestTable_ptr);
      ERROR_MSG("Aborting");
      deadlock = true;
    }
  }
  total_outstanding += m_skipL1RequestTable_ptr->size();
  assert(m_outstanding_count == total_outstanding);

  if (m_outstanding_count > 0) { // If there are still outstanding requests, keep checking
    g_eventQueue_ptr->scheduleEvent(this, g_DEADLOCK_THRESHOLD);
  } else {
    m_deadlock_check_scheduled = false;
  }
}

//returns the total number of requests
int Sequencer::getNumberOutstanding(){
  return m_outstanding_count;
}

// returns the total number of demand requests
int Sequencer::getNumberOutstandingDemand(){
  int smt_threads = RubyConfig::numberofSMTThreads();
  int total_demand = 0;
  for(int p=0; p < smt_threads; ++p){
    Vector<Address> keys = m_readRequestTable_ptr[p]->keys();
    for (int i=0; i< keys.size(); i++) {
      CacheMsg& request = m_readRequestTable_ptr[p]->lookup(keys[i]);
      // don't count transactional begin/commit requests
      if(request.getType() != CacheRequestType_BEGIN_XACT && request.getType() != CacheRequestType_COMMIT_XACT){
        if(request.getPrefetch() == PrefetchBit_No){
          total_demand++;
        }
      }
    }

    keys = m_writeRequestTable_ptr[p]->keys();
    for (int i=0; i< keys.size(); i++) {
      CacheMsg& request = m_writeRequestTable_ptr[p]->lookup(keys[i]);
      if(request.getPrefetch() == PrefetchBit_No){
        total_demand++;
      }
    }
  }

  return total_demand;
}

int Sequencer::getNumberOutstandingPrefetch(){
  int smt_threads = RubyConfig::numberofSMTThreads();
  int total_prefetch = 0;
  for(int p=0; p < smt_threads; ++p){
    Vector<Address> keys = m_readRequestTable_ptr[p]->keys();
    for (int i=0; i< keys.size(); i++) {
      CacheMsg& request = m_readRequestTable_ptr[p]->lookup(keys[i]);
      if(request.getPrefetch() == PrefetchBit_Yes){
        total_prefetch++;
      }
    }

    keys = m_writeRequestTable_ptr[p]->keys();
    for (int i=0; i< keys.size(); i++) {
      CacheMsg& request = m_writeRequestTable_ptr[p]->lookup(keys[i]);
      if(request.getPrefetch() == PrefetchBit_Yes){
        total_prefetch++;
      }
    }
  }

  return total_prefetch;
}


void Sequencer::printProgress(ostream& out) const{

  int total_demand = 0;
  out << "Sequencer Stats Version " << m_version << endl;
  out << "Current time = " << g_eventQueue_ptr->getTime() << endl;
  out << "---------------" << endl;
  out << "outstanding requests" << endl;

  int smt_threads = RubyConfig::numberofSMTThreads();
  for(int p=0; p < smt_threads; ++p){
    Vector<Address> rkeys = m_readRequestTable_ptr[p]->keys();
    int read_size = rkeys.size();
    out << "proc " << m_chip_ptr->getID() << " thread " << p << " Read Requests = " << read_size << endl;
    // print the request table
    for(int i=0; i < read_size; ++i){
      CacheMsg & request = m_readRequestTable_ptr[p]->lookup(rkeys[i]);
      out << "\tRequest[ " << i << " ] = " << request.getType() << " Address " << rkeys[i]  << " Posted " << request.getTime() << " PF " << request.getPrefetch() << endl;
      if( request.getPrefetch() == PrefetchBit_No ){
        total_demand++;
      }
    }

    Vector<Address> wkeys = m_writeRequestTable_ptr[p]->keys();
    int write_size = wkeys.size();
    out << "proc " << m_chip_ptr->getID() << " thread " << p << " Write Requests = " << write_size << endl;
    // print the request table
    for(int i=0; i < write_size; ++i){
      CacheMsg & request = m_writeRequestTable_ptr[p]->lookup(wkeys[i]);
      out << "\tRequest[ " << i << " ] = " << request.getType() << " Address " << wkeys[i]  << " Posted " << request.getTime() << " PF " << request.getPrefetch() << endl;
      if( request.getPrefetch() == PrefetchBit_No ){
        total_demand++;
      }
    }

    out << endl;
  }
  out << "Total Number Outstanding: " << m_outstanding_count << endl;
  out << "Total Number Demand     : " << total_demand << endl;
  out << "Total Number Prefetches : " << m_outstanding_count - total_demand << endl;
  out << endl;
  out << endl;

}

void Sequencer::printConfig(ostream& out) {
  if (TSO) {
    out << "sequencer: Sequencer - TSO" << endl;
  } else {
    out << "sequencer: Sequencer - SC" << endl;
  }
  out << "  max_outstanding_requests: " << g_SEQUENCER_OUTSTANDING_REQUESTS << endl;
}

bool Sequencer::empty() const {
  return m_outstanding_count == 0;
}

void Sequencer::checkOutstandingRequests() const {
   int total_outstanding = 0;
   int smt_threads = RubyConfig::numberofSMTThreads();
   for(int p=0; p < smt_threads; ++p){
     total_outstanding += m_writeRequestTable_ptr[p]->size() + m_readRequestTable_ptr[p]->size();
   }
   total_outstanding += m_skipL1RequestTable_ptr->size();
   assert(m_outstanding_count == total_outstanding);
}

// Insert a skip L1 request
void Sequencer::insertSkipL1Request(const CacheMsg& request) {
   // See if we should schedule a deadlock check
   if (m_deadlock_check_scheduled == false) {
     g_eventQueue_ptr->scheduleEvent(this, g_DEADLOCK_THRESHOLD);
     m_deadlock_check_scheduled = true;
   }
   checkOutstandingRequests();

   assert(!m_skipL1RequestTable_ptr->exist(request.getmemfetch()));


   m_skipL1RequestTable_ptr->allocate(request.getmemfetch());
   m_skipL1RequestTable_ptr->lookup(request.getmemfetch()) = request;
   m_outstanding_count++;

   g_system_ptr->getProfiler()->sequencerRequests(m_outstanding_count);

   checkOutstandingRequests();
}

// Insert the request on the correct request table.  Return true if
// the entry was already present.
bool Sequencer::insertRequest(const CacheMsg& request) {
  int thread = request.getThreadID();
  assert(thread >= 0);
  checkOutstandingRequests();

  // See if we should schedule a deadlock check
  if (m_deadlock_check_scheduled == false) {
    g_eventQueue_ptr->scheduleEvent(this, g_DEADLOCK_THRESHOLD);
    m_deadlock_check_scheduled = true;
  }

  if ((request.getType() == CacheRequestType_ST) ||
      (request.getType() == CacheRequestType_ST_XACT) ||
      (request.getType() == CacheRequestType_LDX_XACT) ||
      (request.getType() == CacheRequestType_ATOMIC)) {
    if (m_writeRequestTable_ptr[thread]->exist(line_address(request.getAddress()))) {
      m_writeRequestTable_ptr[thread]->lookup(line_address(request.getAddress())) = request;
      return true;
    }
    m_writeRequestTable_ptr[thread]->allocate(line_address(request.getAddress()));
    m_writeRequestTable_ptr[thread]->lookup(line_address(request.getAddress())) = request;
    m_outstanding_count++;
  } else {
    if (m_readRequestTable_ptr[thread]->exist(line_address(request.getAddress()))) {
      m_readRequestTable_ptr[thread]->lookup(line_address(request.getAddress())) = request;
      return true;
    }
    m_readRequestTable_ptr[thread]->allocate(line_address(request.getAddress()));
    m_readRequestTable_ptr[thread]->lookup(line_address(request.getAddress())) = request;
    m_outstanding_count++;
  }

  g_system_ptr->getProfiler()->sequencerRequests(m_outstanding_count);
  checkOutstandingRequests();
  return false;
}


void Sequencer::removeSkipL1Request(const CacheMsg& request) {
   checkOutstandingRequests();
   assert(m_skipL1RequestTable_ptr->exist(request.getmemfetch()));
   m_skipL1RequestTable_ptr->deallocate(request.getmemfetch());
   m_outstanding_count--;
   checkOutstandingRequests();
}


void Sequencer::removeRequest(const CacheMsg& request) {
  int thread = request.getThreadID();
  assert(thread >= 0);

  checkOutstandingRequests();
  if ((request.getType() == CacheRequestType_ST) ||
      (request.getType() == CacheRequestType_ST_XACT) ||
      (request.getType() == CacheRequestType_LDX_XACT) ||
      (request.getType() == CacheRequestType_ATOMIC)) {
    m_writeRequestTable_ptr[thread]->deallocate(line_address(request.getAddress()));
  } else {
    m_readRequestTable_ptr[thread]->deallocate(line_address(request.getAddress()));
  }
  m_outstanding_count--;
  checkOutstandingRequests();
}

void Sequencer::writeCallback(const Address& address) {
  DataBlock data;
  writeCallback(address, data);
}

void Sequencer::writeCallback(const Address& address, uint64 memfetch) {
  assert(m_skipL1RequestTable_ptr->exist(memfetch));
  DataBlock data;
  CacheMsg request = m_skipL1RequestTable_ptr->lookup(memfetch);
  removeSkipL1Request(request);
  int thread = request.getThreadID();

  hitCallback(request, data, GenericMachineType_NULL, thread);
}

void Sequencer::writeCallback(const Address& address, DataBlock& data) {
  // process oldest thread first
  int thread = -1;
  Time oldest_time = 0;
  int smt_threads = RubyConfig::numberofSMTThreads();
  for(int t=0; t < smt_threads; ++t){
    if(m_writeRequestTable_ptr[t]->exist(address)){
      CacheMsg & request = m_writeRequestTable_ptr[t]->lookup(address);
      if(thread == -1 || (request.getTime() < oldest_time) ){
        thread = t;
        oldest_time = request.getTime();
      }
    }
  }
  // make sure we found an oldest thread
  ASSERT(thread != -1);

  CacheMsg & request = m_writeRequestTable_ptr[thread]->lookup(address);

  writeCallback(address, data, GenericMachineType_NULL, PrefetchBit_No, thread);
}

void Sequencer::writeCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, PrefetchBit pf, int thread) {

  assert(address == line_address(address));
  assert(thread >= 0);
  assert(m_writeRequestTable_ptr[thread]->exist(line_address(address)));

  writeCallback(address, data, respondingMach, thread);

}

void Sequencer::writeCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, int thread) {
  assert(address == line_address(address));
  assert(m_writeRequestTable_ptr[thread]->exist(line_address(address)));
  CacheMsg request = m_writeRequestTable_ptr[thread]->lookup(address);
  assert( request.getThreadID() == thread);
  removeRequest(request);

  assert((request.getType() == CacheRequestType_ST) ||
         (request.getType() == CacheRequestType_ST_XACT) ||
         (request.getType() == CacheRequestType_LDX_XACT) ||
         (request.getType() == CacheRequestType_ATOMIC));

  hitCallback(request, data, respondingMach, thread);

}

void Sequencer::readCallback(const Address& address) {
  DataBlock data;
  readCallback(address, data);
}

void Sequencer::readCallback(const Address& address, uint64 memfetch) {
  assert(m_skipL1RequestTable_ptr->exist(memfetch));
  DataBlock data;
  CacheMsg request = m_skipL1RequestTable_ptr->lookup(memfetch);
  removeSkipL1Request(request);
  int thread = request.getThreadID();

  hitCallback(request, data, GenericMachineType_NULL, thread);
}

void Sequencer::readCallback(const Address& address, MemfetchSet mfset) {
   std::list<uint64>& mf_list = mfset.get_list((m_chip_ptr->getID() * RubyConfig::numberOfProcsPerChip()) + m_version);
   std::list<uint64>::iterator it;
   for(it=mf_list.begin(); it!=mf_list.end(); it++) {
      readCallback(address, *it);
   }
}

void Sequencer::readCallback(const Address& address, DataBlock& data) {
  // process oldest thread first
  int thread = -1;
  Time oldest_time = 0;
  int smt_threads = RubyConfig::numberofSMTThreads();
  for(int t=0; t < smt_threads; ++t){
    if(m_readRequestTable_ptr[t]->exist(address)){
      CacheMsg & request = m_readRequestTable_ptr[t]->lookup(address);
      if(thread == -1 || (request.getTime() < oldest_time) ){
        thread = t;
        oldest_time = request.getTime();
      }
    }
  }
  // make sure we found an oldest thread
  ASSERT(thread != -1);

  CacheMsg & request = m_readRequestTable_ptr[thread]->lookup(address);

  readCallback(address, data, GenericMachineType_NULL, PrefetchBit_No, thread);
}

void Sequencer::readCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, PrefetchBit pf, int thread) {

  assert(address == line_address(address));
  assert(m_readRequestTable_ptr[thread]->exist(line_address(address)));

  readCallback(address, data, respondingMach, thread);
}

void Sequencer::readCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, int thread) {
  assert(address == line_address(address));
  assert(m_readRequestTable_ptr[thread]->exist(line_address(address)));

  CacheMsg request = m_readRequestTable_ptr[thread]->lookup(address);
  assert( request.getThreadID() == thread );
  removeRequest(request);

  assert((request.getType() == CacheRequestType_LD) ||
         (request.getType() == CacheRequestType_LD_XACT) ||
         (request.getType() == CacheRequestType_IFETCH)
         );

  hitCallback(request, data, respondingMach, thread);
}

void Sequencer::hitCallback(const CacheMsg& request, DataBlock& data, GenericMachineType respondingMach, int thread) {
  int size = request.getSize();
  Address request_address = request.getAddress();
  Address request_logical_address = request.getLogicalAddress();
  Address request_line_address = line_address(request_address);
  CacheRequestType type = request.getType();
  MemorySpaceType space = request.getSpace();
  int threadID = request.getThreadID();
  uint64 memfetch = request.getmemfetch();
  Time issued_time = request.getTime();
  int logical_proc_no = ((m_chip_ptr->getID() * RubyConfig::numberOfProcsPerChip()) + m_version) * RubyConfig::numberofSMTThreads() + threadID;

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, size);

  // Set this cache entry to the most recently used
  if (type == CacheRequestType_IFETCH) {
    if (Protocol::m_TwoLevelCache) {
      if (m_chip_ptr->m_L1Cache_L1IcacheMemory_vec[m_version]->isTagPresent(request_line_address)) {
        m_chip_ptr->m_L1Cache_L1IcacheMemory_vec[m_version]->setMRU(request_line_address);
      }
    }
    else {
      if (m_chip_ptr->m_L1Cache_cacheMemory_vec[m_version]->isTagPresent(request_line_address)) {
        m_chip_ptr->m_L1Cache_cacheMemory_vec[m_version]->setMRU(request_line_address);
      }
    }
  } else {
    if (Protocol::m_TwoLevelCache) {
      if (m_chip_ptr->m_L1Cache_L1DcacheMemory_vec[m_version]->isTagPresent(request_line_address)) {
        m_chip_ptr->m_L1Cache_L1DcacheMemory_vec[m_version]->setMRU(request_line_address);
      }
    }
    else {
      if (m_chip_ptr->m_L1Cache_cacheMemory_vec[m_version]->isTagPresent(request_line_address)) {
        m_chip_ptr->m_L1Cache_cacheMemory_vec[m_version]->setMRU(request_line_address);
      }
    }
  }

  assert(g_eventQueue_ptr->getTime() >= issued_time);
  Time miss_latency = g_eventQueue_ptr->getTime() - issued_time;

  if (PROTOCOL_DEBUG_TRACE) {
    g_system_ptr->getProfiler()->profileTransition("Seq", (m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version), -1, request.getAddress(), "", "Done", "",
                                                   int_to_string(miss_latency)+" cycles "+GenericMachineType_to_string(respondingMach)+" "+CacheRequestType_to_string(request.getType())+" "+PrefetchBit_to_string(request.getPrefetch()));
  }

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, request_address);
  DEBUG_MSG(SEQUENCER_COMP, MedPrio, request.getPrefetch());
  if (request.getPrefetch() == PrefetchBit_Yes) {
    DEBUG_MSG(SEQUENCER_COMP, MedPrio, "return");
    g_system_ptr->getProfiler()->swPrefetchLatency(miss_latency, type, respondingMach);
    return; // Ignore the software prefetch, don't callback the driver
  }

  // Profile the miss latency for all non-zero demand misses
  if (miss_latency != 0) {
    g_system_ptr->getProfiler()->missLatency(miss_latency, type, respondingMach);


  }

  bool write =
    (type == CacheRequestType_ST) ||
    (type == CacheRequestType_ST_XACT) ||
    (type == CacheRequestType_LDX_XACT) ||
    (type == CacheRequestType_ATOMIC);

  if (TSO && write) {
    m_chip_ptr->m_L1Cache_storeBuffer_vec[m_version]->callBack(line_address(request.getAddress()), data);
  } else {

    // Copy the correct bytes out of the cache line into the subblock
    SubBlock subblock(request_address, request_logical_address, size);
    subblock.mergeFrom(data);  // copy the correct bytes from DataBlock in the SubBlock

    // Scan the store buffer to see if there are any outstanding stores we need to collect
    if (TSO) {
      m_chip_ptr->m_L1Cache_storeBuffer_vec[m_version]->updateSubBlock(subblock);
    }

    // Call into the Driver (Tester or Simics) and let it read and/or modify the sub-block
    g_system_ptr->getDriver()->hitCallback(m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version, subblock, type, space, threadID, memfetch);

    // If the request was a Store or Atomic, apply the changes in the SubBlock to the DataBlock
    // (This is only triggered for the non-TSO case)
    if (write) {
      assert(!TSO);
      subblock.mergeTo(data);    // copy the correct bytes from SubBlock into the DataBlock
    }
  }
}



void Sequencer::memfenceCallback(int thread) {
    g_system_ptr->getDriver()->memfenceCallback(m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version, thread);
}

void Sequencer::readConflictCallback(const Address& address) {
  // process oldest thread first
  int thread = -1;
  Time oldest_time = 0;
  int smt_threads = RubyConfig::numberofSMTThreads();
  for(int t=0; t < smt_threads; ++t){
    if(m_readRequestTable_ptr[t]->exist(address)){
      CacheMsg & request = m_readRequestTable_ptr[t]->lookup(address);
      if(thread == -1 || (request.getTime() < oldest_time) ){
        thread = t;
        oldest_time = request.getTime();
      }
    }
  }
  // make sure we found an oldest thread
  ASSERT(thread != -1);

  CacheMsg & request = m_readRequestTable_ptr[thread]->lookup(address);

  readConflictCallback(address, GenericMachineType_NULL, thread);
}

void Sequencer::readConflictCallback(const Address& address, GenericMachineType respondingMach, int thread) {
  assert(address == line_address(address));
  assert(m_readRequestTable_ptr[thread]->exist(line_address(address)));

  CacheMsg request = m_readRequestTable_ptr[thread]->lookup(address);
  assert( request.getThreadID() == thread );
  removeRequest(request);

  assert((request.getType() == CacheRequestType_LD) ||
         (request.getType() == CacheRequestType_LD_XACT) ||
         (request.getType() == CacheRequestType_IFETCH)
         );

  conflictCallback(request, respondingMach, thread);
}

void Sequencer::writeConflictCallback(const Address& address) {
  // process oldest thread first
  int thread = -1;
  Time oldest_time = 0;
  int smt_threads = RubyConfig::numberofSMTThreads();
  for(int t=0; t < smt_threads; ++t){
    if(m_writeRequestTable_ptr[t]->exist(address)){
      CacheMsg & request = m_writeRequestTable_ptr[t]->lookup(address);
      if(thread == -1 || (request.getTime() < oldest_time) ){
        thread = t;
        oldest_time = request.getTime();
      }
    }
  }
  // make sure we found an oldest thread
  ASSERT(thread != -1);

  CacheMsg & request = m_writeRequestTable_ptr[thread]->lookup(address);

  writeConflictCallback(address, GenericMachineType_NULL, thread);
}

void Sequencer::writeConflictCallback(const Address& address, GenericMachineType respondingMach, int thread) {
  assert(address == line_address(address));
  assert(m_writeRequestTable_ptr[thread]->exist(line_address(address)));
  CacheMsg request = m_writeRequestTable_ptr[thread]->lookup(address);
  assert( request.getThreadID() == thread);
  removeRequest(request);

  assert((request.getType() == CacheRequestType_ST) ||
         (request.getType() == CacheRequestType_ST_XACT) ||
         (request.getType() == CacheRequestType_LDX_XACT) ||
         (request.getType() == CacheRequestType_ATOMIC));

  conflictCallback(request, respondingMach, thread);

}

void Sequencer::conflictCallback(const CacheMsg& request, GenericMachineType respondingMach, int thread) {
  assert(XACT_MEMORY);
  int size = request.getSize();
  Address request_address = request.getAddress();
  Address request_logical_address = request.getLogicalAddress();
  Address request_line_address = line_address(request_address);
  CacheRequestType type = request.getType();
  int threadID = request.getThreadID();
  Time issued_time = request.getTime();
  int logical_proc_no = ((m_chip_ptr->getID() * RubyConfig::numberOfProcsPerChip()) + m_version) * RubyConfig::numberofSMTThreads() + threadID;

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, size);

  assert(g_eventQueue_ptr->getTime() >= issued_time);
  Time miss_latency = g_eventQueue_ptr->getTime() - issued_time;

  if (PROTOCOL_DEBUG_TRACE) {
    g_system_ptr->getProfiler()->profileTransition("Seq", (m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version), -1, request.getAddress(), "", "Conflict", "",
                                                   int_to_string(miss_latency)+" cycles "+GenericMachineType_to_string(respondingMach)+" "+CacheRequestType_to_string(request.getType())+" "+PrefetchBit_to_string(request.getPrefetch()));
  }

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, request_address);
  DEBUG_MSG(SEQUENCER_COMP, MedPrio, request.getPrefetch());
  if (request.getPrefetch() == PrefetchBit_Yes) {
    DEBUG_MSG(SEQUENCER_COMP, MedPrio, "return");
    g_system_ptr->getProfiler()->swPrefetchLatency(miss_latency, type, respondingMach);
    return; // Ignore the software prefetch, don't callback the driver
  }

  bool write =
    (type == CacheRequestType_ST) ||
    (type == CacheRequestType_ST_XACT) ||
    (type == CacheRequestType_LDX_XACT) ||
    (type == CacheRequestType_ATOMIC);

  // Copy the correct bytes out of the cache line into the subblock
  SubBlock subblock(request_address, request_logical_address, size);

  // Call into the Driver (Tester or Simics)
  g_system_ptr->getDriver()->conflictCallback(m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version, subblock, type, threadID);

  // If the request was a Store or Atomic, apply the changes in the SubBlock to the DataBlock
  // (This is only triggered for the non-TSO case)
  if (write) {
    assert(!TSO);
  }
}

void Sequencer::printDebug(){
  //notify driver of debug
  g_system_ptr->getDriver()->printDebug();
}

// Returns true if the sequencer already has a load or store outstanding
bool Sequencer::isReady(const CacheMsg& request) const {

  if (m_outstanding_count >= g_SEQUENCER_OUTSTANDING_REQUESTS) {
    //cout << "TOO MANY OUTSTANDING: " << m_outstanding_count << " " << g_SEQUENCER_OUTSTANDING_REQUESTS << " VER " << m_version << endl;
    //printProgress(cout);
    return false;
  }

  if(request.getType() == CacheRequestType_MEM_FENCE) {
     return true;    // memfences can go ahead as long as there is space in sequencer
  }

  int thread = request.getThreadID();

  // This code allows reads to be performed even when we have a write
  // request outstanding for the line
  bool write =
    (request.getType() == CacheRequestType_ST) ||
    (request.getType() == CacheRequestType_ST_XACT) ||
    (request.getType() == CacheRequestType_LDX_XACT) ||
    (request.getType() == CacheRequestType_ATOMIC);

  // LUKE - disallow more than one request type per address
  //     INVARIANT: at most one request type per address, per processor

  // Skip this check if the request is bypassing MSHRs. This is if
  // 1. Global memory accesses if gmem_skip_L1D==true
  // 2. Stores and atomics if writes_stall_at_mshr==false
  if(
        (request.getSpace() == MemorySpaceType_GLOBAL and g_GMEM_SKIP_L1D == true) or
        ( (request.getType()==CacheRequestType_ST || request.getType()==CacheRequestType_ATOMIC) and WRITES_STALL_AT_MSHR == false)
  ) {
     // skip the check
  } else {
     int smt_threads = RubyConfig::numberofSMTThreads();
     for(int p=0; p < smt_threads; ++p){
       if( m_writeRequestTable_ptr[p]->exist(line_address(request.getAddress())) ||
           m_readRequestTable_ptr[p]->exist(line_address(request.getAddress())) ){
         //cout << "OUTSTANDING REQUEST EXISTS " << p << " VER " << m_version << endl;
         //printProgress(cout);
         //return false;
         assert(0);     // GPGPU-Sim side MSHR should prevent this
       }
     }
  }

  if (TSO) {
    return m_chip_ptr->m_L1Cache_storeBuffer_vec[m_version]->isReady();
  }
  return true;
}

// Called by Driver (Simics or Tester).
void Sequencer::makeRequest(const CacheMsg& request) {
  //assert(isReady(request));
  bool write = (request.getType() == CacheRequestType_ST) ||
    (request.getType() == CacheRequestType_ST_XACT) ||
    (request.getType() == CacheRequestType_LDX_XACT) ||
    (request.getType() == CacheRequestType_ATOMIC);

  if (TSO && (request.getPrefetch() == PrefetchBit_No) && write) {
    assert(m_chip_ptr->m_L1Cache_storeBuffer_vec[m_version]->isReady());
    m_chip_ptr->m_L1Cache_storeBuffer_vec[m_version]->insertStore(request);
    return;
  }

  // Skip the normal read/write table if
  // 1. Global memory accesses if gmem_skip_L1D==true
  // 2. Stores and atomics if writes_stall_at_mshr==false
  if(
        (request.getSpace() == MemorySpaceType_GLOBAL and g_GMEM_SKIP_L1D == true) or
        ( (request.getType()==CacheRequestType_ST || request.getType()==CacheRequestType_ATOMIC) and WRITES_STALL_AT_MSHR == false)
  ) {
     issueSkipL1Request(request);
  } else {
     bool hit = doRequest(request);
  }

}

void Sequencer::makeMemfenceRequest(const CacheMsg& request) {
   assert(request.getType() == CacheRequestType_MEM_FENCE);

   CacheMsg msg = request;
   msg.getAddress() = line_address(request.getAddress()); // Make line address

   Time latency = 0;  // initialzed to an null value

   latency = SEQUENCER_TO_CONTROLLER_LATENCY;

   // Send the message to the cache controller
   assert(latency > 0);
   m_chip_ptr->m_L1Cache_mandatoryQueue_vec[m_version]->enqueue(msg, latency);

}

void Sequencer::makeIdealInvRequest(const CacheMsg& request) {
   assert(request.getType() == CacheRequestType_IDEAL_INV);

   CacheMsg msg = request;
   msg.getAddress() = line_address(request.getAddress()); // Make line address

   Time latency = 1;

   // Send the message to the cache controller
   assert(latency > 0);
   m_chip_ptr->m_L1Cache_idealInvQueue_vec[m_version]->enqueue(msg, latency);

}

bool Sequencer::doRequest(const CacheMsg& request) {
  bool hit = false;
  // Check the fast path
  DataBlock* data_ptr;

  int thread = request.getThreadID();

  hit = tryCacheAccess(line_address(request.getAddress()),
                       request.getType(),
                       request.getProgramCounter(),
                       request.getAccessMode(),
                       request.getSize(),
                       data_ptr);

  if (hit && (request.getType() == CacheRequestType_IFETCH || !REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH) ) {
    DEBUG_MSG(SEQUENCER_COMP, MedPrio, "Fast path hit");
    hitCallback(request, *data_ptr, GenericMachineType_L1Cache, thread);
    return true;
  }


  if (TSO && (request.getType() == CacheRequestType_LD || request.getType() == CacheRequestType_IFETCH)) {

    // See if we can satisfy the load entirely from the store buffer
    SubBlock subblock(line_address(request.getAddress()), request.getSize());
    if (m_chip_ptr->m_L1Cache_storeBuffer_vec[m_version]->trySubBlock(subblock)) {
      DataBlock dummy;
      hitCallback(request, dummy, GenericMachineType_NULL, thread);  // Call with an 'empty' datablock, since the data is in the store buffer
      return true;
    }
  }

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, "Fast path miss");
  issueRequest(request);
  return hit;
}

void Sequencer::issueSkipL1Request(const CacheMsg& request) {
   insertSkipL1Request(request);

   CacheMsg msg = request;
   msg.getAddress() = line_address(request.getAddress()); // Make line address

   if (PROTOCOL_DEBUG_TRACE) {
   g_system_ptr->getProfiler()->profileTransition("Seq", (m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip() + m_version), -1, msg.getAddress(),"", "Begin", "", CacheRequestType_to_string(request.getType()));
   }

   if (g_system_ptr->getTracer()->traceEnabled()) {
   g_system_ptr->getTracer()->traceRequest((m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version), msg.getAddress(), msg.getProgramCounter(),
                                           msg.getType(), g_eventQueue_ptr->getTime());
   }

   Time latency = 0;  // initialzed to an null value

   latency = SEQUENCER_TO_CONTROLLER_LATENCY;

   // Send the message to the cache controller
   assert(latency > 0);
   m_chip_ptr->m_L1Cache_mandatoryQueue_vec[m_version]->enqueue(msg, latency);
}

void Sequencer::issueRequest(const CacheMsg& request) {
  bool found = insertRequest(request);

  if (!found) {
    CacheMsg msg = request;
    msg.getAddress() = line_address(request.getAddress()); // Make line address

    if (PROTOCOL_DEBUG_TRACE) {
      g_system_ptr->getProfiler()->profileTransition("Seq", (m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip() + m_version), -1, msg.getAddress(),"", "Begin", "", CacheRequestType_to_string(request.getType()));
    }

    if (g_system_ptr->getTracer()->traceEnabled()) {
      g_system_ptr->getTracer()->traceRequest((m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version), msg.getAddress(), msg.getProgramCounter(),
                                              msg.getType(), g_eventQueue_ptr->getTime());
    }

    Time latency = 0;  // initialzed to an null value

    latency = SEQUENCER_TO_CONTROLLER_LATENCY;

    // Send the message to the cache controller
    assert(latency > 0);
    m_chip_ptr->m_L1Cache_mandatoryQueue_vec[m_version]->enqueue(msg, latency);

  }  // !found
}

bool Sequencer::tryCacheAccess(const Address& addr, CacheRequestType type,
                               const Address& pc, AccessModeType access_mode,
                               int size, DataBlock*& data_ptr) {
  if (type == CacheRequestType_IFETCH) {
    if (Protocol::m_TwoLevelCache) {
      return m_chip_ptr->m_L1Cache_L1IcacheMemory_vec[m_version]->tryCacheAccess(line_address(addr), type, data_ptr);
    }
    else {
      return m_chip_ptr->m_L1Cache_cacheMemory_vec[m_version]->tryCacheAccess(line_address(addr), type, data_ptr);
    }
  } else {
    if (Protocol::m_TwoLevelCache) {
      return m_chip_ptr->m_L1Cache_L1DcacheMemory_vec[m_version]->tryCacheAccess(line_address(addr), type, data_ptr);
    }
    else {
      return m_chip_ptr->m_L1Cache_cacheMemory_vec[m_version]->tryCacheAccess(line_address(addr), type, data_ptr);
    }
  }
}

void Sequencer::resetRequestTime(const Address& addr, int thread){
  assert(thread >= 0);
  //reset both load and store requests, if they exist
  if(m_readRequestTable_ptr[thread]->exist(line_address(addr))){
    CacheMsg& request = m_readRequestTable_ptr[thread]->lookup(addr);
    if( request.m_AccessMode != AccessModeType_UserMode){
      cout << "resetRequestType ERROR read request addr = " << addr << " thread = "<< thread << " is SUPERVISOR MODE" << endl;
      printProgress(cout);
    }
    //ASSERT(request.m_AccessMode == AccessModeType_UserMode);
    request.setTime(g_eventQueue_ptr->getTime());
  }
  if(m_writeRequestTable_ptr[thread]->exist(line_address(addr))){
    CacheMsg& request = m_writeRequestTable_ptr[thread]->lookup(addr);
    if( request.m_AccessMode != AccessModeType_UserMode){
      cout << "resetRequestType ERROR write request addr = " << addr << " thread = "<< thread << " is SUPERVISOR MODE" << endl;
      printProgress(cout);
    }
    //ASSERT(request.m_AccessMode == AccessModeType_UserMode);
    request.setTime(g_eventQueue_ptr->getTime());
  }
}

void Sequencer::print(ostream& out) const {
  out << "[Sequencer: " << m_chip_ptr->getID()
      << ", outstanding requests: " << m_outstanding_count;

  int smt_threads = RubyConfig::numberofSMTThreads();
  for(int p=0; p < smt_threads; ++p){
    out << ", read request table[ " << p << " ]: " << *m_readRequestTable_ptr[p]
        << ", write request table[ " << p << " ]: " << *m_writeRequestTable_ptr[p];
  }
  out << "]";
}

// this can be called from setState whenever coherence permissions are upgraded
// when invoked, coherence violations will be checked for the given block
void Sequencer::checkCoherence(const Address& addr) {
#ifdef CHECK_COHERENCE
  g_system_ptr->checkGlobalCoherenceInvariant(addr);
#endif
}

bool Sequencer::getRubyMemoryValue(const Address& addr, char* value,unsigned int size_in_bytes ) {

  if(g_SIMICS){
 #if 0
      for(unsigned int i=0; i < size_in_bytes; i++) {
        /*value[i] = SIMICS_read_physical_memory( m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version,
          addr.getAddress() + i, 1 );*/
        value[i]=0;

    }
#endif
      return false; // Do nothing?
  } else {
    bool found = false;
    const Address lineAddr = line_address(addr);
    DataBlock data;
    PhysAddress paddr(addr);
    DataBlock* dataPtr = &data;
    Chip* n = dynamic_cast<Chip*>(m_chip_ptr);
    // LUKE - use variable names instead of macros
    assert(n->m_L1Cache_L1IcacheMemory_vec[m_version] != NULL);
    assert(n->m_L1Cache_L1DcacheMemory_vec[m_version] != NULL);

    MachineID l2_mach = map_L2ChipId_to_L2Cache(addr, m_chip_ptr->getID() );
    int l2_ver = l2_mach.num%RubyConfig::numberOfL2CachePerChip();

    if (Protocol::m_TwoLevelCache) {
      if(Protocol::m_CMP){
        assert(n->m_L2Cache_L2cacheMemory_vec[l2_ver] != NULL);
      }
      else{
        assert(n->m_L1Cache_cacheMemory_vec[m_version] != NULL);
      }
    }

    if (n->m_L1Cache_L1IcacheMemory_vec[m_version]->tryCacheAccess(lineAddr, CacheRequestType_IFETCH, dataPtr)){
      n->m_L1Cache_L1IcacheMemory_vec[m_version]->getMemoryValue(addr, value, size_in_bytes);
      found = true;
    } else if (n->m_L1Cache_L1DcacheMemory_vec[m_version]->tryCacheAccess(lineAddr, CacheRequestType_LD, dataPtr)){
      n->m_L1Cache_L1DcacheMemory_vec[m_version]->getMemoryValue(addr, value, size_in_bytes);
      found = true;
    } else if (Protocol::m_CMP && n->m_L2Cache_L2cacheMemory_vec[l2_ver]->tryCacheAccess(lineAddr, CacheRequestType_LD, dataPtr)){
      n->m_L2Cache_L2cacheMemory_vec[l2_ver]->getMemoryValue(addr, value, size_in_bytes);
      found = true;
    // } else if (n->TBE_TABLE_MEMBER_VARIABLE->isPresent(lineAddr)){
//       ASSERT(n->TBE_TABLE_MEMBER_VARIABLE->isPresent(lineAddr));
//       L1Cache_TBE tbeEntry = n->TBE_TABLE_MEMBER_VARIABLE->lookup(lineAddr);

//       int offset = addr.getOffset();
//       for(int i=0; i<size_in_bytes; ++i){
//         value[i] = tbeEntry.getDataBlk().getByte(offset + i);
//       }

//       found = true;
    } else {
      // Address not found
      //cout << "  " << m_chip_ptr->getID() << " NOT IN CACHE, Value at Directory is: " << (int) value[0] << endl;
      n = dynamic_cast<Chip*>(g_system_ptr->getChip(map_Address_to_DirectoryNode(addr)/RubyConfig::numberOfDirectoryPerChip()));
      int dir_version = map_Address_to_DirectoryNode(addr)%RubyConfig::numberOfDirectoryPerChip();
      for(unsigned int i=0; i<size_in_bytes; ++i){
        int offset = addr.getOffset();
        value[i] = n->m_Directory_directory_vec[dir_version]->lookup(lineAddr).m_DataBlk.getByte(offset + i);
      }
      // Address not found
      //WARN_MSG("Couldn't find address");
      //WARN_EXPR(addr);
      found = false;
    }
    return true;
  }
}

bool Sequencer::setRubyMemoryValue(const Address& addr, char *value,
                                   unsigned int size_in_bytes) {
  char test_buffer[64];

  if(g_SIMICS){
    return false; // Do nothing?
  } else {
    // idea here is that coherent cache should find the
    // latest data, the update it
    bool found = false;
    const Address lineAddr = line_address(addr);
    PhysAddress paddr(addr);
    DataBlock data;
    DataBlock* dataPtr = &data;
    Chip* n = dynamic_cast<Chip*>(m_chip_ptr);

    MachineID l2_mach = map_L2ChipId_to_L2Cache(addr, m_chip_ptr->getID() );
    int l2_ver = l2_mach.num%RubyConfig::numberOfL2CachePerChip();
    // LUKE - use variable names instead of macros
    //cout << "number of L2caches per chip = " << RubyConfig::numberOfL2CachePerChip(m_version) << endl;
    //cout << "L1I cache vec size = " << n->m_L1Cache_L1IcacheMemory_vec.size() << endl;
    //cout << "L1D cache vec size = " << n->m_L1Cache_L1DcacheMemory_vec.size() << endl;
    //cout << "L1cache_cachememory size = " << n->m_L1Cache_cacheMemory_vec.size() << endl;
    //cout << "L1cache_l2cachememory size = " << n->m_L1Cache_L2cacheMemory_vec.size() << endl;
    // if (Protocol::m_TwoLevelCache) {
//       if(Protocol::m_CMP){
//         cout << "CMP L2 cache vec size = " << n->m_L2Cache_L2cacheMemory_vec.size() << endl;
//       }
//       else{
//        cout << "L2 cache vec size = " << n->m_L1Cache_cacheMemory_vec.size() << endl;
//       }
//     }

    assert(n->m_L1Cache_L1IcacheMemory_vec[m_version] != NULL);
    assert(n->m_L1Cache_L1DcacheMemory_vec[m_version] != NULL);
    if (Protocol::m_TwoLevelCache) {
      if(Protocol::m_CMP){
        assert(n->m_L2Cache_L2cacheMemory_vec[l2_ver] != NULL);
      }
      else{
        assert(n->m_L1Cache_cacheMemory_vec[m_version] != NULL);
      }
    }

    if (n->m_L1Cache_L1IcacheMemory_vec[m_version]->tryCacheAccess(lineAddr, CacheRequestType_IFETCH, dataPtr)){
      n->m_L1Cache_L1IcacheMemory_vec[m_version]->setMemoryValue(addr, value, size_in_bytes);
      found = true;
    } else if (n->m_L1Cache_L1DcacheMemory_vec[m_version]->tryCacheAccess(lineAddr, CacheRequestType_LD, dataPtr)){
      n->m_L1Cache_L1DcacheMemory_vec[m_version]->setMemoryValue(addr, value, size_in_bytes);
      found = true;
    } else if (Protocol::m_CMP && n->m_L2Cache_L2cacheMemory_vec[l2_ver]->tryCacheAccess(lineAddr, CacheRequestType_LD, dataPtr)){
      n->m_L2Cache_L2cacheMemory_vec[l2_ver]->setMemoryValue(addr, value, size_in_bytes);
      found = true;
    // } else if (n->TBE_TABLE_MEMBER_VARIABLE->isTagPresent(lineAddr)){
//       L1Cache_TBE& tbeEntry = n->TBE_TABLE_MEMBER_VARIABLE->lookup(lineAddr);
//       DataBlock tmpData;
//       int offset = addr.getOffset();
//       for(int i=0; i<size_in_bytes; ++i){
//         tmpData.setByte(offset + i, value[i]);
//       }
//       tbeEntry.setDataBlk(tmpData);
//       tbeEntry.setDirty(true);
    } else {
      // Address not found
      n = dynamic_cast<Chip*>(g_system_ptr->getChip(map_Address_to_DirectoryNode(addr)/RubyConfig::numberOfDirectoryPerChip()));
      int dir_version = map_Address_to_DirectoryNode(addr)%RubyConfig::numberOfDirectoryPerChip();
      for(unsigned int i=0; i<size_in_bytes; ++i){
        int offset = addr.getOffset();
        n->m_Directory_directory_vec[dir_version]->lookup(lineAddr).m_DataBlk.setByte(offset + i, value[i]);
      }
      found = false;
    }

    if (found){
      found = getRubyMemoryValue(addr, test_buffer, size_in_bytes);
      assert(found);
      if(value[0] != test_buffer[0]){
        WARN_EXPR((int) value[0]);
        WARN_EXPR((int) test_buffer[0]);
        ERROR_MSG("setRubyMemoryValue failed to set value.");
      }
    }

    return true;
  }
}
