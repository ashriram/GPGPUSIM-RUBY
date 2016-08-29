#include "GpusimInterface.h"

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "System.h"
#include "SubBlock.h"
#include "Chip.h"
#include "RubyConfig.h"
#include "Sequencer.h"
#include "GpusimDramInterface.h"

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static CacheRequestType get_request_type( OpalMemop_t opaltype );
static OpalMemop_t get_opal_request_type( CacheRequestType type );

/// The static opalinterface instance
GpusimInterface *GpusimInterface::inst = NULL;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
GpusimInterface::GpusimInterface(System* sys_ptr)
   : m_callback_queue(g_NUM_PROCESSORS), m_memfence_callback_queue(g_NUM_PROCESSORS),
     m_skipL1_callback_queue(g_NUM_PROCESSORS)
{
  clearStats();
  ASSERT( inst == NULL );
  inst = this;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void GpusimInterface::printConfig(ostream& out) const {
  out << "Gpusim_ruby_multiplier: " << OPAL_RUBY_MULTIPLIER << endl;
  out << endl;
}

void GpusimInterface::printStats(ostream& out) const {
  out << endl;
  out << "Gpusim Interface Stats" << endl;
  out << "----------------------" << endl;
  out << endl;
}

void GpusimInterface::callbackQueuePrint(unsigned core_id) const 
{
   assert(core_id < m_callback_queue.size()); 
   printf("CallbackQueue[%u]:", core_id); 
   for (callback_queue::const_iterator iCallback = m_callback_queue[core_id].begin(); 
        iCallback != m_callback_queue[core_id].end(); ++iCallback) 
   {
      printf("%llx ", *iCallback); 
   }
   printf("\n");
}

//**************************************************************************
void GpusimInterface::clearStats() {
}

static OpalMemop_t get_opal_request_type( CacheRequestType type ) {
  OpalMemop_t opal_type;

  if(type == CacheRequestType_LD){
    opal_type = OPAL_LOAD;
  }
  else if( type == CacheRequestType_ST){
    opal_type = OPAL_STORE;
  }
  else if( type == CacheRequestType_IFETCH){
    opal_type = OPAL_IFETCH;
  }
  else if( type == CacheRequestType_ATOMIC){
    opal_type = OPAL_ATOMIC;
  }
  else{
    ERROR_MSG("Error: Strange memory transaction type: not a LD or a ST");
  }

  //cout << "get_opal_request_type() CacheRequestType[ " << type << " ] opal_type[ " << opal_type << " ] " << endl;
  return opal_type;
}

static CacheRequestType get_request_type( OpalMemop_t opaltype ) {
  CacheRequestType type;

  if (opaltype == OPAL_LOAD) {
    type = CacheRequestType_LD;
  } else if (opaltype == OPAL_STORE){
    type = CacheRequestType_ST;
  } else if (opaltype == OPAL_IFETCH){
    type = CacheRequestType_IFETCH;
  } else if (opaltype == OPAL_ATOMIC){
    type = CacheRequestType_ATOMIC;
  } else {
    ERROR_MSG("Error: Strange memory transaction type: not a LD or a ST");
  }
  return type;
}



//**************************************************************************

void GpusimInterface::advanceTime( void ) {
  g_eventQueue_ptr->triggerEvents(g_eventQueue_ptr->getTime()+1);
}

// return ruby's time
//**************************************************************************
unsigned long long GpusimInterface::getTime( void ) {
  return (g_eventQueue_ptr->getTime());
}

// print's Ruby outstanding request table
void GpusimInterface::printProgress(int cpuNumber){
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  assert(targetSequencer_ptr != NULL);

  targetSequencer_ptr->printProgress(cout);
}


void  GpusimInterface::notify( int status ) {
  if (GpusimInterface::inst == NULL) {
    if (status == 1) {
      // notify system that opal is now loaded
      g_system_ptr->opalLoadNotify();
    } else {
      return;
    }
  }

  // opal interface must be allocated now
  ASSERT( GpusimInterface::inst != NULL );
  if ( status == 0 ) {

  } else if ( status == 1 ) {
    // install notification: query opal for its interface
    //GpusimInterface::inst->queryGpusimInterface();
    /*
    if ( GpusimInterface::inst->m_opal_intf != NULL ) {
      cout << "GpusimInterface: installation successful." << endl;
      // test: (*(GpusimInterface::inst->m_opal_intf->hitCallback))( 0, 0xFFULL );
    }
    */
  } else if ( status == 2 ) {
    // unload notification
    // NOTE: this is not tested, as we can't unload ruby or opal right now.
    //GpusimInterface::inst->removeGpusimInterface();
  }
}

#define WATCH_ADDR 0x80000400

void  GpusimInterface::makeRequest( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                                    CacheRequestType req_type, bool isPriv, MemorySpaceType space,
                                    ruby_mem_access_byte_mask_t access_mask, void* mf )
{
  unsigned cpuNumber = sid;
  unsigned long long physicalAddr = addr;
  unsigned long long logicalAddr = addr;
  unsigned long long virtualPC = pc;
  unsigned thread = tid;
  unsigned requestSize = req_size;

  AccessModeType access_mode;  
  if (isPriv) {
    access_mode = AccessModeType_SupervisorMode;
  } else {
    access_mode = AccessModeType_UserMode;
  }

  // Send request to sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  assert(addr != 0); // Address 0 is reserved for memfences

  targetSequencer_ptr->makeRequest(CacheMsg(Address( physicalAddr ),
                                            Address( physicalAddr ),
                                            req_type,
                                            Address(virtualPC),
                                            access_mode,   // User/supervisor mode
                                            requestSize,   // Size in bytes of request
                                            PrefetchBit_No, // Not a prefetch
                                            0,              // Version number
                                            Address(logicalAddr),   // Virtual Address
                                            thread,              // SMT thread
                                            0,          // TM specific - timestamp of memory request
                                            false,      // TM specific - whether request is part of escape action
                                            space,    // memory space
                                            false,                    // profiled yet?
                                            access_mask,         // access mask,
                                            (uint64) mf                  // memfetch pointer
                                            )
                                   );
}

void  GpusimInterface::makeMemfenceRequest( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                                            CacheRequestType req_type, bool isPriv, MemorySpaceType space,
                                            ruby_mem_access_byte_mask_t access_mask )
{
  unsigned cpuNumber = sid;
  unsigned long long physicalAddr = addr;
  unsigned long long logicalAddr = addr;
  unsigned long long virtualPC = pc;
  unsigned thread = tid;
  unsigned requestSize = req_size;

  AccessModeType access_mode;
  if (isPriv) {
    access_mode = AccessModeType_SupervisorMode;
  } else {
    access_mode = AccessModeType_UserMode;
  }

  // Send request to sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  assert(req_type == CacheRequestType_MEM_FENCE);
  assert(addr == 0); // memfences must be address 0

  targetSequencer_ptr->makeMemfenceRequest(CacheMsg(Address( physicalAddr ),
                                               Address( physicalAddr ),
                                               req_type,
                                               Address(virtualPC),
                                               access_mode,   // User/supervisor mode
                                               requestSize,   // Size in bytes of request
                                               PrefetchBit_No, // Not a prefetch
                                               0,              // Version number
                                               Address(logicalAddr),   // Virtual Address
                                               thread,              // SMT thread
                                               0,          // TM specific - timestamp of memory request
                                               false,      // TM specific - whether request is part of escape action
                                               space,    // memory space
                                               false,                    // profiled yet?
                                               access_mask,         // access mask
                                               0                  // memfetch pointer
                                               )
                                      );
}


//**************************************************************************
int  GpusimInterface::isReady( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, CacheRequestType req_type, MemorySpaceType space)
{
  unsigned cpuNumber = sid;
  unsigned long long physicalAddr = addr;
  unsigned long long logicalAddr = addr;
  unsigned long long virtualPC = 0;
  unsigned thread = tid;

  unsigned requestSize = req_size;

  // Send request to sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  // FIXME - some of these fields have bogus values sinced isReady()
  // doesn't need them.  However, it would be cleaner if all of these
  // fields were exact.

  // if (cpuNumber == 0) 
  //   printf("Ruby::isReady(proc=%d,addr=%llx,tid=%d)\n", cpuNumber, addr, thread); 
  return (targetSequencer_ptr->isReady(CacheMsg(Address( physicalAddr ),
                                                Address( physicalAddr ),
                                                req_type,
                                                Address(0),
                                                AccessModeType_UserMode,   // User/supervisor mode
                                                requestSize,   // Size in bytes of request
                                                PrefetchBit_No,  // Not a prefetch
                                                0,              // Version number
                                                Address(logicalAddr),   // Virtual Address
                                                thread,              // SMT thread
                                                0,          // TM specific - timestamp of memory request
                                                false,      // TM specific - whether request is part of escape action
                                                space,    // memory space
                                                false,                    // profiled yet?
                                                0,         // access mask
                                                0                  // memfetch pointer
                                                )
                                       ));
}


// Memory request call back to GPGPU-Sim
void GpusimInterface::hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, MemorySpaceType space, int thread, uint64 memfetch) {
  unsigned long long addr = data.getAddress().getAddress();
  // Use skipL1 callback queue for requests that skipped MSHRs
  // 1. Global memory accesses if gmem_skip_L1D==true
  // 2. Stores and atomics if writes_stall_at_mshr==false
  if(
        (space == MemorySpaceType_GLOBAL and g_GMEM_SKIP_L1D == true) or
        ( (type==CacheRequestType_ST || type==CacheRequestType_ATOMIC) and WRITES_STALL_AT_MSHR == false)
  ) {
     m_skipL1_callback_queue[proc].push_back(skipL1_request_t(addr,(void*)memfetch));
  } else {
     m_callback_queue[proc].push_back(addr);
  }
}

// Memory fence call back to GPGPU-Sim
void GpusimInterface::memfenceCallback(NodeID proc, int thread) {
  assert((unsigned)proc < m_memfence_callback_queue.size());
  m_memfence_callback_queue[proc].push_back(thread);
}


bool GpusimInterface::ToGpusimDram_empty(unsigned partition_id) {
   if(!g_system_ptr->getChip(0)->usingGpusimDram())
       return true;

   GpusimDramInterface* dramInterface_ptr = g_system_ptr->getChip(0)->getDramInterface(partition_id);
   return dramInterface_ptr->inputQueueEmpty();
}
MsgPtr* GpusimInterface::ToGpusimDram_top(unsigned partition_id) {
   GpusimDramInterface* dramInterface_ptr = g_system_ptr->getChip(0)->getDramInterface(partition_id);
   return dramInterface_ptr->peekInputRequest();
}
void GpusimInterface::ToGpusimDram_pop(unsigned partition_id) {
   GpusimDramInterface* dramInterface_ptr = g_system_ptr->getChip(0)->getDramInterface(partition_id);
   dramInterface_ptr->popInputRequest();
}


void GpusimInterface::FromGpusimDram_push(unsigned partition_id, MsgPtr* msg_ptr) {
   GpusimDramInterface* dramInterface_ptr = g_system_ptr->getChip(0)->getDramInterface(partition_id);
   dramInterface_ptr->pushResponseRequest(msg_ptr);
}


void GpusimInterface::flushAllL1DCaches(ostream& out) {
    g_system_ptr->getChip(0)->flushAllL1DCaches(out);
}



//**************************************************************************
// Useful functions if Ruby needs to read/write physical memory when running with Opal
integer_t GpusimInterface::readPhysicalMemory(int procID,
                                           physical_address_t address,
                                           int len ){
	assert(0);
	return 0;
  //return SIMICS_read_physical_memory(procID, address, len);
}

//**************************************************************************
void GpusimInterface::writePhysicalMemory( int procID,
                                        physical_address_t address,
                                        integer_t value,
                                        int len ){
	assert(0);
  //SIMICS_write_physical_memory(procID, address, value, len);
}

//***************************************************************
// notifies Opal to print debug info
void GpusimInterface::printDebug(){
  //(*m_opal_intf->printDebug)();
}


