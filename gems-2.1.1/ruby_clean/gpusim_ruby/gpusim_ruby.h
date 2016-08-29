#ifndef GPUSIM_RUBY_H
#define GPUSIM_RUBY_H

#include <stdio.h>
#include "Global.h"

// Forward declarations
class GpusimInterface;
class gpgpu_sim_config;

// These enums are used to communicate between GPGPU-Sim and Ruby
// The interface to Ruby (gpusim_ruby.C) understands these and converts them to appropriate Ruby types
enum ruby_memory_space_t{
   ruby_global_space,
   ruby_local_space,
   ruby_memory_space_num,
};

enum ruby_memory_type_t{
    ruby_load_type,
    ruby_store_type,
    ruby_membar_type,
    ruby_atomic_type,
    ruby_memory_type_num,
};

struct ruby_dram_req_info {
   bool m_write;
   void* m_msg_ptr;
   unsigned long long m_addr;
   unsigned m_size;
};

struct gpusim_ruby_config {
    double m_core_freq;
    double m_icnt_freq;
    unsigned m_num_processors;
    unsigned m_num_procs_per_cluster;
    unsigned m_num_warps_per_processor;
    unsigned m_num_L2_banks;
    unsigned m_num_memories;
    bool m_CMP;
    bool m_garnet;
    unsigned m_debug_start_time;
    char* m_debug_filter_string;
    char* m_debug_verbosity_string;
    char* m_debug_output_filename;
    unsigned m_num_TBE;
    unsigned m_num_seq_entries;
    unsigned m_flit_size;
    unsigned m_vns;
    unsigned m_vcs_per_class;
    unsigned m_vc_buffer_size;
    unsigned m_latency_L1toL2;
    unsigned m_latency_L2toMem;
    float m_latency_L1toL2_multiplier;
    float m_latency_L2toMem_multiplier;
    bool m_gmem_skip_L1D;
    bool m_writes_stall_at_mshr;
    unsigned m_cl_fixed_lease;
    unsigned m_cl_predictor_type;
    unsigned m_cl_pred_global_writeunexpired;
    unsigned m_cl_pred_global_l2evict;
    unsigned m_cl_pred_global_expiredhit;
    unsigned m_cl_pred_perL2_writeunexpired;
    unsigned m_cl_pred_perL2_l2evict;
    unsigned m_cl_pred_perL2_expiredhit;
    unsigned m_cl_pred_pcadaptive_writeunexpired;
    unsigned m_cl_pred_pcadaptive_l2evict;
    unsigned m_cl_pred_pcadaptive_expiredhit;
    char* m_cl_pred_pcfixed_infile;
    char* m_cl_pred_addrfixed_infile;
    unsigned m_lifetime_profiler_perpc_outmode;
    char* m_lifetime_profiler_perpc_outfile;
    unsigned m_lifetime_profiler_peraddr_outmode;
    char* m_lifetime_profiler_peraddr_outfile;
    bool m_lifetimetrace_enabled;
    char * m_lifetimetrace_outfile;
    unsigned m_l1_t_per_cycle;
    unsigned m_l2_t_per_cycle;
    unsigned m_dir_t_per_cycle;
    unsigned m_l1_assoc;
    unsigned m_l2_assoc;
    unsigned m_dir_assoc;
    unsigned m_l1_sets_bits;
    unsigned m_l2_sets_bits;
    unsigned m_dir_sets_bits;
    unsigned m_deadlock_threshold;

    gpusim_ruby_config(const gpgpu_sim_config &config);
};

class gpusim_ruby {
public:
   gpusim_ruby( gpusim_ruby_config R_config);
   ~gpusim_ruby();

   int isReady( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, 
                ruby_memory_type_t type, bool isPriv, ruby_memory_space_t space);
   void send_request( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                      ruby_memory_type_t type, bool isPriv, ruby_memory_space_t space, ruby_mem_access_byte_mask_t access_mask,
                      void* mf);
   void send_memfence_request( unsigned long long addr, unsigned req_size, unsigned sid, unsigned tid, unsigned pc,
                      ruby_memory_type_t type, bool isPriv, ruby_memory_space_t space, ruby_mem_access_byte_mask_t access_mask);
   void advance_time();

   bool callbackQueueEmpty(unsigned core_id);
   unsigned long long callbackQueueTop(unsigned core_id);
   void callbackQueuePop(unsigned core_id);

   bool memfenceCallbackQueueEmpty(unsigned core_id);
   unsigned long long memfenceCallbackQueueTop(unsigned core_id);
   void memfenceCallbackQueuePop(unsigned core_id);

   bool skipL1CallbackQueueEmpty(unsigned core_id);
   void skipL1CallbackQueueTop(unsigned core_id, unsigned long long& addr, void** memfetch);
   void skipL1CallbackQueuePop(unsigned core_id);

   // Interface with GPGPU-Sim's DRAM
   bool ToGpusimDram_empty(unsigned partition_id);
   ruby_dram_req_info ToGpusimDram_top(unsigned partition_id);
   void ToGpusimDram_pop(unsigned partition_id);

   void FromGpusimDram_push(unsigned partition_id, ruby_dram_req_info dram_req);

   // Flush all L1 caches in Ruby
   void flushAllL1DCaches();

   void print_stats(); 

   void initialize();
   void destroy();


   unsigned get_ruby_block_size();
   void set_benchmark_contains_membar(bool flag);
private:

   // Members
   char * my_default_param;
   class initvar_t * my_initvar;
   GpusimInterface * m_driver_ptr; 

   // Options passed in
   gpusim_ruby_config m_config;
};

#endif //GPUSIM_RUBY_H
