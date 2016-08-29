#include "ruby_wrapper.h"
#include "mem_fetch.h"
#include "gpu-config.h"
#include "../option_parser.h"

#include "../gems-2.1.1/ruby_clean/gpusim_ruby/gpusim_ruby.h"


// Convert information from memfetch into enum recognized by the interface
static ruby_memory_space_t get_ruby_mem_space(mem_fetch * mf) {
   if(mf->get_inst().space.is_global())
      return ruby_global_space;
   else if(mf->get_inst().space.is_local())
      return ruby_local_space;
   else {
      assert(mf->is_membar());
      return ruby_global_space;
   }
}
static ruby_memory_type_t get_ruby_mem_type(mem_fetch * mf) {
   if(mf->is_membar())
       return ruby_membar_type;
   else if (mf->isatomic())
      return ruby_atomic_type;
   else
       return (mf->get_is_write()? ruby_store_type : ruby_load_type);
}

void ruby_config::reg_options(class OptionParser * opp)
{
    option_parser_register(opp, "-ruby_is_CMP", OPT_BOOL,
                           &m_is_CMP, "CMP Protocol used in RUBY (1=On (default), 0=Off)",
                           "1");
    option_parser_register(opp, "-ruby_garnet", OPT_BOOL,
                           &m_garnet, "Use GARNET in RUBY (1=On (default), 0=Off)",
                           "1");
    option_parser_register(opp, "-ruby_debug_start_time", OPT_UINT32,
                           &m_debug_start_time, "When to start dumping debug traces in RUBY (0=Off)",
                           "0");
    option_parser_register(opp, "-ruby_debug_filter_string", OPT_CSTR, &m_debug_filter_string, 
                           "String specifying which unit to monitor in debug traces in RUBY",
                           "none");
    option_parser_register(opp, "-ruby_debug_verbosity_string", OPT_CSTR, &m_debug_verbosity_string, 
                           "String specifying the verbosity of debug traces in RUBY (none|low|med|high)",
                           "none");
    option_parser_register(opp, "-ruby_debug_output_filename", OPT_CSTR, &m_debug_output_filename, 
                           "String specifying the output filename of debug traces in RUBY",
                           "none");
    option_parser_register(opp, "-ruby_num_TBE", OPT_UINT32,
                           &m_num_TBE, "Number of TBEs for each unit (default=128)",
                           "128");
    option_parser_register(opp, "-ruby_num_seq_entries", OPT_UINT32,
                           &m_num_seq_entries, "Number of entries in each sequencer (default=64)",
                           "64");
    option_parser_register(opp, "-ruby_mshr_count", OPT_UINT32,
                           &m_mshr_count, "Number of Ruby MSHRs (default=64)",
                           "64");
    option_parser_register(opp, "-ruby_mshr_size", OPT_UINT32,
                           &m_mshr_size, "Number of accesses that can be merged in each Ruby MSHR (default=32)",
                           "32");

    option_parser_register(opp, "-ruby_flit_size", OPT_UINT32,
                           &m_flit_size, "Flit size (default=32)",
                           "32");
    option_parser_register(opp, "-ruby_vns", OPT_UINT32,
                           &m_vns, "Number of virtual networks (default=2)",
                           "2");
    option_parser_register(opp, "-ruby_vcs_per_class", OPT_UINT32,
                           &m_vcs_per_class, "Virtual channels per class (default=1)",
                           "1");
    option_parser_register(opp, "-ruby_vc_buffer_size", OPT_UINT32,
                           &m_vc_buffer_size, "Virtual channel buffer size in # flits (default=8)",
                           "8");

    option_parser_register(opp, "-ruby_latencies", OPT_CSTR, &m_latencies,
                           "Minimum latencies in core cycles (L1->L2:L2->Mem) (default=70:100)",
                           "70:100" );
    option_parser_register(opp, "-ruby_latencies_multiplier", OPT_CSTR, &m_latencies_multiplier,
                           "Multiplier for ruby's latencies (L1->L2:L2->Mem) (default=1.0:1.0)",
                           "1.0:1.0" );

    option_parser_register(opp, "-ruby_l1_t_per_cycle", OPT_UINT32,
                           &m_l1_t_per_cycle, "Number of transitions per Ruby L1 cache controller per cycle (default=32)",
                           "32");
    option_parser_register(opp, "-ruby_l2_t_per_cycle", OPT_UINT32,
                           &m_l2_t_per_cycle, "Number of transitions per Ruby L2 cache controller per cycle (default=32)",
                           "32");
    option_parser_register(opp, "-ruby_dir_t_per_cycle", OPT_UINT32,
                           &m_dir_t_per_cycle, "Number of transitions per Ruby memory controller per cycle (default=32)",
                           "32");

    option_parser_register(opp, "-ruby_cl_fixed_lease", OPT_UINT32,
                           &m_cl_fixed_lease, "Cache Lease option - fixed lease time in Ruby/ICNT cycles (default=800)",
                           "800");

    option_parser_register(opp, "-ruby_cl_predictor_type", OPT_UINT32,
                           &m_cl_predictor_type,
                           "Cache Lease option - predictor type. 0=fixed, 1=global, 2=perL2, 3=PC-fixed, 4=PC-adaptive, 5=addr-fixed, 6=oracle, 7=PCsampler (default=0)",
                           "0");
    option_parser_register(opp, "-ruby_cl_pred_global", OPT_CSTR, &m_cl_pred_global_string,
                           "Cache Lease option - predictor settings for global predictor (writeunexpired:l2evict:expiredhit) (default=64:16:1)",
                           "64:16:1" );
    option_parser_register(opp, "-ruby_cl_pred_perL2", OPT_CSTR, &m_cl_pred_perL2_string,
                           "Cache Lease option - predictor settings for per-L2 predictor (writeunexpired:l2evict:expiredhit) (default=64:16:1)",
                           "64:16:1" );
    option_parser_register(opp, "-ruby_cl_pred_pcfixed", OPT_CSTR, &m_cl_pred_pcfixed_string,
                           "Cache Lease option - predictor settings for PC-fixed predictor (inputfile) (default=lifetime_profiler_perpc.txt)",
                           "lifetime_profiler_perpc.txt" );
    option_parser_register(opp, "-ruby_cl_pred_pcadaptive", OPT_CSTR, &m_cl_pred_pcadaptive_string,
                           "Cache Lease option - predictor settings for PC-adaptive predictor (writeunexpired:l2evict:expiredhit) (default=64:16:1)",
                           "64:16:1" );
    option_parser_register(opp, "-ruby_cl_pred_addrfixed", OPT_CSTR, &m_cl_pred_addrfixed_string,
                           "Cache Lease option - predictor settings for Addr-fixed predictor (inputfile) (default=lifetime_profiler_peraddr.txt)",
                           "lifetime_profiler_peraddr.txt" );

    option_parser_register(opp, "-ruby_send_memfence_to_protocol", OPT_BOOL,
                           &m_send_memfence_to_protocol, "Mem fence requests are sent to Ruby protocol (1=On (default), 0=Off)",
                           "1");

    option_parser_register(opp, "-ruby_writes_stall_at_mshr", OPT_BOOL,
                           &m_writes_stall_at_mshr, "Write and atomics are stalled by pending requests in MSHRs (1=On, 0=Off (default) )",
                           "0");

    option_parser_register(opp, "-ruby_single_write_per_warp", OPT_BOOL,
                           &m_single_write_per_warp, "Only one outstanding write request per warp (1=On, 0=Off (default) )",
                           "0");

    option_parser_register(opp, "-ruby_locality_profiler", OPT_CSTR, &m_locality_profiler_string,
                   "locality profiler configuration. Bins specified as pow2"
                   " {<start_bin>,<end_bin>, <skip_bin>}",
                   "none" );

    option_parser_register(opp, "-ruby_l1_assoc", OPT_UINT32,
                           &m_l1_assoc, "Ruby L1 associativity (default=4)",
                           "4");
    option_parser_register(opp, "-ruby_l1_sets_bits", OPT_UINT32,
                           &m_l1_sets_bits, "Ruby L1 # of set bits (#sets=2^bits) (default=6)",
                           "6");
    option_parser_register(opp, "-ruby_l2_assoc", OPT_UINT32,
                           &m_l2_assoc, "Ruby L2 associativity (default=8)",
                           "8");
    option_parser_register(opp, "-ruby_l2_sets_bits", OPT_UINT32,
                           &m_l2_sets_bits, "Ruby L2 # of set bits (#sets=2^bits) (default=9)",
                           "9");
    option_parser_register(opp, "-ruby_dir_assoc", OPT_UINT32,
                           &m_dir_assoc, "Ruby Directory associativity (default=8)",
                           "8");
    option_parser_register(opp, "-ruby_dir_sets_bits", OPT_UINT32,
                           &m_dir_sets_bits, "Ruby Directory # of set bits (#sets=2^bits) (default=9)",
                           "9");


    option_parser_register(opp, "-ruby_lifetime_profiler_perpc_outmode", OPT_UINT32,
                           &m_lifetime_profiler_perpc_outmode, "Output mode for per-pc lifetime profiler (0=none, 1=stdout, 2=file, 3=both) (default=0)",
                           "0");
    option_parser_register(opp, "-ruby_lifetime_profiler_perpc_outfile", OPT_CSTR, &m_lifetime_profiler_perpc_outfile,
                           "Filename for output of per-pc information from lifetime profiler (default=lifetime_profiler_perpc.txt)",
                           "lifetime_profiler_perpc.txt");

    option_parser_register(opp, "-ruby_lifetime_profiler_peraddr_outmode", OPT_UINT32,
                           &m_lifetime_profiler_peraddr_outmode, "Output mode for per-address lifetime profiler (0=none, 1=stdout, 2=file, 3=both) (default=0)",
                           "0");
    option_parser_register(opp, "-ruby_lifetime_profiler_peraddr_outfile", OPT_CSTR, &m_lifetime_profiler_peraddr_outfile,
                           "Filename for output of per-address information from lifetime profiler (default=lifetime_profiler_peraddr.txt)",
                           "lifetime_profiler_peraddr.txt");

    option_parser_register(opp, "-ruby_lifetimetrace", OPT_BOOL,
                           &m_lifetimetrace_enabled, "Lifetime trace enabled? (1=On, 0=Off (default) )",
                           "0");
    option_parser_register(opp, "-ruby_lifetimetrace_outfile", OPT_CSTR, &m_lifetimetrace_outfile,
                           "Filename for output of lifetime trace (default=lifetime_trace.txt)",
                           "lifetime_trace.txt");


    option_parser_register(opp, "-ruby_flush_L1D", OPT_BOOL,
                           &m_flush_L1D, "Flush Ruby's L1D at kernel finish? (1=On, 0=Off (default) )",
                           "0");
}

void ruby_config::init() {
   m_latency_L1toL2 = 0;
   m_latency_L2toMem = 0;
   int ntok = sscanf(m_latencies,"%u:%u",
                        &m_latency_L1toL2, &m_latency_L2toMem);
   if( ntok < 2 ) {
       printf("GPGPU-Sim uArch: error while parsing configuration string ruby_latencies\n");
       abort();
   }

   m_latency_L1toL2_multiplier = 0.0;
   m_latency_L2toMem_multiplier = 0.0;
   ntok = sscanf(m_latencies_multiplier,"%f:%f",
                        &m_latency_L1toL2_multiplier, &m_latency_L2toMem_multiplier);
   if( ntok < 2 ) {
       printf("GPGPU-Sim uArch: error while parsing configuration string ruby_latencies_multiplier\n");
       abort();
   }

   m_cl_pred_global_writeunexpired = 0;
   m_cl_pred_global_l2evict = 0;
   m_cl_pred_global_expiredhit = 0;
   ntok = sscanf(m_cl_pred_global_string,"%u:%u:%u",
                        &m_cl_pred_global_writeunexpired, &m_cl_pred_global_l2evict, &m_cl_pred_global_expiredhit);
   if( ntok < 3 ) {
       printf("GPGPU-Sim uArch: error while parsing configuration string ruby_cl_pred_global\n");
       abort();
   }

   m_cl_pred_perL2_writeunexpired = 0;
   m_cl_pred_perL2_l2evict = 0;
   m_cl_pred_perL2_expiredhit = 0;
   ntok = sscanf(m_cl_pred_perL2_string,"%u:%u:%u",
                        &m_cl_pred_perL2_writeunexpired, &m_cl_pred_perL2_l2evict, &m_cl_pred_perL2_expiredhit);
   if( ntok < 3 ) {
       printf("GPGPU-Sim uArch: error while parsing configuration string ruby_cl_pred_preL2\n");
       abort();
   }

   m_cl_pred_pcadaptive_writeunexpired = 0;
   m_cl_pred_pcadaptive_l2evict = 0;
   m_cl_pred_pcadaptive_expiredhit = 0;
   ntok = sscanf(m_cl_pred_pcadaptive_string,"%u:%u:%u",
                        &m_cl_pred_pcadaptive_writeunexpired, &m_cl_pred_pcadaptive_l2evict, &m_cl_pred_pcadaptive_expiredhit);
   if( ntok < 3 ) {
       printf("GPGPU-Sim uArch: error while parsing configuration string ruby_cl_pred_pcadaptive\n");
       abort();
   }

   m_cl_pred_pcfixed_infile = m_cl_pred_pcfixed_string;     // lazy method since only one parameter passed in string
   m_cl_pred_addrfixed_infile = m_cl_pred_addrfixed_string;     // lazy method since only one parameter passed in string


   m_locality_profiler_start_bin = 0;
   m_locality_profiler_end_bin = 0;
   m_locality_profiler_end_bin = 0;
   ntok = sscanf(m_locality_profiler_string,"%u:%u:%u",
                     &m_locality_profiler_start_bin, &m_locality_profiler_end_bin, &m_locality_profiler_skip_bin);

   m_locality_profiler_enabled = true;
   if( ntok < 3 ) {
      if ( !strcmp(m_locality_profiler_string,"none") ) {
          m_locality_profiler_enabled = false;
      } else {
         printf("GPGPU-Sim uArch: error while parsing configuration string locality_profiler\n");
         abort();
      }
   }
}

/*
 * Constructor for Ruby's config
 * Convert from gpgpusim config to Ruby config
 */
gpusim_ruby_config::gpusim_ruby_config(const gpgpu_sim_config &config) {
    m_core_freq = config.core_freq;
    m_icnt_freq = config.icnt_freq;
    m_num_processors = config.m_shader_config.num_shader();
    m_num_procs_per_cluster = config.m_shader_config.n_simt_cores_per_cluster;
    m_num_warps_per_processor = config.m_shader_config.max_warps_per_shader;
    m_num_L2_banks = config.m_memory_config.m_n_mem;
    m_num_memories = 0; // Don't need off-chip components in Ruby as DRAM interface is included in L2 cache
    m_CMP = config.m_memory_config.m_ruby_config.m_is_CMP;
    m_garnet = config.m_memory_config.m_ruby_config.m_garnet;
    m_debug_start_time = config.m_memory_config.m_ruby_config.m_debug_start_time;
    m_debug_filter_string = config.m_memory_config.m_ruby_config.m_debug_filter_string;
    m_debug_verbosity_string = config.m_memory_config.m_ruby_config.m_debug_verbosity_string;
    m_debug_output_filename = config.m_memory_config.m_ruby_config.m_debug_output_filename;
    m_num_TBE = config.m_memory_config.m_ruby_config.m_num_TBE;
    m_num_seq_entries = config.m_memory_config.m_ruby_config.m_num_seq_entries;
    m_flit_size = config.m_memory_config.m_ruby_config.m_flit_size;
    m_vns = config.m_memory_config.m_ruby_config.m_vns;
    m_vcs_per_class = config.m_memory_config.m_ruby_config.m_vcs_per_class;
    m_vc_buffer_size = config.m_memory_config.m_ruby_config.m_vc_buffer_size;
    m_latency_L1toL2 = config.m_memory_config.m_ruby_config.m_latency_L1toL2;
    m_latency_L2toMem = config.m_memory_config.m_ruby_config.m_latency_L2toMem;
    m_latency_L1toL2_multiplier = config.m_memory_config.m_ruby_config.m_latency_L1toL2_multiplier;
    m_latency_L2toMem_multiplier = config.m_memory_config.m_ruby_config.m_latency_L2toMem_multiplier;
    m_gmem_skip_L1D = config.m_shader_config.gmem_skip_L1D;
    m_writes_stall_at_mshr = config.m_memory_config.m_ruby_config.m_writes_stall_at_mshr;
    m_cl_fixed_lease = config.m_memory_config.m_ruby_config.m_cl_fixed_lease;
    m_cl_predictor_type = config.m_memory_config.m_ruby_config.m_cl_predictor_type;
    m_cl_pred_global_writeunexpired = config.m_memory_config.m_ruby_config.m_cl_pred_global_writeunexpired;
    m_cl_pred_global_l2evict = config.m_memory_config.m_ruby_config.m_cl_pred_global_l2evict;
    m_cl_pred_global_expiredhit = config.m_memory_config.m_ruby_config.m_cl_pred_global_expiredhit;
    m_cl_pred_perL2_writeunexpired = config.m_memory_config.m_ruby_config.m_cl_pred_perL2_writeunexpired;
    m_cl_pred_perL2_l2evict = config.m_memory_config.m_ruby_config.m_cl_pred_perL2_l2evict;
    m_cl_pred_perL2_expiredhit = config.m_memory_config.m_ruby_config.m_cl_pred_perL2_expiredhit;
    m_cl_pred_pcadaptive_writeunexpired = config.m_memory_config.m_ruby_config.m_cl_pred_pcadaptive_writeunexpired;
    m_cl_pred_pcadaptive_l2evict = config.m_memory_config.m_ruby_config.m_cl_pred_pcadaptive_l2evict;
    m_cl_pred_pcadaptive_expiredhit = config.m_memory_config.m_ruby_config.m_cl_pred_pcadaptive_expiredhit;
    m_cl_pred_pcfixed_infile = config.m_memory_config.m_ruby_config.m_cl_pred_pcfixed_infile;
    m_cl_pred_addrfixed_infile = config.m_memory_config.m_ruby_config.m_cl_pred_addrfixed_infile;
    m_lifetime_profiler_perpc_outmode = config.m_memory_config.m_ruby_config.m_lifetime_profiler_perpc_outmode;
    m_lifetime_profiler_perpc_outfile = config.m_memory_config.m_ruby_config.m_lifetime_profiler_perpc_outfile;
    m_lifetime_profiler_peraddr_outmode = config.m_memory_config.m_ruby_config.m_lifetime_profiler_peraddr_outmode;
    m_lifetime_profiler_peraddr_outfile = config.m_memory_config.m_ruby_config.m_lifetime_profiler_peraddr_outfile;
    m_lifetimetrace_enabled = config.m_memory_config.m_ruby_config.m_lifetimetrace_enabled;
    m_lifetimetrace_outfile = config.m_memory_config.m_ruby_config.m_lifetimetrace_outfile;
    m_l1_t_per_cycle = config.m_memory_config.m_ruby_config.m_l1_t_per_cycle;
    m_l2_t_per_cycle = config.m_memory_config.m_ruby_config.m_l2_t_per_cycle;
    m_dir_t_per_cycle = config.m_memory_config.m_ruby_config.m_dir_t_per_cycle;
    m_l1_assoc = config.m_memory_config.m_ruby_config.m_l1_assoc;
    m_l2_assoc = config.m_memory_config.m_ruby_config.m_l2_assoc;
    m_dir_assoc = config.m_memory_config.m_ruby_config.m_dir_assoc;
    m_l1_sets_bits = config.m_memory_config.m_ruby_config.m_l1_sets_bits;
    m_l2_sets_bits = config.m_memory_config.m_ruby_config.m_l2_sets_bits;
    m_dir_sets_bits = config.m_memory_config.m_ruby_config.m_dir_sets_bits;
    m_deadlock_threshold = config.gpu_deadlock_threshold;
}

ruby_wrapper::ruby_wrapper(const gpgpu_sim_config &g_config)
   : m_ruby( new gpusim_ruby(gpusim_ruby_config(g_config)) ),
     m_num_cores(g_config.m_shader_config.num_shader()),
     m_mshr(g_config.m_shader_config.num_shader()),
     m_locality_profiler(g_config.m_shader_config.num_shader(), g_config.m_shader_config.max_warps_per_shader, g_config.m_memory_config.m_ruby_config),
     m_config(g_config.m_memory_config.m_ruby_config),
     m_gpgpu_config(g_config)
{
	printf("GPGPU-Sim Ruby: constructing ruby_wrapper\n");

	m_ruby->initialize();

   for (size_t c = 0; c < m_mshr.size(); c++) {
      m_mshr[c] = new ruby_mshr_table(g_config.m_memory_config.m_ruby_config.m_mshr_count, g_config.m_memory_config.m_ruby_config.m_mshr_size,
                                      g_config.m_memory_config.m_ruby_config);
   }
}

ruby_wrapper::~ruby_wrapper() {

   printf("GPGPU-Sim Ruby: destroying ruby_wrapper\n");

   m_ruby->destroy();
   delete m_ruby;

   for (size_t c = 0; c < m_mshr.size(); c++) {
      delete m_mshr[c]; 
   }
}

bool ruby_wrapper::memfenceCallbackQueueEmpty(unsigned core_id)
{
   return m_ruby->memfenceCallbackQueueEmpty(core_id);
}
unsigned long long ruby_wrapper::memfenceCallbackQueueTop(unsigned core_id)
{
   return m_ruby->memfenceCallbackQueueTop(core_id);
}
void ruby_wrapper::memfenceCallbackQueuePop(unsigned core_id)
{
    m_ruby->memfenceCallbackQueuePop(core_id);
}


bool ruby_wrapper::skipL1CallbackQueueEmpty(unsigned core_id)
{
   return m_ruby->skipL1CallbackQueueEmpty(core_id);
}
mem_fetch* ruby_wrapper::skipL1CallbackQueueTop(unsigned core_id)
{
   unsigned long long addr;
   mem_fetch* mf;
   m_ruby->skipL1CallbackQueueTop(core_id, addr, (void**)&mf);
   return mf;
}
void ruby_wrapper::skipL1CallbackQueuePop(unsigned core_id)
{
    m_ruby->skipL1CallbackQueuePop(core_id);
}



bool ruby_wrapper::ToGpusimDram_empty(unsigned partition_id) {
   /*if(!m_ruby->ToGpusimDram_empty(partition_id)) {
      ruby_dram_req_info ruby_dram_req = m_ruby->ToGpusimDram_top(partition_id);
      m_ruby->ToGpusimDram_pop(partition_id);
      m_ruby->FromGpusimDram_push(partition_id, ruby_dram_req);
   }*/
   return m_ruby->ToGpusimDram_empty(partition_id);
}

mem_fetch* ruby_wrapper::ToGpusimDram_top(unsigned partition_id) {
   // Create a memfetch in this function
   // NOTE: Can't call this function more than once, otherwise we'll have memory leaks
   ruby_dram_req_info dram_req = m_ruby->ToGpusimDram_top(partition_id);
   mem_access_type dram_access_type = dram_req.m_write ? RUBY_DRAM_W : RUBY_DRAM_R;
   mem_access_t access( dram_access_type, dram_req.m_addr, dram_req.m_size, dram_req.m_write );
   mem_fetch *mf = new mem_fetch( access,
                                  NULL,
                                  8,
                                  -1,
                                  0,
                                  -1,
                                  &m_gpgpu_config.m_memory_config );
   mf->set_ruby_msg_ptr(dram_req.m_msg_ptr);
   return mf;
}

void ruby_wrapper::ToGpusimDram_pop(unsigned partition_id) {
   m_ruby->ToGpusimDram_pop(partition_id);
}

void ruby_wrapper::FromGpusimDram_push(unsigned partition_id, mem_fetch* mf) {
   // Delete the memfetch in this function
   // NOTE: Don't call this function more than once on the same memfetch
   assert(mf != NULL);
   assert(mf->get_ruby_msg_ptr() != NULL);
   ruby_dram_req_info dram_req;
   dram_req.m_msg_ptr = mf->get_ruby_msg_ptr();
   dram_req.m_addr = mf->get_addr();
   dram_req.m_size = mf->get_data_size();
   dram_req.m_write = mf->get_is_write();
   m_ruby->FromGpusimDram_push(partition_id, dram_req);

   delete mf;
}


void ruby_wrapper::flushAllL1DCaches() {
    if(m_config.m_flush_L1D)
        m_ruby->flushAllL1DCaches();
}


void ruby_wrapper::mshr_mark_skipped_write_done(int core_id, mem_fetch* mf) {
   assert(core_id < m_mshr.size());
   unsigned long long line_size = (unsigned long long) m_ruby->get_ruby_block_size();
   unsigned long long block_addr = mf->get_addr() & ~(line_size-1);
   //printf("SKIPPED_WRITE_DONE addr=%x core_id=%d wid=%d\n", block_addr, core_id, mf->get_wid());
   m_mshr[core_id]->mark_skipped_write_done(block_addr, mf->get_wid());
}

void ruby_wrapper::advance_time()
{
   m_ruby->advance_time();

   // Do call back for memory request
   for (unsigned c = 0; c < m_num_cores; c++) {
      if (m_ruby->callbackQueueEmpty(c)) continue; 
      unsigned long long addr = m_ruby->callbackQueueTop(c);
      m_ruby->callbackQueuePop(c); 
      m_mshr[c]->mark_ready(addr); 
      // m_mshr[c]->display(stdout);
   }

   // Profiler
   if(m_config.m_locality_profiler_enabled)
      m_locality_profiler.advance_time(gpu_sim_cycle+gpu_tot_sim_cycle);
}

void ruby_wrapper::send_request( unsigned core_id, mem_fetch * mf )
{

   // only send request if it has not been sent yet
   // WF: mask out non-block address bits?
   bool is_write = mf->get_is_write() or mf->isatomic(); // atomic requires modifying the cache line
   unsigned long long line_size = (unsigned long long) m_ruby->get_ruby_block_size();
   unsigned long long block_addr = mf->get_addr() & ~(line_size-1);

   // mem-fetch must fit within the line size
   assert(mf->get_data_size() <= line_size);

   // PC must be valid
   assert(mf->get_pc() != (address_type)-1);

   ruby_memory_space_t space = get_ruby_mem_space(mf);
   ruby_memory_type_t type = get_ruby_mem_type(mf);


   if(mf->is_membar()) {
      m_ruby->send_memfence_request( block_addr, mf->get_data_size(), core_id, mf->get_wid(), mf->get_pc(), type, false, space, mf->get_byte_mask());
   } else {
      bool sent_to_ruby = false;

      // Three possibilities here
      // 1. Global memory access and gmem_skip_L1D==true: skip MSHRs entirely and send to Ruby
      // 2. Store/atomic and writes_stall_at_mshr==false: inform MSHRs about this write, but don't stall due to MSHRs
      // 3. Else: Merge into MSHR and stall if necessary
      // Don't add request to MSHRs if we are skipping the L1 cache
      if(m_gpgpu_config.m_shader_config.gmem_skip_L1D and space==ruby_global_space) {
         // Skipping MSHRs and L1 for global memory access
         m_ruby->send_request( block_addr, mf->get_data_size(), core_id, mf->get_wid(), mf->get_pc(), type, false, space, mf->get_byte_mask(), mf);
         sent_to_ruby = true;
      } else if (is_write and !m_config.m_writes_stall_at_mshr) {
         // Skipping MSHRs for writes/atomics
         m_ruby->send_request( block_addr, mf->get_data_size(), core_id, mf->get_wid(), mf->get_pc(), type, false, space, mf->get_byte_mask(), mf);
         sent_to_ruby = true;
         m_mshr[core_id]->add(block_addr, is_write, mf); // still need to tell MSHR about this write
      } else {
         // Normal request
         if ( m_mshr[core_id]->do_access(block_addr, is_write) ) {
            m_ruby->send_request( block_addr, mf->get_data_size(), core_id, mf->get_wid(), mf->get_pc(), type, false, space, mf->get_byte_mask(), mf);
            sent_to_ruby = true;
         }
         m_mshr[core_id]->add(block_addr, is_write, mf);
      }

      // stats
      m_stats.count_memory_access(mf, sent_to_ruby);

      // locality profiler
      if(m_config.m_locality_profiler_enabled)
         m_locality_profiler.profile_access(block_addr,core_id,mf->get_wid(),gpu_sim_cycle+gpu_tot_sim_cycle);
   }
}



bool ruby_wrapper::is_ready( unsigned core_id, mem_fetch * mf )
{
   bool isReady; 
   bool is_write = mf->get_is_write() or mf->isatomic(); // atomic requires modifying the cache line

   unsigned long long line_size = (unsigned long long) m_ruby->get_ruby_block_size();
   unsigned long long block_addr = mf->get_addr() & ~(line_size-1);

   ruby_memory_space_t space = get_ruby_mem_space(mf);
   ruby_memory_type_t type = get_ruby_mem_type(mf);

   if(mf->is_membar()) {
       // For memory barrier, don't need to check MSHRs, just check if Ruby is ready
       isReady =  m_ruby->isReady( block_addr, mf->get_data_size(), core_id, mf->get_wid(), type, false, space);
       return isReady;
   }

   // mem-fetch must fit within the line size
   assert(mf->get_data_size() <= line_size);

   // If we are skipping the L1 cache, then don't need to check MSHRs, but need to check Ruby
   if(m_gpgpu_config.m_shader_config.gmem_skip_L1D && space==ruby_global_space) {
      isReady = m_ruby->isReady( block_addr, mf->get_data_size(), core_id, mf->get_wid(), type, false, space);
      if(!isReady)
         m_stats.m_stall_reasons[ruby_unready]++;
   } else {
      // We are not skipping L1 cache, check MSHRs
      if ( m_mshr[core_id]->probe(block_addr) ) {
         // MSHR hit - check for tracking space
         bool isMSHRReady = (not m_mshr[core_id]->full(block_addr, is_write));
         ruby_stall_reason_t ruby_stall_reason = ruby_no_stall;
         bool isAccessAllowed = m_mshr[core_id]->access_allowed(block_addr, is_write, mf->get_wid(), ruby_stall_reason);

         isReady = isMSHRReady and isAccessAllowed;
         if(!isReady) {
            if(!isMSHRReady)
                m_stats.m_stall_reasons[mshr_hit_full]++;
            else {
                assert(!isAccessAllowed);    // MSHR must be the one blocking us
                assert(ruby_stall_reason != ruby_no_stall);    // MSHR must have given us a reason
                m_stats.m_stall_reasons[ruby_stall_reason]++;
            }
         }

      } else {
         // MSHR miss - check for downstream back pressure from ruby
         bool isRubyReady = m_ruby->isReady( block_addr, mf->get_data_size(), core_id, mf->get_wid(), type, false, space);
         isReady = isRubyReady and (not m_mshr[core_id]->full(block_addr, is_write));

         if(!isReady) {
            if(!isRubyReady)
               m_stats.m_stall_reasons[ruby_unready]++;
            else
               m_stats.m_stall_reasons[mshr_miss_full]++;
         }
      }
   }
   return isReady; 
}

void ruby_wrapper::print_stats() const 
{
   m_stats.print(stdout);
   if(m_config.m_locality_profiler_enabled)
      m_locality_profiler.print_stats(stdout);
   m_ruby->print_stats();
}

void ruby_wrapper::print(FILE *fout, int core_id) const {
   fprintf(fout, "Ruby contents (sid=%d)\n", core_id);
   m_mshr[core_id]->display(fout);
}


void ruby_wrapper::set_benchmark_contains_membar(bool flag) {
    m_ruby->set_benchmark_contains_membar(flag);
}

//////
// Ruby Stats
//

void ruby_stats::count_memory_access( mem_fetch * mf, bool sent_to_ruby )
{
   if(mf->isatomic()) {
      m_global_atomic += 1;
      if(sent_to_ruby) m_mem_global_atomic += 1;
      return;
   }

   switch (mf->get_access_type()) {
   case GLOBAL_ACC_R:
      m_global_load += 1;
      if(sent_to_ruby) m_mem_global_load += 1;
      break;
   case GLOBAL_ACC_W:
      m_global_store += 1;
      if(sent_to_ruby) m_mem_global_store += 1;
      break;
   case LOCAL_ACC_R:
      m_local_load += 1;
      if(sent_to_ruby) m_mem_local_load += 1;
      break;
   case LOCAL_ACC_W:
      m_local_store += 1;
      if(sent_to_ruby) m_mem_local_store += 1;
      break;
   default: break; 
   }
}

void ruby_stats::print(FILE *fout) const
{
   fprintf(fout, "ruby_n_accesses[global_load] = %u\n", m_global_load);
   fprintf(fout, "ruby_n_accesses[global_store] = %u\n", m_global_store);
   fprintf(fout, "ruby_n_accesses[global_atomic] = %u\n", m_global_atomic);
   fprintf(fout, "ruby_n_accesses[local_load] = %u\n", m_local_load);
   fprintf(fout, "ruby_n_accesses[local_store] = %u\n", m_local_store);

   fprintf(fout, "ruby_n_mem_accesses[global_load] = %u\n", m_mem_global_load);
   fprintf(fout, "ruby_n_mem_accesses[global_store] = %u\n", m_mem_global_store);
   fprintf(fout, "ruby_n_mem_accesses[global_atomic] = %u\n", m_mem_global_atomic);
   fprintf(fout, "ruby_n_mem_accesses[local_load] = %u\n", m_mem_local_load);
   fprintf(fout, "ruby_n_mem_accesses[local_store] = %u\n", m_mem_local_store);

   fprintf(fout, "ruby_stall[ruby_unready] = %u\n", m_stall_reasons[ruby_unready]);
   fprintf(fout, "ruby_stall[mshrs_hit_full] = %u\n", m_stall_reasons[mshr_hit_full]);
   fprintf(fout, "ruby_stall[mshrs_miss_full] = %u\n", m_stall_reasons[mshr_miss_full]);
   fprintf(fout, "ruby_stall[mshr_write_to_pending] = %u\n", m_stall_reasons[mshr_write_to_pending]);
   fprintf(fout, "ruby_stall[mshr_same_warp_second_W] = %u\n", m_stall_reasons[mshr_same_warp_second_W]);
   fprintf(fout, "ruby_stall[mshr_same_warp_WR] = %u\n", m_stall_reasons[mshr_same_warp_WR]);
   fprintf(fout, "ruby_stall[mshr_diff_warp_RWR] = %u\n", m_stall_reasons[mshr_diff_warp_RWR]);
}



/////
// Ruby MSHR table

ruby_mshr_table::ruby_mshr_table( unsigned num_entries, unsigned max_merged, const ruby_config& ruby_config )
: m_num_entries(num_entries),
  m_max_merged(max_merged),
  m_config(ruby_config)
{
}

// does an MSHR entry already exist?
bool ruby_mshr_table::probe( ruby_addr_type block_addr ) const
{
    table::const_iterator a = m_data.find(block_addr);
    return a != m_data.end();
}

// should this access be sent to Ruby?
bool ruby_mshr_table::do_access( ruby_addr_type block_addr, bool is_write ) const {
   table::const_iterator a = m_data.find(block_addr);
   // If writes stall at MSHR, then only do the access if an MSHR entry already doesn't exist
   if(m_config.m_writes_stall_at_mshr) {
       return a == m_data.end();
   }
   // If writes are skipping MSHR, only do this read if one hasn't already been done
   else {
      assert(!is_write); // only reads should call this function if writes are skipping MSHRs
      if(a == m_data.end())
         return true;
      else {
         return a->second.m_readset.size()==0;
      }
   }
}

// is there space for tracking a new memory access?
bool ruby_mshr_table::full( ruby_addr_type block_addr, bool is_write ) const 
{ 
    table::const_iterator i=m_data.find(block_addr);
    if ( i != m_data.end() ) {
        const entry& e = i->second; 
        if (e.m_list.size() >= m_max_merged) {
            return true; // entry full, request cannot be merged 
        } else {
            return false; 
        }
    } else {
        return m_data.size() >= m_num_entries;
    }
}

// is this access allowed to go ahead?
bool ruby_mshr_table::access_allowed(ruby_addr_type block_addr, bool is_write, int warp_id, ruby_stall_reason_t& stall_reason) const {
    table::const_iterator i=m_data.find(block_addr);
    assert(i != m_data.end());      // there must be a pending request
    const entry& e = i->second;

    if(m_config.m_single_write_per_warp and e.m_writeset.count(warp_id)>0) {
        stall_reason = mshr_write_to_pending;
        return false;
    }

    if(m_config.m_writes_stall_at_mshr) {
       // Writes are stalled if the first request was a read
       // This is because the protocol only asked for read permissions
       // If this request is a write, check that at least one other write exists in the MSHR (the one that allocated the MSHR)
       // (this function is not called for the request that allocates an MSHR, so that write will always go through)
       if (is_write && e.m_writeset.size()==0) {
          stall_reason = mshr_write_to_pending;
          return false; // silent modify not allowed
       }
       else return true;
    } else {
       // Writes are not stalled by pending requests in the MSHR
       // Check the list of stall conditions

       // 1. If write, the same warp must not have done a read or write before
       //    because there is no point-to-point ordering
       /*
       if(is_write) {
          if(e.m_readset.find(warp_id)!=e.m_readset.end() or e.m_writeset.find(warp_id)!=e.m_writeset.end()) {
             stall_reason = mshr_same_warp_second_W;
             return false;
          }
       }
       */
       // 2. If read, the same warp must not have done a write before because
       //    there is no point-to-point ordering
       /*
       else {
          std::map<int,int>::const_iterator it = e.m_writeset.find(warp_id);
          if(it != e.m_writeset.end()) {
             assert(it->second > 0);
             stall_reason = mshr_same_warp_WR;
             return false;
          }
       }
       */

       // 3. Check for the R0-W1-R1 case (accesses by warps 0 and 1). Here R1 is incorrectly merged with R0.
       //    If a warp is doing a read and it has previously done a write, and the write_after_read flag is set, then stall.
       if(!is_write and e.m_writeset.count(warp_id)>0 and e.m_write_after_read) {
          stall_reason = mshr_diff_warp_RWR;
          return false;
       }

       return true;
    }
}

// add or merge this access
void ruby_mshr_table::add( ruby_addr_type block_addr, bool is_write, mem_fetch *mf )
{
    entry& e = m_data[block_addr];

    if(m_config.m_writes_stall_at_mshr) {
       // Writes are only allowed if they are the first one to allocate the MSHR,
       // or if another write exists (the one that allocated the MSHR)
       if(is_write) {
          assert(
                   (e.m_readset.size() == 0 and e.m_writeset.size() == 0)
                   or
                   (e.m_writeset.size() > 0)
                );
       }
    }

    // Add warp_id to list
    if(is_write)
       e.m_writeset[mf->get_wid()] += 1;
    else
       e.m_readset[mf->get_wid()] += 1;

    // Only push a write mf in if writes are not skipping MSHRs
    // Always push a read mf in
    if(!is_write or m_config.m_writes_stall_at_mshr==true)
       e.m_list.push_back(mf);

    // Keep track of RW to prevent RWR case
    if(is_write and e.m_readset.size()>0)
       e.m_write_after_read = true;

    assert( m_data.size() <= m_num_entries );
    assert( e.m_list.size() <= m_max_merged );
}

// true if cannot accept new fill responses
bool ruby_mshr_table::busy() const 
{ 
    return false;
}

// accept a new cache fill response: mark entry ready for processing
void ruby_mshr_table::mark_ready( ruby_addr_type block_addr )
{
    assert( !busy() );
    table::iterator a = m_data.find(block_addr);
    assert( a != m_data.end() ); // don't remove same request twice
    m_current_response.push_back( block_addr );
    assert( m_current_response.size() <= m_data.size() );
}

// true if ready accesses exist
bool ruby_mshr_table::access_ready() const 
{
    return !m_current_response.empty(); 
}

// next ready access
mem_fetch *ruby_mshr_table::next_access()
{
    assert( access_ready() );
    ruby_addr_type block_addr = m_current_response.front();
    assert( !m_data[block_addr].m_list.empty() );
    mem_fetch *result = m_data[block_addr].m_list.front();
    //printf("NEXT_ACCESS addr=%x core_id=x wid=%d\n", block_addr, result->get_wid());
    m_data[block_addr].m_list.pop_front();


    bool is_write = result->get_is_write() or result->isatomic();

    if(m_config.m_writes_stall_at_mshr) {
       // If writes stall at MSHR, then this access being popped can be either a read or write
       // Pop the correct one
       if(is_write) {
          assert(m_data[block_addr].m_writeset.find(result->get_wid()) != m_data[block_addr].m_writeset.end());   // should be in read set
          assert(m_data[block_addr].m_writeset[result->get_wid()] > 0);
          m_data[block_addr].m_writeset[result->get_wid()] -= 1;
          if(m_data[block_addr].m_writeset[result->get_wid()] == 0)
             m_data[block_addr].m_writeset.erase(result->get_wid());

       } else {
          assert(m_data[block_addr].m_readset.find(result->get_wid()) != m_data[block_addr].m_readset.end());   // should be in read set
          assert(m_data[block_addr].m_readset[result->get_wid()] > 0);
          m_data[block_addr].m_readset[result->get_wid()] -= 1;
          if(m_data[block_addr].m_readset[result->get_wid()] == 0)
             m_data[block_addr].m_readset.erase(result->get_wid());

       }
    } else {
       // If writes don't stall at MSHR, then this access returning to MSHR must be a read
       assert(m_data[block_addr].m_readset.find(result->get_wid()) != m_data[block_addr].m_readset.end());   // should be in read set
       assert(m_data[block_addr].m_readset[result->get_wid()] > 0);
       m_data[block_addr].m_readset[result->get_wid()] -= 1;
       if(m_data[block_addr].m_readset[result->get_wid()] == 0)
          m_data[block_addr].m_readset.erase(result->get_wid());
    }


    if ( m_data[block_addr].m_list.empty() ) {
        assert(m_data[block_addr].m_readset.size() == 0);   // the reads are guaranteed to be done
        // if writes skipped MSHRs, don't release it until all of them have returned
        // otherwise we can always release the MSHR
        if(m_config.m_writes_stall_at_mshr or m_data[block_addr].m_writeset.size()==0) {
           // release entry
           //printf("RELEASE addr=%x core_id=x wid=x\n", block_addr);
           m_data.erase(block_addr);
        }
        m_current_response.pop_front();
    }
    return result;
}

// Mark a write that was skipped done in the MSHR
void ruby_mshr_table::mark_skipped_write_done(ruby_addr_type block_addr, int warp_id)
{
   assert(m_config.m_writes_stall_at_mshr == false);  //  don't call this if writes don't skip MSHRs
   table::iterator a = m_data.find(block_addr);
   assert( a != m_data.end() );                       // request must exist
   entry& e = a->second;
   assert(e.m_writeset.find(warp_id) != e.m_writeset.end());               // we were waiting for this warp's write
   assert(e.m_writeset[warp_id] > 0);
   e.m_writeset[warp_id] -= 1;
   if(e.m_writeset[warp_id] == 0)
      e.m_writeset.erase(warp_id);

   // Delete MSHR if all reads and writes are complete
   if(e.m_list.size()==0 and e.m_writeset.size()==0) {
      //printf("RELEASE addr=%x core_id=x wid=%d\n", block_addr, warp_id);
      assert(e.m_readset.size()==0);
      // release entry
      m_data.erase(block_addr);
   }
}

void ruby_mshr_table::display( FILE *fp ) const
{
    fprintf(fp,"MSHR contents\n");
    fprintf(fp,"%d of %d MSHR entries allocated\n", (int)m_data.size(), m_num_entries);
    for ( table::const_iterator e=m_data.begin(); e!=m_data.end(); ++e ) {
        unsigned block_addr = e->first;
        const entry& en = e->second;
        fprintf(fp,"MSHR: tag=0x%06x, %zu entries : ", block_addr, en.m_list.size());
        if ( !en.m_list.empty() ) {
            mem_fetch *mf = en.m_list.front();
            fprintf(fp,"%p :",mf);
            mf->print(fp);
        }
        fprintf(fp, "readset: ");
        if(!en.m_readset.empty()) {
           for(std::map<int,int>::const_iterator it=en.m_readset.begin();
                 it!=en.m_readset.end();
                 it++)
           {
              fprintf(fp, "%d:%d ", it->first, it->second);
           }
        }
        fprintf(fp, "writeset: ");
        if(!en.m_writeset.empty()) {
           for(std::map<int,int>::const_iterator it=en.m_writeset.begin();
                 it!=en.m_writeset.end();
                 it++)
           {
              fprintf(fp, "%d:%d ", it->first, it->second);
           }
        }
        fprintf(fp,"\n");
    }
}


// Initialize locality profiler
ruby_locality_profiler::ruby_locality_profiler( unsigned int cores, unsigned int warps_per_core, const ruby_config& config )
   : m_n_cores(cores), m_n_warps_per_core(warps_per_core) {
   if(!config.m_locality_profiler_enabled)
      return;

   // Initialize the queues and periods in the profiler
   unsigned bin = 1 << config.m_locality_profiler_start_bin;
   unsigned old_bin = 0;
   m_num_periods = 0;
   while(bin <= (1<<config.m_locality_profiler_end_bin)) {
      m_num_periods++;
      m_period_sizes.push_back(bin-old_bin);
      old_bin = bin;
      bin = bin << config.m_locality_profiler_skip_bin;
   }

   m_queues.resize(m_num_periods);
   m_periods.resize(m_num_periods);
   m_stats_locality_breakdown.resize(m_num_periods+1);      // extra one for misses
   for(unsigned i=0; i<m_num_periods+1; i++) {
      m_stats_locality_breakdown[i].resize(n_LB);
   }
}

// Profile the acess in locality profiler
void ruby_locality_profiler::profile_access(ruby_addr_type addr, unsigned core_id, unsigned warp_id, unsigned long long cycle) {
   // Convert per core warp id into global warp id
   unsigned g_warp_id = core_id*m_n_warps_per_core + warp_id;

   // Let's check if the addr is in the profiler
   bool found_addr = false;
   for(unsigned p=0; p<m_num_periods; p++) {
      ruby_profiler_period_t::iterator period_it = m_periods[p].find(addr);
      if(period_it != m_periods[p].end()) {
         // Found it in p'th set
         // Count the stats by checking in order for warp, core, system
         if(period_it->second.m_accessed_warps.find(g_warp_id) != period_it->second.m_accessed_warps.end()) {
            m_stats_locality_breakdown[p][LB_WARP]++;
         } else if(period_it->second.m_accessed_SMs.find(core_id) != period_it->second.m_accessed_SMs.end()) {
            m_stats_locality_breakdown[p][LB_CORE]++;
         } else {
            m_stats_locality_breakdown[p][LB_SYSTEM]++;
         }
         found_addr = true;
         break;
      }
   }
   if(!found_addr)
      m_stats_locality_breakdown[m_num_periods][LB_SYSTEM]++;     // is a miss in the profiler

   // Now add the access to the profiler
   add_access(addr, core_id, g_warp_id, cycle);
}

// Add the access to the internal structures in profiler
void ruby_locality_profiler::add_access(ruby_addr_type addr, unsigned core_id, unsigned warp_id, unsigned long long cycle) {
   // We always add accesses to the first queue / period
   m_queues[0].push( ruby_profiler_access_stats(addr, cycle+m_period_sizes[0]) );
   m_periods[0][addr].m_pending_accesses += 1;
   m_periods[0][addr].m_accessed_SMs.insert(core_id);
   m_periods[0][addr].m_accessed_warps.insert(warp_id);
}

// Advance one cycle in the profiler
void ruby_locality_profiler::advance_time(unsigned long long cycle) {
   // Pop from the end of each queue, and insert into next queue if necessary
   for(unsigned p=0; p<m_num_periods; p++) {
      while(m_queues[p].size()>0 and m_queues[p].front().m_pop_cycle <= cycle) {
         // Need to pop this access
         ruby_addr_type addr = m_queues[p].front().m_addr;
         // Update the set first
         assert( m_periods[p][addr].m_pending_accesses > 0 );
         // We have other accesses to this addr in flight, keep it
         if(m_periods[p][addr].m_pending_accesses > 1) {
            // Keep it in this period
            m_periods[p][addr].m_pending_accesses--;
         } else {
            // This addr needs to be moved to the next period, if one exists
            if(p<m_num_periods-1) {
               m_periods[p+1][addr].m_pending_accesses++;
               m_periods[p+1][addr].m_accessed_SMs.insert(m_periods[p][addr].m_accessed_SMs.begin(),m_periods[p][addr].m_accessed_SMs.end());
               m_periods[p+1][addr].m_accessed_warps.insert(m_periods[p][addr].m_accessed_warps.begin(), m_periods[p][addr].m_accessed_warps.end());

               // Insert access into the next queue as well
               m_queues[p+1].push(ruby_profiler_access_stats(addr, m_queues[p].front().m_pop_cycle+m_period_sizes[p+1]));
            }
            m_periods[p].erase(addr);
         }
         // Pop this access from the queue
         m_queues[p].pop();
      }
   }
}

void ruby_locality_profiler::print_stats(FILE *fout) const {
   unsigned old_period_size = 0;
   for(unsigned p=0; p<m_num_periods; p++) {
      old_period_size += m_period_sizes[p];
      fprintf(fout, "locality_profiler[%d][LB_WARP] = %llu\n", old_period_size, m_stats_locality_breakdown[p][LB_WARP]);
      fprintf(fout, "locality_profiler[%d][LB_CORE] = %llu\n", old_period_size, m_stats_locality_breakdown[p][LB_CORE]);
      fprintf(fout, "locality_profiler[%d][LB_SYSTEM] = %llu\n", old_period_size, m_stats_locality_breakdown[p][LB_SYSTEM]);
   }
   fprintf(fout, "locality_profiler[misses] = %llu\n", m_stats_locality_breakdown[m_num_periods][LB_SYSTEM]);
}
