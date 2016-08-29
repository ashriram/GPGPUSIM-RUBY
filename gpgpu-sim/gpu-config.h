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

#ifndef GPU_CONFIG_H
#define GPU_CONFIG_H

#include "../abstract_hardware_model.h"
#include "addrdec.h"
#include "gpu-misc.h"

enum replacement_policy_t {
    LRU,
    FIFO
};

enum write_policy_t {
    READ_ONLY,
    WRITE_BACK,
    WRITE_THROUGH,
    WRITE_EVICT,
    LOCAL_WB_GLOBAL_WT
};

enum allocation_policy_t {
    ON_MISS,
    ON_FILL
};


enum write_allocate_policy_t {
    NO_WRITE_ALLOCATE,
    WRITE_ALLOCATE
};

enum mshr_config_t {
    TEX_FIFO,
    ASSOC // normal cache
};


class cache_config {
public:
    cache_config()
    {
        m_valid = false;
        m_disabled = false;
        m_config_string = NULL; // set by option parser
    }
    void init()
    {
        assert( m_config_string );
        char rp, wp, ap, mshr_type, wap;

        int ntok = sscanf(m_config_string,"%u:%u:%u,%c:%c:%c:%c,%c:%u:%u,%u:%u",
                          &m_nset, &m_line_sz, &m_assoc, &rp, &wp, &ap, &wap,
                          &mshr_type, &m_mshr_entries,&m_mshr_max_merge,
                          &m_miss_queue_size,&m_result_fifo_entries);

        if ( ntok < 10 ) {
            if ( !strcmp(m_config_string,"none") ) {
                m_disabled = true;
                return;
            }
            exit_parse_error();
        }
        switch (rp) {
        case 'L': m_replacement_policy = LRU; break;
        case 'F': m_replacement_policy = FIFO; break;
        default: exit_parse_error();
        }
        switch (wp) {
        case 'R': m_write_policy = READ_ONLY; break;
        case 'B': m_write_policy = WRITE_BACK; break;
        case 'T': m_write_policy = WRITE_THROUGH; break;
        case 'E': m_write_policy = WRITE_EVICT; break;
        case 'L': m_write_policy = LOCAL_WB_GLOBAL_WT; break;
        default: exit_parse_error();
        }
        switch (ap) {
        case 'm': m_alloc_policy = ON_MISS; break;
        case 'f': m_alloc_policy = ON_FILL; break;
        default: exit_parse_error();
        }
        switch (mshr_type) {
        case 'F': m_mshr_type = TEX_FIFO; assert(ntok==12); break;
        case 'A': m_mshr_type = ASSOC; break;
        default: exit_parse_error();
        }
        m_line_sz_log2 = LOGB2(m_line_sz);
        m_nset_log2 = LOGB2(m_nset);
        m_valid = true;

        switch(wap){
        case 'W': m_write_alloc_policy = WRITE_ALLOCATE; break;
        case 'N': m_write_alloc_policy = NO_WRITE_ALLOCATE; break;
        default: exit_parse_error();
        }
    }
    bool disabled() const { return m_disabled;}
    unsigned get_line_sz() const
    {
        assert( m_valid );
        return m_line_sz;
    }
    unsigned get_num_lines() const
    {
        assert( m_valid );
        return m_nset * m_assoc;
    }

    void print( FILE *fp ) const
    {
        fprintf( fp, "Size = %d B (%d Set x %d-way x %d byte line)\n",
                 m_line_sz * m_nset * m_assoc,
                 m_nset, m_assoc, m_line_sz );
    }

    unsigned set_index( new_addr_type addr ) const
    {
        return(addr >> m_line_sz_log2) & (m_nset-1);
    }
    new_addr_type tag( new_addr_type addr ) const
    {
        return addr >> (m_line_sz_log2+m_nset_log2);
    }
    new_addr_type block_addr( new_addr_type addr ) const
    {
        return addr & ~(m_line_sz-1);
    }

    char *m_config_string;

private:
    void exit_parse_error()
    {
        printf("GPGPU-Sim uArch: cache configuration parsing error (%s)\n", m_config_string );
        abort();
    }

    bool m_valid;
    bool m_disabled;
    unsigned m_line_sz;
    unsigned m_line_sz_log2;
    unsigned m_nset;
    unsigned m_nset_log2;
    unsigned m_assoc;

    enum replacement_policy_t m_replacement_policy; // 'L' = LRU, 'F' = FIFO
    enum write_policy_t m_write_policy;             // 'T' = write through, 'B' = write back, 'R' = read only
    enum allocation_policy_t m_alloc_policy;        // 'm' = allocate on miss, 'f' = allocate on fill
    enum mshr_config_t m_mshr_type;

    write_allocate_policy_t m_write_alloc_policy;   // 'W' = Write allocate, 'N' = No write allocate

    union {
        unsigned m_mshr_entries;
        unsigned m_fragment_fifo_entries;
    };
    union {
        unsigned m_mshr_max_merge;
        unsigned m_request_fifo_entries;
    };
    union {
        unsigned m_miss_queue_size;
        unsigned m_rob_entries;
    };
    unsigned m_result_fifo_entries;

    friend class tag_array;
    friend class baseline_cache;
    friend class read_only_cache;
    friend class tex_cache;
    friend class data_cache;
    friend class l1_cache;
    friend class l2_cache;
};


class ruby_config {
public:
   bool m_is_CMP;
   bool m_garnet;
   unsigned m_debug_start_time;
   char *m_debug_filter_string;
   char *m_debug_verbosity_string;
   char *m_debug_output_filename;

   unsigned m_num_TBE;
   unsigned m_num_seq_entries;
   unsigned m_mshr_count;
   unsigned m_mshr_size;

   unsigned m_l1_t_per_cycle;
   unsigned m_l2_t_per_cycle;
   unsigned m_dir_t_per_cycle;

   unsigned m_flit_size;
   unsigned m_vns;
   unsigned m_vcs_per_class;
   unsigned m_vc_buffer_size;

   char* m_latencies;
   char* m_latencies_multiplier;
   unsigned m_latency_L1toL2;
   unsigned m_latency_L2toMem;
   float m_latency_L1toL2_multiplier;
   float m_latency_L2toMem_multiplier;

   unsigned m_cl_fixed_lease;
   unsigned m_cl_predictor_type;
   char* m_cl_pred_global_string;
   char* m_cl_pred_perL2_string;
   char* m_cl_pred_pcfixed_string;
   char* m_cl_pred_pcadaptive_string;
   char* m_cl_pred_addrfixed_string;

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
   char *m_lifetime_profiler_perpc_outfile;

   unsigned m_lifetime_profiler_peraddr_outmode;
   char *m_lifetime_profiler_peraddr_outfile;

   bool m_lifetimetrace_enabled;
   char * m_lifetimetrace_outfile;

   bool m_flush_L1D;

   unsigned m_l1_assoc;
   unsigned m_l2_assoc;
   unsigned m_dir_assoc;
   unsigned m_l1_sets_bits;
   unsigned m_l2_sets_bits;
   unsigned m_dir_sets_bits;

   bool m_send_memfence_to_protocol;
   bool m_writes_stall_at_mshr;
   bool m_single_write_per_warp;

   char * m_locality_profiler_string;
   bool m_locality_profiler_enabled;
   unsigned m_locality_profiler_start_bin;
   unsigned m_locality_profiler_end_bin;
   unsigned m_locality_profiler_skip_bin;

   void reg_options(class OptionParser * opp);
   void init();
};


enum dram_ctrl_t {
   DRAM_FIFO=0,
   DRAM_FRFCFS=1
};

struct memory_config {
   memory_config()
   {
       m_valid = false;
       gpgpu_dram_timing_opt=NULL;
       gpgpu_L2_queue_config=NULL;
   }
   void init()
   {
      assert(gpgpu_dram_timing_opt);
      if (strchr(gpgpu_dram_timing_opt, '=') == NULL) {
         // dram timing option in ordered variables (legacy)
         // Disabling bank groups if their values are not specified
         nbkgrp = 1;
         tCCDL = 0;
         tRTPL = 0;
         sscanf(gpgpu_dram_timing_opt,"%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
                &nbk,&tCCD,&tRRD,&tRCD,&tRAS,&tRP,&tRC,&CL,&WL,&tCDLR,&tWR,&nbkgrp,&tCCDL,&tRTPL);
      } else {
         // named dram timing options (unordered)
         option_parser_t dram_opp = option_parser_create();

         option_parser_register(dram_opp, "nbk",  OPT_UINT32, &nbk,   "number of banks", "");
         option_parser_register(dram_opp, "CCD",  OPT_UINT32, &tCCD,  "column to column delay", "");
         option_parser_register(dram_opp, "RRD",  OPT_UINT32, &tRRD,  "minimal delay between activation of rows in different banks", "");
         option_parser_register(dram_opp, "RCD",  OPT_UINT32, &tRCD,  "row to column delay", "");
         option_parser_register(dram_opp, "RAS",  OPT_UINT32, &tRAS,  "time needed to activate row", "");
         option_parser_register(dram_opp, "RP",   OPT_UINT32, &tRP,   "time needed to precharge (deactivate) row", "");
         option_parser_register(dram_opp, "RC",   OPT_UINT32, &tRC,   "row cycle time", "");
         option_parser_register(dram_opp, "CDLR", OPT_UINT32, &tCDLR, "switching from write to read (changes tWTR)", "");
         option_parser_register(dram_opp, "WR",   OPT_UINT32, &tWR,   "last data-in to row precharge", "");

         option_parser_register(dram_opp, "CL", OPT_UINT32, &CL, "CAS latency", "");
         option_parser_register(dram_opp, "WL", OPT_UINT32, &WL, "Write latency", "");

         //Disabling bank groups if their values are not specified
         option_parser_register(dram_opp, "nbkgrp", OPT_UINT32, &nbkgrp, "number of bank groups", "1");
         option_parser_register(dram_opp, "CCDL",   OPT_UINT32, &tCCDL,  "column to column delay between accesses to different bank groups", "0");
         option_parser_register(dram_opp, "RTPL",   OPT_UINT32, &tRTPL,  "read to precharge delay between accesses to different bank groups", "0");

         option_parser_delimited_string(dram_opp, gpgpu_dram_timing_opt, "=:;");
         fprintf(stdout, "DRAM Timing Options:\n");
         option_parser_print(dram_opp, stdout);
         option_parser_destroy(dram_opp);
      }

        int nbkt = nbk/nbkgrp;
        unsigned i;
        for (i=0; nbkt>0; i++) {
            nbkt = nbkt>>1;
        }
        bk_tag_length = i;
      assert(nbkgrp>0 && "Number of bank groups cannot be zero");
      tRCDWR = tRCD-(WL+1);
      tRTW = (CL+(BL/data_command_freq_ratio)+2-WL);
      tWTR = (WL+(BL/data_command_freq_ratio)+tCDLR);
      tWTP = (WL+(BL/data_command_freq_ratio)+tWR);
      dram_atom_size = BL * busW * gpu_n_mem_per_ctrlr; // burst length x bus width x # chips per partition
      m_address_mapping.init(m_n_mem);
      m_L2_config.init();
      m_valid = true;
      m_ruby_config.init();
   }
   void reg_options(class OptionParser * opp);

   bool m_valid;
   cache_config m_L2_config;
   bool m_L2_texure_only;

   char *gpgpu_dram_timing_opt;
   char *gpgpu_L2_queue_config;
   bool l2_ideal;
   unsigned gpgpu_dram_sched_queue_size;
   enum dram_ctrl_t scheduler_type;
   bool gpgpu_memlatency_stat;
   unsigned m_n_mem;
   unsigned gpu_n_mem_per_ctrlr;

   unsigned rop_latency;
   unsigned dram_latency;

   // DRAM parameters

   unsigned tCCDL;  //column to column delay when bank groups are enabled
   unsigned tRTPL;  //read to precharge delay when bank groups are enabled for GDDR5 this is identical to RTPS, if for other DRAM this is different, you will need to split them in two

   unsigned tCCD;   //column to column delay
   unsigned tRRD;   //minimal time required between activation of rows in different banks
   unsigned tRCD;   //row to column delay - time required to activate a row before a read
   unsigned tRCDWR; //row to column delay for a write command
   unsigned tRAS;   //time needed to activate row
   unsigned tRP;    //row precharge ie. deactivate row
   unsigned tRC;    //row cycle time ie. precharge current, then activate different row
   unsigned tCDLR;  //Last data-in to Read command (switching from write to read)
   unsigned tWR;    //Last data-in to Row precharge

   unsigned CL;     //CAS latency
   unsigned WL;     //WRITE latency
   unsigned BL;     //Burst Length in bytes (4 in GDDR3, 8 in GDDR5)
   unsigned tRTW;   //time to switch from read to write
   unsigned tWTR;   //time to switch from write to read
   unsigned tWTP;   //time to switch from write to precharge in the same bank
   unsigned busW;

   unsigned nbkgrp; // number of bank groups (has to be power of 2)
   unsigned bk_tag_length; //number of bits that define a bank inside a bank group

   unsigned nbk;

   unsigned data_command_freq_ratio; // frequency ratio between DRAM data bus and command bus (2 for GDDR3, 4 for GDDR5)

   unsigned dram_atom_size; // number of bytes transferred per read or write command

   linear_to_raw_address_translation m_address_mapping;

   // service global/local memory access via RUBY
   bool use_ruby;
   ruby_config m_ruby_config;
};

enum pipeline_stage_name_t {
    ID_OC_SP=0,
    ID_OC_SFU,
    ID_OC_MEM,
    OC_EX_SP,
    OC_EX_SFU,
    OC_EX_MEM,
    EX_WB,
    N_PIPELINE_STAGES
};

const char* const pipeline_stage_name_decode[] = {
    "ID_OC_SP",
    "ID_OC_SFU",
    "ID_OC_MEM",
    "OC_EX_SP",
    "OC_EX_SFU",
    "OC_EX_MEM",
    "EX_WB",
    "N_PIPELINE_STAGES"
};

struct shader_core_config : public core_config
{
    shader_core_config(){
    pipeline_widths_string = NULL;
    }
    void init()
    {
        int ntok = sscanf(gpgpu_shader_core_pipeline_opt,"%d:%d",
                          &n_thread_per_shader,
                          &warp_size);
        if(ntok != 2) {
           printf("GPGPU-Sim uArch: error while parsing configuration string gpgpu_shader_core_pipeline_opt\n");
           abort();
    }

    char* toks = new char[100];
    char* tokd = toks;
    strcpy(toks,pipeline_widths_string);

    toks = strtok(toks,",");
    for (unsigned i = 0; i < N_PIPELINE_STAGES; i++) {
        assert(toks);
        ntok = sscanf(toks,"%d", &pipe_widths[i]);
        assert(ntok == 1);
        toks = strtok(NULL,",");
    }
    delete[] tokd;

        if (n_thread_per_shader > MAX_THREAD_PER_SM) {
           printf("GPGPU-Sim uArch: Error ** increase MAX_THREAD_PER_SM in abstract_hardware_model.h from %u to %u\n",
                  MAX_THREAD_PER_SM, n_thread_per_shader);
           abort();
        }
        max_warps_per_shader =  n_thread_per_shader/warp_size;
        assert( !(n_thread_per_shader % warp_size) );
        max_sfu_latency = 512;
        max_sp_latency = 32;
        m_L1I_config.init();
        m_L1T_config.init();
        m_L1C_config.init();
        m_L1D_config.init();
        gpgpu_cache_texl1_linesize = m_L1T_config.get_line_sz();
        gpgpu_cache_constl1_linesize = m_L1C_config.get_line_sz();
        m_valid = true;
    }
    void reg_options(class OptionParser * opp );
    unsigned max_cta( const kernel_info_t &k ) const;
    unsigned num_shader() const { return n_simt_clusters*n_simt_cores_per_cluster; }
    unsigned sid_to_cluster( unsigned sid ) const { return sid / n_simt_cores_per_cluster; }
    unsigned sid_to_cid( unsigned sid )     const { return sid % n_simt_cores_per_cluster; }
    unsigned cid_to_sid( unsigned cid, unsigned cluster_id ) const { return cluster_id*n_simt_cores_per_cluster + cid; }

// data
    char *gpgpu_shader_core_pipeline_opt;
    bool gpgpu_perfect_mem;
    enum divergence_support_t model;
    unsigned n_thread_per_shader;
    unsigned max_warps_per_shader;
    unsigned max_cta_per_core; //Limit on number of concurrent CTAs in shader core

    char * gpgpu_scheduler_string;

    char* pipeline_widths_string;
    int pipe_widths[N_PIPELINE_STAGES];

    cache_config m_L1I_config;
    cache_config m_L1T_config;
    cache_config m_L1C_config;
    cache_config m_L1D_config;

    bool gmem_skip_L1D; // on = global memory access always skip the L1 cache

    bool gpgpu_dwf_reg_bankconflict;

    int gpgpu_num_sched_per_core;
    int gpgpu_max_insn_issue_per_warp;

    //op collector
    int gpgpu_operand_collector_num_units_sp;
    int gpgpu_operand_collector_num_units_sfu;
    int gpgpu_operand_collector_num_units_mem;
    int gpgpu_operand_collector_num_units_gen;

    unsigned int gpgpu_operand_collector_num_in_ports_sp;
    unsigned int gpgpu_operand_collector_num_in_ports_sfu;
    unsigned int gpgpu_operand_collector_num_in_ports_mem;
    unsigned int gpgpu_operand_collector_num_in_ports_gen;

    unsigned int gpgpu_operand_collector_num_out_ports_sp;
    unsigned int gpgpu_operand_collector_num_out_ports_sfu;
    unsigned int gpgpu_operand_collector_num_out_ports_mem;
    unsigned int gpgpu_operand_collector_num_out_ports_gen;

    int gpgpu_num_sp_units;
    int gpgpu_num_sfu_units;
    int gpgpu_num_mem_units;

    //Shader core resources
    unsigned gpgpu_shader_registers;
    int gpgpu_warpdistro_shader;
    unsigned gpgpu_num_reg_banks;
    bool gpgpu_reg_bank_use_warp_id;
    bool gpgpu_local_mem_map;

    unsigned max_sp_latency;
    unsigned max_sfu_latency;

    unsigned n_simt_cores_per_cluster;
    unsigned n_simt_clusters;
    unsigned n_simt_ejection_buffer_size;
    unsigned ldst_unit_response_queue_size;

    int simt_core_sim_order;

    unsigned mem2device(unsigned memid) const { return memid + n_simt_clusters; }
};

class gpgpu_sim_config : public gpgpu_functional_sim_config {
public:
    gpgpu_sim_config() { m_valid = false; }
    void reg_options(class OptionParser * opp);
    void init()
    {
        gpu_stat_sample_freq = 10000;
        gpu_runtime_stat_flag = 0;
        sscanf(gpgpu_runtime_stat, "%d:%x", &gpu_stat_sample_freq, &gpu_runtime_stat_flag);
        m_shader_config.init();
        ptx_set_tex_cache_linesize(m_shader_config.m_L1T_config.get_line_sz());
        m_memory_config.init();
        init_clock_domains();

        // initialize file name if it is not set
        time_t curr_time;
        time(&curr_time);
        char *date = ctime(&curr_time);
        char *s = date;
        while (*s) {
            if (*s == ' ' || *s == '\t' || *s == ':') *s = '-';
            if (*s == '\n' || *s == '\r' ) *s = 0;
            s++;
        }
        char buf[1024];
        snprintf(buf,1024,"gpgpusim_visualizer__%s.log.gz",date);
        g_visualizer_filename = strdup(buf);

        m_valid=true;
    }

    unsigned num_shader() const { return m_shader_config.num_shader(); }
    unsigned get_max_concurrent_kernel() const { return max_concurrent_kernel; }

private:
    void init_clock_domains(void );

    bool m_valid;
    shader_core_config m_shader_config;
    memory_config m_memory_config;

    // clock domains - frequency
    double core_freq;
    double icnt_freq;
    double dram_freq;
    double l2_freq;
    double core_period;
    double icnt_period;
    double dram_period;
    double l2_period;

    // GPGPU-Sim timing model options
    unsigned gpu_max_cycle_opt;
    unsigned gpu_max_insn_opt;
    unsigned gpu_max_cta_opt;
    char *gpgpu_runtime_stat;
    bool  gpgpu_flush_cache;
    bool  gpu_deadlock_detect;
    int gpu_deadlock_threshold;
    int   gpgpu_dram_sched_queue_size;
    int   gpgpu_cflog_interval;
    char * gpgpu_clock_domains;
    char * gpgpu_scale_bandwidths;
    unsigned max_concurrent_kernel;

    // visualizer
    bool  g_visualizer_enabled;
    char *g_visualizer_filename;
    int   g_visualizer_zlevel;

    // statistics collection
    int gpu_stat_sample_freq;
    int gpu_runtime_stat_flag;

    friend class gpgpu_sim;
    friend class ruby_wrapper;
    friend class gpusim_ruby_config;        // config struct used by Ruby
};

#endif //GPU_CONFIG_H
