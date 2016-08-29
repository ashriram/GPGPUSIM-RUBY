#ifndef RUBY_WRAPPER_H_
#define RUBY_WRAPPER_H_

#include <stdio.h>
#include <assert.h>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <set>
#include "gpu-config.h"

extern unsigned long long  gpu_sim_cycle;
extern unsigned long long  gpu_tot_sim_cycle;

class gpusim_ruby;
class mem_fetch;
class gpgpu_sim_config;


typedef unsigned long long ruby_addr_type;




// Locality profiler
// Struct to keep track for per addr stats in the locality profiler
struct ruby_profiler_addr_stats {
   std::set<unsigned int> m_accessed_warps;     // which warps have accessed this address?
   std::set<unsigned int> m_accessed_SMs;       // which cores have accessed this address?
   unsigned m_pending_accesses;                 // accesses pending in the queue

   ruby_profiler_addr_stats() : m_pending_accesses(0) {}

   void clear() {
      m_accessed_warps.clear();
      m_accessed_SMs.clear();
   }
};

// Struct to keep track of accesses. This is inserted into fixed latency queues
struct ruby_profiler_access_stats {
   ruby_addr_type m_addr;
   unsigned long long m_pop_cycle;        // cycle to pop this request from fixed latency queue

   ruby_profiler_access_stats(ruby_addr_type addr, unsigned long long pop_cycle) : m_addr(addr), m_pop_cycle(pop_cycle) { }
};

enum locality_breakdown_t {
   LB_WARP,
   LB_CORE,
   LB_SYSTEM,
   n_LB
};

class ruby_locality_profiler {
public:
   ruby_locality_profiler ( unsigned int cores, unsigned int warps_per_core, const ruby_config& config );

   void profile_access(ruby_addr_type addr, unsigned core_id, unsigned warp_id, unsigned long long cycle);
   void advance_time(unsigned long long cycle);
   void print_stats(FILE *fout) const;
private:

   void add_access(ruby_addr_type addr, unsigned core_id, unsigned warp_id, unsigned long long cycle);

   unsigned m_num_periods;

   typedef std::map<ruby_addr_type, ruby_profiler_addr_stats> ruby_profiler_period_t;        // set of addresses within one locality period
   std::vector<ruby_profiler_period_t> m_periods;                                            // vector of locality periods

   typedef std::queue<ruby_profiler_access_stats> ruby_profiler_queue_t;                     // fixed latency queue for each locality period
   std::vector<ruby_profiler_queue_t> m_queues;                                              // vector of queues, one queue per locality period

   std::vector<unsigned int> m_period_sizes;                                                 // sizes of periods in SIMT cycles

   unsigned int m_n_cores;
   unsigned int m_n_warps_per_core;

   typedef std::vector<unsigned long long> stats_locality_breakdown_t;    // per period locality breakdown
   std::vector<stats_locality_breakdown_t> m_stats_locality_breakdown;                       // locality breakdown stats for all periods
};

enum ruby_stall_reason_t {
   ruby_no_stall,
   ruby_unready,
   mshr_hit_full,
   mshr_miss_full,
   mshr_write_to_pending,
   mshr_same_warp_second_W,
   mshr_same_warp_WR,
   mshr_diff_warp_RWR,
   n_ruby_stall_reasons
};

// Implement MSHR for handling LD/ST requesting the same cache block
// - directly copied from gpu-cache 
class ruby_mshr_table {
public:
    ruby_mshr_table( unsigned num_entries, unsigned max_merged, const ruby_config& ruby_config );

    // does an MSHR entry already exist?
    bool probe( ruby_addr_type block_addr ) const;

    // should this access be sent to Ruby?
    bool do_access( ruby_addr_type block_addr, bool is_write ) const;

    // is there space for tracking a new memory access?
    bool full( ruby_addr_type block_addr, bool is_write ) const;

    // is this access allowed to be merged into MSHRs?
    bool access_allowed(ruby_addr_type block_addr, bool is_write, int warp_id, ruby_stall_reason_t& stall_reason) const;

    // add or merge this access
    void add( ruby_addr_type block_addr, bool is_write, mem_fetch *mf ); 

    // true if cannot accept new fill responses
    bool busy() const; 

    // accept a new cache fill response: mark entry ready for processing
    void mark_ready( ruby_addr_type block_addr ); 

    // true if ready accesses exist
    bool access_ready() const;

    // next ready access
    mem_fetch *next_access();

    void mark_skipped_write_done(ruby_addr_type block_addr, int warp_id);

    void display( FILE *fp ) const;

private:

    // finite sized, fully associative table, with a finite maximum number of merged requests
    const unsigned m_num_entries;
    const unsigned m_max_merged;
    const ruby_config& m_config;

    typedef std::list<mem_fetch*> mf_list;
    struct entry {
        std::map<int,int> m_readset;   // map wid -> pending reads
        std::map<int,int> m_writeset;  // map wid -> pending writes
        bool m_write_after_read;    // has there been a write on a pending read?
        mf_list m_list; 
        entry() : m_write_after_read(false) {}
    }; 
    typedef std::map<ruby_addr_type,entry> table;
    table m_data;

    // it may take several cycles to process the merged requests
    bool m_current_response_ready;
    std::list<ruby_addr_type> m_current_response;
};

class ruby_stats {
public:
   // Accesses received by ruby interface
   unsigned m_global_load;
   unsigned m_global_store;
   unsigned m_global_atomic;
   unsigned m_local_load;
   unsigned m_local_store;

   // Accesses sent to ruby by ruby interface
   // Note, these include L1 hits, whereas the equivalent GPGPU-Sim ones don't
   unsigned m_mem_global_load;
   unsigned m_mem_global_store;
   unsigned m_mem_global_atomic;
   unsigned m_mem_local_load;
   unsigned m_mem_local_store;

   unsigned m_stall_mshrs_miss_full;

   unsigned m_stall_reasons[n_ruby_stall_reasons];

   ruby_stats() {
      m_global_load = 0;
      m_global_store = 0;
      m_global_atomic = 0;
      m_local_load = 0;
      m_local_store = 0;
      m_mem_global_load = 0;
      m_mem_global_store = 0;
      m_mem_global_atomic = 0;
      m_mem_local_load = 0;
      m_mem_local_store = 0;

      for(int i=0; i<n_ruby_stall_reasons; i++)
         m_stall_reasons[i] = 0;
   }

   void count_memory_access( mem_fetch * mf, bool sent_to_ruby );

   void print(FILE *fout) const; 
};


// This is the true interface code between ruby and gpgpusim. 
// It is supposed to do: 
// 1. Initialize ruby via gpusim_ruby
// 2. Implement MSHR for handling LD/ST requesting the same cache block
// 3. Handle any back-logging of reply back to the core
class ruby_wrapper {
public:
   ruby_wrapper(const gpgpu_sim_config &g_config);
   ~ruby_wrapper();

   bool is_ready( unsigned core_id, mem_fetch * mf );
   void send_request( unsigned core_id, mem_fetch * mf );
   void advance_time();

   ruby_mshr_table * get_mshr( unsigned core_id ) {
      assert(core_id < m_mshr.size()); 
      return m_mshr[core_id]; 
   }

   // Memory fence callback queue interface
   bool memfenceCallbackQueueEmpty(unsigned core_id);
   unsigned long long memfenceCallbackQueueTop(unsigned core_id);
   void memfenceCallbackQueuePop(unsigned core_id);

   // Skip L1  callback queue
   bool skipL1CallbackQueueEmpty(unsigned core_id);
   mem_fetch* skipL1CallbackQueueTop(unsigned core_id);
   void skipL1CallbackQueuePop(unsigned core_id);

   // Tell MSHRs about writes that were skipped but are done now
   void mshr_mark_skipped_write_done(int core_id, mem_fetch* mf);

   // Interface with GPGPU-Sim's DRAM
   bool ToGpusimDram_empty(unsigned partition_id);
   mem_fetch* ToGpusimDram_top(unsigned partition_id);
   void ToGpusimDram_pop(unsigned partition_id);

   void FromGpusimDram_push(unsigned partition_id, mem_fetch* mf);

   // Flush all L1 caches in Ruby
   void flushAllL1DCaches();

   void set_benchmark_contains_membar(bool flag);


   void print_stats() const;
   void print(FILE *fout, int core_id) const;

private:

   // pointer to ruby
   gpusim_ruby *m_ruby;

   unsigned m_num_cores;

   // MSHR structure to merge multiple reads to the same cache line
   // one for each core 
   std::vector<ruby_mshr_table *> m_mshr;

   // statistics 
   ruby_stats m_stats;

   // profiler
   ruby_locality_profiler m_locality_profiler;

   const ruby_config& m_config;
   const gpgpu_sim_config &m_gpgpu_config;
};

#endif /* RUBY_WRAPPER_H_ */
