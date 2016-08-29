// Copyright (c) 2009-2011, Tor M. Aamodt, Wilson W.L. Fung
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GPU_SIM_H
#define GPU_SIM_H

#include "../option_parser.h"
#include "../abstract_hardware_model.h"
#include "gpu-config.h"
#include "addrdec.h"
#include "shader.h"
#include "ruby_wrapper.h"

#include <list>
#include <stdio.h>

// constants for statistics printouts
#define GPU_RSTAT_SHD_INFO 0x1
#define GPU_RSTAT_BW_STAT  0x2
#define GPU_RSTAT_WARP_DIS 0x4
#define GPU_RSTAT_DWF_MAP  0x8
#define GPU_RSTAT_L1MISS 0x10
#define GPU_RSTAT_PDOM 0x20
#define GPU_RSTAT_SCHED 0x40
#define GPU_MEMLATSTAT_MC 0x2

// constants for configuring merging of coalesced scatter-gather requests
#define TEX_MSHR_MERGE 0x4
#define CONST_MSHR_MERGE 0x2
#define GLOBAL_MSHR_MERGE 0x1

// clock constants
#define MhZ *1000000

#define CREATELOG 111
#define SAMPLELOG 222
#define DUMPLOG 333

// global counters and flags (please try not to add to this list!!!)
extern unsigned long long  gpu_sim_cycle;
extern unsigned long long  gpu_tot_sim_cycle;
extern bool g_interactive_debugger_enabled;


class gpgpu_sim : public gpgpu_t {
public:
   gpgpu_sim( const gpgpu_sim_config &config );

   void set_prop( struct cudaDeviceProp *prop );

   void launch( kernel_info_t *kinfo );
   bool can_start_kernel();
   unsigned finished_kernel();
   void set_kernel_done( kernel_info_t *kernel );

   void init();
   void cycle();
   bool active(); 
   void print_stats();
   void update_stats();
   void deadlock_check();

   int shared_mem_size() const;
   int num_registers_per_core() const;
   int wrp_size() const;
   int shader_clock() const;
   const struct cudaDeviceProp *get_prop() const;
   enum divergence_support_t simd_model() const; 

   unsigned threads_per_core() const;
   bool get_more_cta_left() const;
   kernel_info_t *select_kernel();

   const gpgpu_sim_config &get_config() const { return m_config; }
   void gpu_print_stat() const;
   void dump_pipeline( int mask, int s, int m ) const;

   //The next three functions added to be used by the functional simulation function
   
   //! Get shader core configuration
   /*!
    * Returning the configuration of the shader core, used by the functional simulation only so far
    */
   const struct shader_core_config * getShaderCoreConfig();
   
   
   //! Get shader core Memory Configuration
    /*!
    * Returning the memory configuration of the shader core, used by the functional simulation only so far
    */
   const struct memory_config * getMemoryConfig();
   
   
   //! Get shader core SIMT cluster
   /*!
    * Returning the cluster of of the shader core, used by the functional simulation so far
    */
    simt_core_cluster * getSIMTCluster();

   ruby_wrapper &get_ruby_wrapper() { return m_ruby_wrapper; }

private:
   // clocks
   void reinit_clock_domains(void);
   int  next_clock_domain(void);
   void issue_block2core();
    
   void L2c_print_cache_stat() const;
   void shader_print_runtime_stat( FILE *fout );
   void shader_print_l1_miss_stat( FILE *fout ) const;
   void visualizer_printstat();
   void print_shader_cycle_distro( FILE *fout ) const;

   void gpgpu_debug();

///// data /////

   class simt_core_cluster **m_cluster;
   class memory_partition_unit **m_memory_partition_unit;

   std::vector<kernel_info_t*> m_running_kernels;
   unsigned m_last_issued_kernel;

   std::list<unsigned> m_finished_kernel;
   unsigned m_total_cta_launched;
   unsigned m_last_cluster_issue;

   // time of next rising edge 
   double core_time;
   double icnt_time;
   double dram_time;
   double l2_time;

   // debug
   bool gpu_deadlock;

   //// configuration parameters ////
   const gpgpu_sim_config &m_config;
  
   const struct cudaDeviceProp     *m_cuda_properties;
   const struct shader_core_config *m_shader_config;
   const struct memory_config      *m_memory_config;

   // stats
   class shader_core_stats  *m_shader_stats;
   class memory_stats_t     *m_memory_stats;
   unsigned long long  gpu_tot_issued_cta;
   unsigned long long  last_gpu_sim_insn;

   // Ruby wrapper
   ruby_wrapper m_ruby_wrapper;

public:
   unsigned long long  gpu_sim_insn;
   unsigned long long  gpu_tot_sim_insn;
   unsigned long long  gpu_sim_insn_last_update;
   unsigned gpu_sim_insn_last_update_sid;
};

#endif
