#include "gpusim_ruby.h"

#include "protocol_name.h"
#include "System.h"
#include "init.h"
#include "Tester.h"
#include "EventQueue.h"
#include "getopt.h"
#include "Network.h"
#include "CacheRecorder.h"
#include "Tracer.h"

#include "System.h"
#include "SubBlock.h"
#include "Chip.h"
#include "RubyConfig.h"
#include "Sequencer.h"
#include "Message.h"

#include "GpusimInterface.h"

using namespace std;
#include <string>
#include <map>

extern "C" {
#include "simics/api.h"
};

#include "confio.h"
#include "initvar.h"

// A generated file containing the default tester parameters in string form
// The defaults are stored in the variables
// global_default_param and global_default_tester_param global_default_gpusim_param
#include "default_param.h"
#include "tester_param.h"
#include "gpusim_param.h"

static void gpusim_ruby_allocate( void );
static void gpusim_ruby_generate_values( void ) { }


// Convert ruby_wrapper's space information into Ruby's space information
static MemorySpaceType getRubyMemSpace(ruby_memory_space_t space) {
    switch(space) {
       case ruby_global_space:
          return MemorySpaceType_GLOBAL;
       case ruby_local_space:
          return MemorySpaceType_LOCAL;
       default:
          return MemorySpaceType_NULL;
    }
}

// Convert ruby_wrapper's type information into Ruby's type information
static CacheRequestType getRubyMemType(ruby_memory_type_t type) {
    switch(type) {
        case ruby_load_type:
            return CacheRequestType_LD;
        case ruby_store_type:
            return CacheRequestType_ST;
        case ruby_membar_type:
            return CacheRequestType_MEM_FENCE;
        case ruby_atomic_type:
            return CacheRequestType_ATOMIC;
        default:
            assert(0);
            return CacheRequestType_NULL;
    }
}


gpusim_ruby::gpusim_ruby( gpusim_ruby_config R_config) : m_config(R_config) {
   printf("Ruby: constructed gpusim_ruby\n");
}

gpusim_ruby::~gpusim_ruby() {
}

void gpusim_ruby::initialize() {
   printf("Ruby: initializing gpusim_ruby\n");

   // TODO: remove tester param and use our own param
   int param_len = strlen( global_default_param ) + 
                   strlen( global_default_tester_param ) + 
                   strlen( global_default_gpusim_param) + 1;

   char *default_param = (char *) malloc( sizeof(char) * param_len );
   my_default_param = default_param;
   strcpy( default_param, global_default_param );
   strcat( default_param, global_default_tester_param );
   strcat( default_param, global_default_gpusim_param );

   /** note: default_param is included twice in the tester:
    *       -once in init.C
    *       -again in this file
    */
   initvar_t *ruby_initvar = new initvar_t( "ruby", "../../../ruby_clean/",
                                            default_param,
                                            &gpusim_ruby_allocate,
                                            &gpusim_ruby_generate_values,
                                            NULL,
                                            NULL );
   my_initvar = ruby_initvar;
   ruby_initvar->checkInitialization();
   //parseOptions(argc, argv);

   // Set options here
   g_NUM_PROCESSORS = m_config.m_num_processors;
   g_NUM_SMT_THREADS = m_config.m_num_warps_per_processor;
   g_PROCS_PER_CLUSTER = m_config.m_num_procs_per_cluster;
   if (m_config.m_CMP) {
      g_PROCS_PER_CHIP = m_config.m_num_processors;
      g_NUM_L2_BANKS = m_config.m_num_L2_banks;
      g_NUM_MEMORIES = m_config.m_num_memories;
   }
   g_GARNET_NETWORK = m_config.m_garnet;
   g_FLIT_SIZE = m_config.m_flit_size;
   NUMBER_OF_VIRTUAL_NETWORKS = m_config.m_vns;
   g_VCS_PER_CLASS = m_config.m_vcs_per_class;
   g_BUFFER_SIZE = m_config.m_vc_buffer_size;

   // Convert from core clock latency to ruby clock latencies
   // Scale by latency multiplier
   L1_to_L2_MSG_LATENCY = (unsigned) ((float)m_config.m_latency_L1toL2 * m_config.m_icnt_freq/m_config.m_core_freq * m_config.m_latency_L1toL2_multiplier);
   L1_to_L2_DATA_LATENCY = (unsigned) ((float)m_config.m_latency_L1toL2 * m_config.m_icnt_freq/m_config.m_core_freq * m_config.m_latency_L1toL2_multiplier);
   L2_to_MEM_MSG_LATENCY = (unsigned) ((float)m_config.m_latency_L2toMem * m_config.m_icnt_freq/m_config.m_core_freq * m_config.m_latency_L2toMem_multiplier);
   L2_to_MEM_DATA_LATENCY = (unsigned) ((float)m_config.m_latency_L2toMem * m_config.m_icnt_freq/m_config.m_core_freq * m_config.m_latency_L2toMem_multiplier);

   DEBUG_START_TIME = m_config.m_debug_start_time;
   DEBUG_FILTER_STRING = strdup(m_config.m_debug_filter_string);
   DEBUG_VERBOSITY_STRING = strdup(m_config.m_debug_verbosity_string);
   DEBUG_OUTPUT_FILENAME = strdup(m_config.m_debug_output_filename);

   NUMBER_OF_TBES = m_config.m_num_TBE;
   g_SEQUENCER_OUTSTANDING_REQUESTS = m_config.m_num_seq_entries;
   PROCESSOR_BUFFER_SIZE = m_config.m_num_seq_entries;   // set the mandatory queue size to be same as sequencer size
   // these are not used in MESI or Token
   // NUMBER_OF_L1_TBES = m_num_L1_TBE; 
   // NUMBER_OF_L2_TBES = m_num_L2_TBE;

   g_GMEM_SKIP_L1D = m_config.m_gmem_skip_L1D;
   WRITES_STALL_AT_MSHR = m_config.m_writes_stall_at_mshr;
   CL_FIXED_LEASE = m_config.m_cl_fixed_lease;
   CL_PREDICTOR_TYPE = m_config.m_cl_predictor_type;
   CL_PRED_GLOBAL_WRITEUNEXPIRED_PENALTY = m_config.m_cl_pred_global_writeunexpired;
   CL_PRED_GLOBAL_L2EVICT_PENALTY = m_config.m_cl_pred_global_l2evict;
   CL_PRED_GLOBAL_EXPIREHIT_CREDIT = m_config.m_cl_pred_global_expiredhit;
   CL_PRED_PERL2_WRITEUNEXPIRED_PENALTY = m_config.m_cl_pred_perL2_writeunexpired;
   CL_PRED_PERL2_L2EVICT_PENALTY = m_config.m_cl_pred_perL2_l2evict;
   CL_PRED_PERL2_EXPIREHIT_CREDIT = m_config.m_cl_pred_perL2_expiredhit;
   CL_PRED_PCADAPTIVE_WRITEUNEXPIRED_PENALTY = m_config.m_cl_pred_pcadaptive_writeunexpired;
   CL_PRED_PCADAPTIVE_L2EVICT_PENALTY = m_config.m_cl_pred_pcadaptive_l2evict;
   CL_PRED_PCADAPTIVE_EXPIREHIT_CREDIT = m_config.m_cl_pred_pcadaptive_expiredhit;
   CL_PRED_PCFIXED_INFILE = m_config.m_cl_pred_pcfixed_infile;
   CL_PRED_ADDRFIXED_INFILE = m_config.m_cl_pred_addrfixed_infile;

   LIFETIME_PROFILER_PERPC_OUTMODE = m_config.m_lifetime_profiler_perpc_outmode;
   LIFETIME_PROFILER_PERPC_OUTFILE = m_config.m_lifetime_profiler_perpc_outfile;
   LIFETIME_PROFILER_PERADDR_OUTMODE = m_config.m_lifetime_profiler_peraddr_outmode;
   LIFETIME_PROFILER_PERADDR_OUTFILE = m_config.m_lifetime_profiler_peraddr_outfile;

   LIFETIMETRACE_ENABLED = m_config.m_lifetimetrace_enabled;
   LIFETIMETRACE_OUTFILE = m_config.m_lifetimetrace_outfile;

   L1CACHE_TRANSITIONS_PER_RUBY_CYCLE = m_config.m_l1_t_per_cycle;
   L2CACHE_TRANSITIONS_PER_RUBY_CYCLE = m_config.m_l2_t_per_cycle;
   DIRECTORY_TRANSITIONS_PER_RUBY_CYCLE = m_config.m_dir_t_per_cycle;

   L1_CACHE_ASSOC = m_config.m_l1_assoc;
   L1_CACHE_NUM_SETS_BITS = m_config.m_l1_sets_bits;
   L2_CACHE_ASSOC = m_config.m_l2_assoc;
   L2_CACHE_NUM_SETS_BITS = m_config.m_l2_sets_bits;
   L2_DIRECTORY_ASSOC = m_config.m_dir_assoc;
   L2_DIRECTORY_NUM_SETS_BITS = m_config.m_dir_sets_bits;

   g_DEADLOCK_THRESHOLD = m_config.m_deadlock_threshold;

   printf("Ruby: Option set g_NUM_PROCESSORS=%u\n", g_NUM_PROCESSORS);
   printf("Ruby: Option set g_NUM_SMT_THREADS=%u\n", g_NUM_SMT_THREADS);


   ruby_initvar->allocate();

   g_system_ptr->printConfig(cout);
   cout << "Testing clear stats...";
   g_system_ptr->clearStats();
   cout << "Done." << endl;

   m_driver_ptr = dynamic_cast<GpusimInterface*>(g_system_ptr->getDriver()); 
}

void gpusim_ruby::print_stats() {
   g_system_ptr->printStats(cout);
}

void gpusim_ruby::destroy() {
   g_debug_ptr->closeDebugOutputFile();

   free(my_default_param);
   delete my_initvar;
   // Clean up
   destroy_simulator();
   cerr << "Success: " << CURRENT_PROTOCOL << endl;
}



//
// Helper functions
//

static void gpusim_ruby_allocate( void )
{
  init_simulator();
}

/*
static void parseOptions(int argc, char **argv)
{
  cout << "Parsing command line arguments:" << endl;

  // construct the short arguments string
  int counter = 0;
  string short_options;
  while (long_options[counter].name != NULL) {
    short_options += char(long_options[counter].val);
    if (long_options[counter].has_arg == required_argument) {
      short_options += char(':');
    }
    counter++;
  }

  char c;
  bool error;
  while ((c = getopt_long (argc, argv, short_options.c_str(), long_options, (int *) 0)) != EOF) {
    switch (c) {
    case 0:
      break;

    case 'c':
      checkArg(c);
      cout << "  component filter string = " << optarg << endl;
      error = Debug::checkFilterString( optarg );
      if (error) {
        usageInstructions();
      }
      DEBUG_FILTER_STRING = strdup( optarg );
      break;

    case 'h':
      usageInstructions();
      break;

    case 'v':
      checkArg(c);
      cout << "  verbosity string = " << optarg << endl;
      error = Debug::checkVerbosityString(optarg);
      if (error) {
        usageInstructions();
      }
      DEBUG_VERBOSITY_STRING = strdup( optarg );
      break;

    case 'r': {
      checkArg(c);
      if (string(optarg) == "random") {
        g_RANDOM_SEED = time(NULL);
      } else {
        g_RANDOM_SEED = atoi(optarg);
        if (g_RANDOM_SEED == 0) {
          usageInstructions();
        }
      }
      break;
    }

    case 'l': {
      checkArg(c);
      g_tester_length = atoi(optarg);
      cout << "  length of run = " << g_tester_length << endl;
      if (g_tester_length == 0) {
        usageInstructions();
      }
      break;
    }

    case 'q': {
      checkArg(c);
      g_synthetic_locks = atoi(optarg);
      cout << "  locks in synthetic workload = " << g_synthetic_locks << endl;
      if (g_synthetic_locks == 0) {
        usageInstructions();
      }
      break;
    }

    case 'p': {
      checkArg(c);
      g_NUM_PROCESSORS = atoi(optarg);
      break;
    }

    case 'a': {
      checkArg(c);
      g_PROCS_PER_CHIP = atoi(optarg);
      cout << "  g_PROCS_PER_CHIP: " << g_PROCS_PER_CHIP << endl;
      break;
    }

    case 'e': {
      checkArg(c);
      g_NUM_L2_BANKS = atoi(optarg);
      cout << "  g_NUM_L2_BANKS: " << g_NUM_L2_BANKS << endl;
      break;
    }

    case 'm': {
      checkArg(c);
      g_NUM_MEMORIES = atoi(optarg);
      cout << "  g_NUM_MEMORIES: " << g_NUM_MEMORIES << endl;
      break;
    }

    case 's': {
      checkArg(c);
      long long start_time = atoll(optarg);
      cout << "  debug start cycle = " << start_time << endl;
      if (start_time == 0) {
        usageInstructions();
      }
      DEBUG_START_TIME = start_time;
      break;
    }

    case 'b': {
      checkArg(c);
      int bandwidth = atoi(optarg);
      cout << "  bandwidth per link (MB/sec) = " << bandwidth << endl;
      g_endpoint_bandwidth = bandwidth;
      if (bandwidth == 0) {
        usageInstructions();
      }
      break;
    }

    case 't': {
      checkArg(c);
      g_bash_bandwidth_adaptive_threshold = atof(optarg);
      if ((g_bash_bandwidth_adaptive_threshold > 1.1) || (g_bash_bandwidth_adaptive_threshold < -0.1)) {
        cerr << "Error: Bandwidth adaptive threshold must be between 0.0 and 1.0" << endl;
        usageInstructions();
      }

      break;
    }

    case 'k': {
      checkArg(c);
      g_think_time = atoi(optarg);
      break;
    }

    case 'o':
      checkArg(c);
      cout << "  output file = " << optarg << endl;
      DEBUG_OUTPUT_FILENAME = strdup( optarg );
      break;

    case 'z':
      checkArg(c);
      trace_filename = string(optarg);
      cout << "  tracefile = " << trace_filename << endl;
      break;

      case 'n':
        checkArg(c);
        cout << "  topology = " << string(optarg) << endl;
        g_NETWORK_TOPOLOGY = strdup(optarg);
        break;

    default:
      cerr << "parameter '" << c << "' unknown" << endl;
      usageInstructions();
    }
  }

  if ((trace_filename != "") || (g_tester_length != 0)) {
    if ((trace_filename != "") && (g_tester_length != 0)) {
      cerr << "Error: both a run length (-l) and a trace file (-z) have been specified." << endl;
      usageInstructions();
    }
  } else {
    cerr << "Error: either run length (-l) must be > 0 or a trace file (-z) must be specified." << endl;
    usageInstructions();
  }
}
  */


void gpusim_ruby::send_request( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                                ruby_memory_type_t type, bool isPriv, ruby_memory_space_t space, ruby_mem_access_byte_mask_t access_mask,
                                void* mf)
{
   GpusimInterface::makeRequest(addr, req_size, sid, tid, pc, getRubyMemType(type), isPriv, getRubyMemSpace(space), access_mask, mf);
}

  void gpusim_ruby::send_memfence_request( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                                  ruby_memory_type_t type, bool isPriv, ruby_memory_space_t space, ruby_mem_access_byte_mask_t access_mask)
  {
     GpusimInterface::makeMemfenceRequest(addr, req_size, sid, tid, pc, getRubyMemType(type), isPriv, getRubyMemSpace(space), access_mask);
  }

void gpusim_ruby::advance_time()
{
   GpusimInterface::advanceTime(); 
}


bool gpusim_ruby::callbackQueueEmpty(unsigned core_id)
{
   return m_driver_ptr->callbackQueueEmpty(core_id); 
}
unsigned long long gpusim_ruby::callbackQueueTop(unsigned core_id)
{
   return m_driver_ptr->callbackQueueTop(core_id); 
}
void gpusim_ruby::callbackQueuePop(unsigned core_id)
{
   m_driver_ptr->callbackQueuePop(core_id); 
}

bool gpusim_ruby::memfenceCallbackQueueEmpty(unsigned core_id)
{
   return m_driver_ptr->memfenceCallbackQueueEmpty(core_id);
}
unsigned long long gpusim_ruby::memfenceCallbackQueueTop(unsigned core_id)
{
   return m_driver_ptr->memfenceCallbackQueueTop(core_id);
}
void gpusim_ruby::memfenceCallbackQueuePop(unsigned core_id)
{
   m_driver_ptr->memfenceCallbackQueuePop(core_id);
}

bool gpusim_ruby::skipL1CallbackQueueEmpty(unsigned core_id)
{
   return m_driver_ptr->skipL1CallbackQueueEmpty(core_id);
}
void gpusim_ruby::skipL1CallbackQueueTop(unsigned core_id, unsigned long long& addr, void** memfetch)
{
   m_driver_ptr->skipL1CallbackQueueTop(core_id, addr, memfetch);
}
void gpusim_ruby::skipL1CallbackQueuePop(unsigned core_id)
{
   m_driver_ptr->skipL1CallbackQueuePop(core_id);
}

int gpusim_ruby::isReady( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, 
                          ruby_memory_type_t type, bool isPriv, ruby_memory_space_t space)
{
   return GpusimInterface::isReady(addr, req_size, sid, tid, getRubyMemType(type), getRubyMemSpace(space));
}


bool gpusim_ruby::ToGpusimDram_empty(unsigned partition_id) {
   return m_driver_ptr->ToGpusimDram_empty(partition_id);
}
ruby_dram_req_info gpusim_ruby::ToGpusimDram_top(unsigned partition_id) {
   MsgPtr* msg_ptr = m_driver_ptr->ToGpusimDram_top(partition_id);
   const MemoryMsg* memMess = dynamic_cast<const MemoryMsg*>(msg_ptr->ref());
   physical_address_t addr = memMess->getAddress().getAddress();
   MemoryRequestType type = memMess->getType();
   bool is_mem_read = (type == MemoryRequestType_MEMORY_READ);

   ruby_dram_req_info dram_req;
   dram_req.m_addr = addr;
   dram_req.m_msg_ptr = (void*) msg_ptr;
   dram_req.m_write = is_mem_read ? false : true;
   dram_req.m_size = 128;     //TODO: remove hardcoding of size
   return dram_req;
}
void gpusim_ruby::ToGpusimDram_pop(unsigned partition_id) {
   m_driver_ptr->ToGpusimDram_pop(partition_id);
}


void gpusim_ruby::FromGpusimDram_push(unsigned partition_id, ruby_dram_req_info dram_req) {
   MsgPtr* msg_ptr = (MsgPtr*) dram_req.m_msg_ptr;
   m_driver_ptr->FromGpusimDram_push(partition_id,msg_ptr);
}


void gpusim_ruby::set_benchmark_contains_membar(bool flag) {
    printf("Setting BENCHMARK_CONTAINS_MEMBAR=%s\n", (flag?"TRUE":"FALSE"));
    BENCHMARK_CONTAINS_MEMBAR = flag;
}

void gpusim_ruby::flushAllL1DCaches() {
    m_driver_ptr->flushAllL1DCaches(cout);
}

//
// Accessor functions used by GPGPU-Sim to peek into Ruby's configuration
//

// block size in bytes
unsigned gpusim_ruby::get_ruby_block_size() { return g_DATA_BLOCK_BYTES; }
