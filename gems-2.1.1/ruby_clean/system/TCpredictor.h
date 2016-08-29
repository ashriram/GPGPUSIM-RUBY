#ifndef TCPREDICTOR_H
#define TCPREDICTOR_H

#include "Global.h"
#include "GenericMachineType.h"
#include "Consumer.h"
#include "Histogram.h"
#include <set>
#include <map>
#include "IntPairSet.h"

class TCpredictorBase;

class TCpredictorContainer : public Consumer {
public:
  // Constructors
   TCpredictorContainer(int numL2Banks);

  // Destructor
  ~TCpredictorContainer();

  void wakeup();

  void printStats(ostream& out, bool short_stats=false);
  void clearStats();
  void printConfig(ostream& out) const;
  void print(ostream& out) const;

  TCpredictorBase* getPredictor();

private:
  TCpredictorBase* m_predictor;
};

// Output operator declaration
ostream& operator<<(ostream& out, const TCpredictorContainer& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const TCpredictorContainer& obj)
{
  obj.print(out);
  out << flush;
  return out;
}


/*
 * TC Predictor base class
 */
class TCpredictorBase {
public:
    virtual ~TCpredictorBase() = 0;

    virtual void printStats(ostream& out, bool short_stats=false) = 0;
    virtual void clearStats() = 0;
    virtual void printConfig(ostream& out) const = 0;
    virtual void print(ostream& out) const = 0;
    virtual void wakeup() = 0;
};

/*
 * Global Predictor
 */
class TCpredictorGlobal : public TCpredictorBase {
public:
    TCpredictorGlobal();
    ~TCpredictorGlobal();

    void printStats(ostream& out, bool short_stats=false);
    void clearStats();
    void printConfig(ostream& out) const;
    void print(ostream& out) const;
    void wakeup() { }

    // Global predictor methods
    Time get_pred_global_lifetime();
    void adjust_lease(int delta, int min, int max);

private:
    int m_lease;

    // stats
    int64 m_stats_lease_requests;
    Histogram m_stats_delta_size;
};

/*
 * PerL2 Predictor
 */
class TCpredictorPerL2 : public TCpredictorBase {
public:
    TCpredictorPerL2(int numL2Banks);
    ~TCpredictorPerL2();

    void printStats(ostream& out, bool short_stats=false);
    void clearStats();
    void printConfig(ostream& out) const;
    void print(ostream& out) const;
    void wakeup() { }

    // PerL2 predictor methods
    Time get_pred_perL2_lifetime(int L2BankId);
    void adjust_lease(int L2BankId, int delta, int min, int max);
    void adjust_lease_all(int delta, int min, int max);

private:
    int m_numL2Banks;
    std::vector<int> m_leases;

    // stats
    std::vector<int64> m_stats_lease_requests;
    std::vector<Histogram> m_stats_delta_sizes;
    Histogram m_stats_delta_sizes_all;

};

/*
 * PC-Adaptive Predictor
 */
class TCpredictorPCAdaptive : public TCpredictorBase {
public:
    TCpredictorPCAdaptive();
    ~TCpredictorPCAdaptive();

    void printStats(ostream& out, bool short_stats=false);
    void clearStats();
    void printConfig(ostream& out) const;
    void print(ostream& out) const;
    void wakeup() { }

    // PCAdaptive predictor methods
    Time get_pred_pcadaptive_lifetime(int pc, int core_id);
    void expiredhit(IntPairSet pcsharers, IntPair reader);
    void unexpiredwrite(IntPairSet pcsharers, IntPair writer);
    void unexpiredatomic(IntPairSet pcsharers, IntPair writer);
    void l2evict(IntPairSet pcsharers);

private:
    // pc <-> lifetime map
    std::map<IntPair, int> m_pcsharer_leases;

    // stats
    std::map<int,Histogram> m_stats_delta_sizes; // keep stats on leases given for each pc
    Histogram m_stats_pc_collisions;             // keep track of how many times multiple pcs exist in an L2 block

};


/*
 * PCfixed Predictor
 */
class TCpredictorPCfixed : public TCpredictorBase {
public:
    TCpredictorPCfixed();
    ~TCpredictorPCfixed();

    void printStats(ostream& out, bool short_stats=false);
    void clearStats();
    void printConfig(ostream& out) const;
    void print(ostream& out) const;
    void wakeup() { }

    // Global predictor methods
    Time get_pred_PCfixed_lifetime(int pc);

private:

    // per-pc fixed lifetime configuration
    std::map<int, int> m_perpc_config_average;

    // stats
    int64 m_stats_lease_requests;
    std::map<int, int> m_stats_undefined_pc_count;
    std::map<int, int> m_stats_defined_pc_count;
};

/*
 * Addrfixed Predictor
 */
class TCpredictorAddrfixed : public TCpredictorBase {
public:
    TCpredictorAddrfixed();
    ~TCpredictorAddrfixed();

    void printStats(ostream& out, bool short_stats=false);
    void clearStats();
    void printConfig(ostream& out) const;
    void print(ostream& out) const;
    void wakeup() { }

    // Global predictor methods
    Time get_pred_Addrfixed_lifetime(int addr);

private:

    // per-addr fixed lifetime configuration
    std::map<int, int> m_peraddr_config_average;
    // chunk size for addresses
    int m_addr_chunk_granularity;

    // stats
    int64 m_stats_lease_requests;
    Histogram m_stats_delta_sizes; // keep stats on leases given
    int64 m_stats_addr_found;
    int64 m_stats_addr_notfound;
};


/*
 * PC-Sampler Predictor
 */
class TCpredictorPCSampler : public TCpredictorBase {
public:
    TCpredictorPCSampler(Consumer* container);
    ~TCpredictorPCSampler();

    void printStats(ostream& out, bool short_stats=false);
    void clearStats();
    void printConfig(ostream& out) const;
    void print(ostream& out) const;
    void wakeup();

    // PCAdaptive predictor methods
    Time get_pred_pcsampler_lifetime(int pc);

    void addLoadPC(Time access_time, int address, int pc);
    void expiredLoadHit(Time access_time, int address, int pc);
    void endLifetime(Time end_time, int address);

private:

    void updatePredictorFromLifetimeEnd(int pc, Time lifetime);
    void updatePredictionTime(int pc, Time lifetime);

    struct predictionInfo {
        Time m_prediction_lifetime;     // average lifetime observed so far
        int m_updates;                  // number of times this prediction has been updated
        int m_sat_counter;              // saturating counter
        predictionInfo() : m_prediction_lifetime(0), m_updates(0), m_sat_counter(0) {}
        predictionInfo(Time lifetime) : m_prediction_lifetime(lifetime), m_updates(0), m_sat_counter(0) {}
    };

    typedef std::map<int,predictionInfo> map_prediction_table_t;       // prediction table to map pc to lifetime prediction
    map_prediction_table_t m_prediction_table;

    typedef std::map<int,Time> map_pc_starttimes_t;          // map to keep track of a list of pc's and their starting access times
    typedef std::map<int, map_pc_starttimes_t> map_sampler_table_t;  // sampler table to map addresses to their list of pc's
    map_sampler_table_t m_sampler_table;

    Consumer* m_container;      // pointer to container for wakeup purposes

    // config
    Time m_max_lifetime;        // max lifetime that can possibly be given out
    int m_sat_counter_max;

    // stats

};

#endif // TCPREDICTOR_H
