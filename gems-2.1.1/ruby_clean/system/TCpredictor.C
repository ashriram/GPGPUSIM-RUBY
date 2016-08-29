#include "TCpredictor.h"
#include "Global.h"
#include "System.h"
#include "Debug.h"

/*******************************************************************
 * TCpredictorContainer class
 *******************************************************************/
TCpredictorContainer::TCpredictorContainer(int numL2Banks) {
    if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_FIXED)
        m_predictor = NULL;
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_GLOBAL)
        m_predictor = new TCpredictorGlobal();
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PERL2)
        m_predictor = new TCpredictorPerL2(numL2Banks);
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCADAPTIVE)
        m_predictor = new TCpredictorPCAdaptive();
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCFIXED)
        m_predictor = new TCpredictorPCfixed();
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ADDRFIXED)
        m_predictor = new TCpredictorAddrfixed();
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ORACLE)
        m_predictor = NULL;
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCSAMPLER)
        m_predictor = new TCpredictorPCSampler(this);
    else {
        printf("Invalid predictor type\n");
        assert(0);
    }
}

TCpredictorContainer::~TCpredictorContainer()
{
    if(m_predictor != NULL)
        delete m_predictor;
}

TCpredictorBase* TCpredictorContainer::getPredictor()
{
    assert(m_predictor != NULL);
    return m_predictor;
}

void TCpredictorContainer::print(ostream& out) const
{
    if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_FIXED)
        out << "[TCpredictorFixed]";
    else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ORACLE)
        out << "[TCpredictorOracle]";
    else {
        assert(m_predictor != NULL);
        m_predictor->print(out);
    }
}

void TCpredictorContainer::printStats(ostream& out, bool short_stats)
{
    if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_FIXED) {

    } else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ORACLE) {

    } else {
        assert(m_predictor != NULL);
        m_predictor->printStats(out, short_stats);
    }
}

void TCpredictorContainer::clearStats()
{
    if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_FIXED) {

    } else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ORACLE) {

    } else {
        assert(m_predictor != NULL);
        m_predictor->clearStats();
    }
}

void TCpredictorContainer::printConfig(ostream& out) const
{
    if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_FIXED) {

    } else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ORACLE) {

    } else {
        assert(m_predictor != NULL);
        m_predictor->printConfig(out);
    }
}

void TCpredictorContainer::wakeup()
{
    if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_FIXED) {

    } else if(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ORACLE) {

    } else {
        assert(m_predictor != NULL);
        m_predictor->wakeup();
    }
}


/*******************************************************************
 * TCpredictorGlobal class
 *******************************************************************/
TCpredictorGlobal::TCpredictorGlobal() {
    m_lease = CL_FIXED_LEASE;   // start lease at specified value
    m_stats_lease_requests = 0;
}

TCpredictorGlobal::~TCpredictorGlobal() { }

void TCpredictorGlobal::print(ostream& out) const
{
    out << "[TCpredictorGlobal]";
}

void TCpredictorGlobal::printStats(ostream& out, bool short_stats)
{

    out << endl;
    if (short_stats) {
        out << "SHORT ";
    }
    out << "TCpredictorGlobal Stats" << endl;
    out << "--------------" << endl;
    out << "lease_requests: " << m_stats_lease_requests << endl;
    out << "delta_size: " << m_stats_delta_size << endl;
    out << "--------------" << endl;
}

void TCpredictorGlobal::clearStats()
{
    m_stats_lease_requests = 0;
    m_stats_delta_size.clear(-1, 20);
}

void TCpredictorGlobal::printConfig(ostream& out) const
{

    out << endl;
    out << "TCpredictorGlobal Configuration" << endl;
    out << "----------------------" << endl;
    out << "----------------------" << endl;
}

Time TCpredictorGlobal::get_pred_global_lifetime() {
    assert(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_GLOBAL);   // global predictor

    Time return_lease;
    assert(m_lease >= 0);
    return_lease = (Time) (m_lease);

    m_stats_lease_requests++;
    m_stats_delta_size.add(m_lease);
    return return_lease;
}

void TCpredictorGlobal::adjust_lease(int delta, int min, int max) {
    // Only adjust lease if within min and max boundaries, if specified
    if( (min==0 or m_lease>min) and (max==0 or m_lease<max) )
    m_lease += delta;

    // Make sure lease is not negative time
    if(m_lease < 0)
        m_lease = 0;
}


/*******************************************************************
 * TCpredictorPerL2 class
 *******************************************************************/
TCpredictorPerL2::TCpredictorPerL2(int numL2Banks) {
    m_numL2Banks = numL2Banks;
    m_leases.resize(m_numL2Banks);
    m_stats_lease_requests.resize(m_numL2Banks);
    m_stats_delta_sizes.resize(m_numL2Banks);

    for(int i=0; i<m_numL2Banks; i++) {
        m_leases[i] = CL_FIXED_LEASE;       // start leases at specified value
    }
}

TCpredictorPerL2::~TCpredictorPerL2() { }

void TCpredictorPerL2::print(ostream& out) const
{
    out << "[TCpredictorPerL2]";
}

void TCpredictorPerL2::printStats(ostream& out, bool short_stats)
{

    out << endl;
    if (short_stats) {
        out << "SHORT ";
    }
    out << "TCpredictorPerL2 Stats" << endl;
    out << "--------------" << endl;
    out << "lease_requests:";
    for(int i=0; i<m_numL2Banks; i++)
        out << " " << m_stats_lease_requests[i];
    out << endl;
    for(int i=0; i<m_numL2Banks; i++)
        out << "delta_sizes[" << i << "]: " << m_stats_delta_sizes[i] << endl;
    out << "delta_sizes_all: " << m_stats_delta_sizes_all << endl;
    out << "delta_sizes_all_sum: " << m_stats_delta_sizes_all.getTotal() << endl;
    out << "delta_sizes_all_count: " << m_stats_delta_sizes_all.size() << endl;
    out << "avg_lifetime_size: " << setw(5) << ((double) m_stats_delta_sizes_all.getTotal()/m_stats_delta_sizes_all.size()) << endl;
    out << "--------------" << endl;
}

void TCpredictorPerL2::clearStats()
{
    for(int i=0; i<m_numL2Banks; i++) {
        m_stats_lease_requests[i] = 0;
        m_stats_delta_sizes[i].clear(-1, 20);
    }
    m_stats_delta_sizes_all.clear(-1,20);
}

void TCpredictorPerL2::printConfig(ostream& out) const
{

    out << endl;
    out << "TCpredictorPerL2 Configuration" << endl;
    out << "----------------------" << endl;
    out << "numL2Banks: " << m_numL2Banks << endl;
    out << "----------------------" << endl;
}

Time TCpredictorPerL2::get_pred_perL2_lifetime(int L2BankId) {
    assert(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PERL2);   // PerL2 predictor
    assert(L2BankId < m_numL2Banks);

    Time return_lease;
    assert(m_leases[L2BankId] >= 0);
    return_lease = (Time) (m_leases[L2BankId]);

    m_stats_lease_requests[L2BankId]++;
    m_stats_delta_sizes[L2BankId].add((int)return_lease);
    m_stats_delta_sizes_all.add((int)return_lease);
    return return_lease;
}

void TCpredictorPerL2::adjust_lease(int L2BankId, int delta, int min, int max) {
    assert(L2BankId < m_numL2Banks);

    // Only adjust lease if within min and max boundaries, if specified
    if( (min==0 or m_leases[L2BankId]>min) and (max==0 or m_leases[L2BankId]<max) )
        m_leases[L2BankId] += delta;

    // Make sure lease is not negative time
    if(m_leases[L2BankId] < 0)
        m_leases[L2BankId] = 0;
}

void TCpredictorPerL2::adjust_lease_all(int delta, int min, int max) {
    for(int i=0; i<m_numL2Banks; i++) {
        adjust_lease(i, delta, min, max);
    }
}


/*******************************************************************
 * TCpredictorPCAdaptive class
 *******************************************************************/
TCpredictorPCAdaptive::TCpredictorPCAdaptive() {

}

TCpredictorPCAdaptive::~TCpredictorPCAdaptive() { }

void TCpredictorPCAdaptive::print(ostream& out) const
{
    out << "[TCpredictorPCAdaptive]";
}

void TCpredictorPCAdaptive::printStats(ostream& out, bool short_stats)
{

    out << endl;
    if (short_stats) {
        out << "SHORT ";
    }
    out << "TCpredictorPCAdaptive Stats" << endl;
    out << "--------------" << endl;
    for(std::map<int,Histogram>::iterator it=m_stats_delta_sizes.begin();
        it!=m_stats_delta_sizes.end();
        it++)
    {
        int pc = (*it).first;
        Histogram& pc_histogram = (*it).second;
        out << "delta_sizes[" << pc << "]: " << pc_histogram << endl;
    }
    out << "--------------" << endl;
    out << "pc_collisions: " << m_stats_pc_collisions << endl;
    out << "--------------" << endl;
}

void TCpredictorPCAdaptive::clearStats()
{
    m_stats_delta_sizes.clear();
    m_stats_pc_collisions.clear(1,20);
}

void TCpredictorPCAdaptive::printConfig(ostream& out) const
{

    out << endl;
    out << "TCpredictorPCAdaptive Configuration" << endl;
    out << "----------------------" << endl;
    out << "----------------------" << endl;
}

Time TCpredictorPCAdaptive::get_pred_pcadaptive_lifetime(int pc, int core_id) {
    assert(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCADAPTIVE);   // PCAdaptive predictor

    IntPair pcsharer = IntPair(pc,core_id);
    // If pc,sharer is not being tracked, add it with default value
    if(m_pcsharer_leases.count(pcsharer) == 0)
        m_pcsharer_leases[pcsharer] = CL_FIXED_LEASE;

    // Add a histogram for this pc if it doesn't exist
    if(m_stats_delta_sizes.count(pc) == 0)
        m_stats_delta_sizes[pc] = Histogram(-1,20);

    Time return_lease = (Time) m_pcsharer_leases[pcsharer];
    m_stats_delta_sizes[pc].add(return_lease);
    return return_lease;
}


// Increment pc,sharer lifetime only if it eists in the pcsharers set (i.e. it has read this address before)
void TCpredictorPCAdaptive::expiredhit(IntPairSet pcsharers, IntPair reader) {
    if(pcsharers.count(reader)) {
        // Read exists in pcsharers
        assert(m_pcsharer_leases.count(reader) > 0);    // it must have requested a lease previously

        m_pcsharer_leases[reader] += CL_PRED_PCADAPTIVE_EXPIREHIT_CREDIT;
    }
}

// Decrement lifetime of all pc,sharers in existing pcsharers vector except those that belong to the writer
// (i.e. writes don't hurt writer's lifetime)
void TCpredictorPCAdaptive::unexpiredwrite(IntPairSet pcsharers, IntPair writer) {
    for(IntPairSet::iterator it=pcsharers.begin(); it!=pcsharers.end(); it++) {
        if(it->second != writer.second) {
            // This pc,sharer does not belong to writing core, decrement its lifetime
            assert(m_pcsharer_leases.count(*it) > 0);   // it must have requested a lease previously
            m_pcsharer_leases[*it] -= CL_PRED_PCADAPTIVE_WRITEUNEXPIRED_PENALTY;
            if( m_pcsharer_leases[*it] < 0)
                m_pcsharer_leases[*it] = 0;
        }
    }
}

// Decrement lifetime of all pc,sharers
void TCpredictorPCAdaptive::unexpiredatomic(IntPairSet pcsharers, IntPair writer) {
    for(IntPairSet::iterator it=pcsharers.begin(); it!=pcsharers.end(); it++) {
        assert(m_pcsharer_leases.count(*it) > 0);   // it must have requested a lease previously
        m_pcsharer_leases[*it] -= CL_PRED_PCADAPTIVE_WRITEUNEXPIRED_PENALTY;
        if( m_pcsharer_leases[*it] < 0)
            m_pcsharer_leases[*it] = 0;
    }
}

// Decrement lifetime of all pc,sharers
void TCpredictorPCAdaptive::l2evict(IntPairSet pcsharers) {
    for(IntPairSet::iterator it=pcsharers.begin(); it!=pcsharers.end(); it++) {
        assert(m_pcsharer_leases.count(*it) > 0);   // it must have requested a lease previously
        m_pcsharer_leases[*it] -= CL_PRED_PCADAPTIVE_L2EVICT_PENALTY;
        if( m_pcsharer_leases[*it] < 0)
            m_pcsharer_leases[*it] = 0;
    }
}





/*******************************************************************
 * TCpredictorPCfixed class
 *******************************************************************/
TCpredictorPCfixed::TCpredictorPCfixed() {
    m_stats_lease_requests = 0;

    // Open infile and read in settings
    FILE * perpc_infile = fopen((const char *)CL_PRED_PCFIXED_INFILE, "r");
    if(perpc_infile == NULL) {
        ERROR_MSG("Unable to open input file for TCpredictorPCfixed");
    } else {
        cout << "[TCpredictorPCfixed]: successfully opened infile\n";
        int in_pc, in_max, in_count;
        float in_average, in_std;
        char in_histogram[300];
        while(
                fscanf(perpc_infile, "PC=%d [binsize: log2 max: %d count: %d average: %f | standard deviation: %f | %[^\n]\n",
                &in_pc, &in_max, &in_count, &in_average, &in_std, in_histogram) == 6
        ) {
            assert(m_perpc_config_average.count(in_pc) == 0); // Must be unique pc
            assert(in_pc >= 0);                                 // Must be a valid pc
            m_perpc_config_average[in_pc] = (int) in_average;   // record average lifetime for this pc
        }
    }
}

TCpredictorPCfixed::~TCpredictorPCfixed() { }

void TCpredictorPCfixed::print(ostream& out) const
{
    out << "[TCpredictorPCfixed]";
}

void TCpredictorPCfixed::printStats(ostream& out, bool short_stats)
{

    out << endl;
    if (short_stats) {
        out << "SHORT ";
    }
    out << "TCpredictorPCfixed Stats" << endl;
    out << "--------------" << endl;
    out << "lease_requests: " << m_stats_lease_requests << endl;
    out << "defined_pc_requests: ";
    for(std::map<int, int>::const_iterator it=m_stats_defined_pc_count.begin();
        it != m_stats_defined_pc_count.end();
        it++) {
        out << (*it).first << "=" << (*it).second << ", ";
    }
    out << endl;
    out << "undefined_pc_requests: ";
    for(std::map<int, int>::const_iterator it=m_stats_undefined_pc_count.begin();
        it != m_stats_undefined_pc_count.end();
        it++) {
        out << (*it).first << "=" << (*it).second << ", ";
    }
    out << endl;
    out << "--------------" << endl;
}

void TCpredictorPCfixed::clearStats()
{
    m_stats_lease_requests = 0;
}

void TCpredictorPCfixed::printConfig(ostream& out) const
{

    out << endl;
    out << "TCpredictorPCfixed Configuration" << endl;
    out << "----------------------" << endl;
    out << "PC averages: ";
    for(std::map<int, int>::const_iterator it=m_perpc_config_average.begin();
        it != m_perpc_config_average.end();
        it++) {
        out << (*it).first << "=" << (*it).second << ", ";
    }
    out << endl;
    out << "----------------------" << endl;
}

Time TCpredictorPCfixed::get_pred_PCfixed_lifetime(int pc) {
    assert(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCFIXED);   // PCfixed predictor
    m_stats_lease_requests++;

    Time return_lease = 0;
    if(m_perpc_config_average.count(pc) == 1) {
        // pc is defined in config
        return_lease = (Time) m_perpc_config_average[pc];
        m_stats_defined_pc_count[pc] += 1;
    } else {
        // pc is not defined in config
        return_lease = (Time) CL_FIXED_LEASE;
        m_stats_undefined_pc_count[pc] += 1;
    }

    return return_lease;
}


/*******************************************************************
 * TCpredictorAddrfixed class
 *******************************************************************/
TCpredictorAddrfixed::TCpredictorAddrfixed() {
    m_stats_lease_requests = 0;
    m_stats_addr_found = 0;
    m_stats_addr_notfound = 0;

    // Open infile and read in settings
    FILE * peraddr_infile = fopen((const char *)CL_PRED_ADDRFIXED_INFILE, "r");
    if(peraddr_infile == NULL) {
        ERROR_MSG("Unable to open input file for TCpredictorAddrfixed");
    } else {
        cout << "[TCpredictorAddrfixed]: successfully opened infile\n";
        // First read in chunk size
        if(fscanf(peraddr_infile, "chunk=%d\n", &m_addr_chunk_granularity) != 1)
            ERROR_MSG("Unable to read chunk value in input file for TCpredictorAddrfixed");
        cout << "[TCpredictorAddrfixed]: chunk=" << m_addr_chunk_granularity << endl;

        long in_addr;
        int in_max, in_count;
        float in_average, in_std;
        char in_histogram[300];
        while(
                fscanf(peraddr_infile, "Addr=%ld [binsize: log2 max: %d count: %d average: %f | standard deviation: %f | %[^\n]\n",
                &in_addr, &in_max, &in_count, &in_average, &in_std, in_histogram) == 6
        ) {
            assert(in_addr >= 0);                                 // Must be a valid addr
            assert(in_addr % m_addr_chunk_granularity == 0);    // addr must conform to chunk size
            assert(m_peraddr_config_average.count(in_addr) == 0); // Must be unique addr chunk
            m_peraddr_config_average[in_addr] = (int) in_average;   // record average lifetime for this pc
        }
        cout << "[TCpredictorAddrfixed]: loaded " << m_peraddr_config_average.size() << " addresses from infile" << endl;

    }
}

TCpredictorAddrfixed::~TCpredictorAddrfixed() { }

void TCpredictorAddrfixed::print(ostream& out) const
{
    out << "[TCpredictorAddrfixed]";
}

void TCpredictorAddrfixed::printStats(ostream& out, bool short_stats)
{

    out << endl;
    if (short_stats) {
        out << "SHORT ";
    }
    out << "TCpredictorAddrfixed Stats" << endl;
    out << "--------------" << endl;
    out << "lease_requests: " << m_stats_lease_requests << endl;
    out << "delta_sizes: " << m_stats_delta_sizes << endl;
    out << "addr_found: " << m_stats_addr_found << endl;
    out << "addr_notfound: " << m_stats_addr_notfound << endl;
    out << "--------------" << endl;
}

void TCpredictorAddrfixed::clearStats()
{
    m_stats_lease_requests = 0;
    m_stats_addr_found = 0;
    m_stats_addr_notfound = 0;
    m_stats_delta_sizes.clear(-1,20);
}

void TCpredictorAddrfixed::printConfig(ostream& out) const
{

    out << endl;
    out << "TCpredictorAddrfixed Configuration" << endl;
    out << "----------------------" << endl;
    out << "----------------------" << endl;
}

Time TCpredictorAddrfixed::get_pred_Addrfixed_lifetime(int addr) {
    assert(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ADDRFIXED);   // Addrfixed predictor
    m_stats_lease_requests++;

    Time return_lease = 0;
    // Calculate the chunk addr from chunk size
    int addr_chunk = addr & (~(m_addr_chunk_granularity-1));
    if(m_peraddr_config_average.count(addr_chunk) > 0) {
        // addr is defined in config
        return_lease = (Time) m_peraddr_config_average[addr_chunk];
        m_stats_addr_found++;
    } else {
        // addr is not defined in config
        return_lease = (Time) CL_FIXED_LEASE;
        m_stats_addr_notfound++;
    }

    m_stats_delta_sizes.add(return_lease);

    return return_lease;
}


/*******************************************************************
 * TCpredictorPCAdaptive class
 *******************************************************************/
TCpredictorPCSampler::TCpredictorPCSampler(Consumer* container) {
    m_container = container;
    m_max_lifetime = 20000;
    m_sat_counter_max = 32;

    // Set wakeup
    g_eventQueue_ptr->scheduleEvent(m_container, 1);
}

TCpredictorPCSampler::~TCpredictorPCSampler() { }

void TCpredictorPCSampler::print(ostream& out) const
{
    out << "[TCpredictorPCSampler]";
}

void TCpredictorPCSampler::printStats(ostream& out, bool short_stats)
{

    out << endl;
    if (short_stats) {
        out << "SHORT ";
    }
    out << "TCpredictorPCSampler Stats" << endl;
    out << "--------------" << endl;
    out << "--------------" << endl;
}

void TCpredictorPCSampler::clearStats()
{
}

void TCpredictorPCSampler::printConfig(ostream& out) const
{

    out << endl;
    out << "TCpredictorPCSampler Configuration" << endl;
    out << "----------------------" << endl;
    out << "----------------------" << endl;
}

Time TCpredictorPCSampler::get_pred_pcsampler_lifetime(int pc) {
    assert(CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCSAMPLER);   // PCAdaptive predictor

    // Set prediction to initial constant if first access
    if(m_prediction_table.count(pc) == 0)
        m_prediction_table[pc] = predictionInfo((Time)CL_FIXED_LEASE);

    assert(m_prediction_table[pc].m_prediction_lifetime >= 0);

    return m_prediction_table[pc].m_prediction_lifetime;
}

void TCpredictorPCSampler::addLoadPC(Time access_time, int address, int pc) {
    // Check if this pc is already being tracked for this address
    // If it is, don't do anything
    if(m_sampler_table.count(address) > 0)
        if(m_sampler_table[address].count(pc) > 0)
            return;

    // track this pc for this address
    m_sampler_table[address][pc] = access_time;
}

void TCpredictorPCSampler::endLifetime(Time end_time, int address) {
    // If no pc's are tracked, do nothing (can happen for L2 evictions after a write access)
    if(m_sampler_table.count(address) == 0)
        return;

    // Update predictor for all pc's being tracked for this address
    for(map_pc_starttimes_t::iterator it=m_sampler_table[address].begin();
        it!=m_sampler_table[address].end();
        it++)
    {
        int pc = it->first;
        Time lifetime = end_time - it->second;
        assert(lifetime >= 0);
        updatePredictorFromLifetimeEnd(pc, lifetime);
    }

    // Remove all pc's for this address
    m_sampler_table.erase(address);
}

void TCpredictorPCSampler::expiredLoadHit(Time access_time, int address, int pc) {
    /*
    // If this pc has no prediction but is being tracked, then set the lifetime to whatever we have measured uptil now
    assert(m_prediction_table.count(pc) > 0);   // pc must have done a load before

    if(m_prediction_table[pc].m_updates == 0) {
        // No updates so far (due to lifetime end)
        if(m_sampler_table.count(address) > 0) {
            if(m_sampler_table[address].count(pc) > 0 and access_time > m_sampler_table[address][pc]) {
                // PC is being tracked
                Time new_lifetime = access_time - m_sampler_table[address][pc];
                updatePredictionTime(pc, new_lifetime);
            }
        }
    }
    */
}

void TCpredictorPCSampler::updatePredictorFromLifetimeEnd(int pc, Time lifetime) {
    // This pc must exist in the predictor (it must have been loaded previously)
    assert(m_prediction_table.count(pc) > 0);

    /*
    // Saturating counter based prediction
    Time prev_prediction = m_prediction_table[pc].m_prediction_lifetime;

    if(lifetime > prev_prediction*2)
        m_prediction_table[pc].m_sat_counter++;
    else if(lifetime < prev_prediction/2)
        m_prediction_table[pc].m_sat_counter--;

    if(m_prediction_table[pc].m_sat_counter > m_sat_counter_max) {
        updatePredictionTime(pc, m_prediction_table[pc].m_prediction_lifetime*2);
        m_prediction_table[pc].m_sat_counter = 0;
    } else if(m_prediction_table[pc].m_sat_counter < -m_sat_counter_max) {
        updatePredictionTime(pc, m_prediction_table[pc].m_prediction_lifetime/2);
        m_prediction_table[pc].m_sat_counter = 0;
    }
    */

    /*
    // Update the average lifetime sampled
    Time new_avg_lifetime = ((m_prediction_table[pc].m_prediction_lifetime * m_prediction_table[pc].m_updates) + lifetime) / (m_prediction_table[pc].m_updates+1);
    updatePredictionTime(pc,new_avg_lifetime);
    m_prediction_table[pc].m_updates += 1;
    */


    // Increment or decrement lifetime
    if(m_prediction_table[pc].m_prediction_lifetime > lifetime) {
        updatePredictionTime(pc, m_prediction_table[pc].m_prediction_lifetime-1);
        m_prediction_table[pc].m_updates += 1;
    } else if(m_prediction_table[pc].m_prediction_lifetime < lifetime) {
        updatePredictionTime(pc, m_prediction_table[pc].m_prediction_lifetime+1);
        m_prediction_table[pc].m_updates += 1;
    }

}


void TCpredictorPCSampler::updatePredictionTime(int pc, Time lifetime) {
    assert(m_prediction_table.count(pc) > 0);
    if(lifetime > m_max_lifetime)
        lifetime = m_max_lifetime;
    else if(lifetime < CL_FIXED_LEASE)
        lifetime = CL_FIXED_LEASE;
    m_prediction_table[pc].m_prediction_lifetime = lifetime;
}

void TCpredictorPCSampler::wakeup() {

    // Wake up every few cycles
    //g_eventQueue_ptr->scheduleEvent(m_container, 1000);
}


