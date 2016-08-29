#include "Global.h"
#include "System.h"
#include "TCpredictor.h"
#include "RubySlicc_TCpredictor_interface.h"
#include "RubySlicc_ComponentMapping.h"
#include "RubySlicc_Util.h"
#include "IntSet.h"

Time get_lease_fixed() {
   return CL_FIXED_LEASE;
}

bool is_pred_fixed() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_FIXED;
}
bool is_pred_global() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_GLOBAL;
}
bool is_pred_perL2() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PERL2;
}
bool is_pred_pcadaptive() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCADAPTIVE;
}
bool is_pred_PCfixed() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCFIXED;
}
bool is_pred_Addrfixed() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ADDRFIXED;
}
bool is_pred_oracle() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_ORACLE;
}
bool is_pred_pcsampler() {
    return CL_PREDICTOR_TYPE == CL_PREDICTOR_TYPE_PCSAMPLER;
}


/*
 * TC Global predictor functions
 */
int get_pred_global_writeunexpired() { return CL_PRED_GLOBAL_WRITEUNEXPIRED_PENALTY; }
int get_pred_global_l2evict() { return CL_PRED_GLOBAL_L2EVICT_PENALTY; }
int get_pred_global_expirehit() { return CL_PRED_GLOBAL_EXPIREHIT_CREDIT; }

Time get_pred_global_lifetime() {
   TCpredictorGlobal* predictorGlobal = dynamic_cast<TCpredictorGlobal*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
   return predictorGlobal->get_pred_global_lifetime();
}
void pred_global_decrease_lease(int delta, int min, int max) {
    TCpredictorGlobal* predictorGlobal = dynamic_cast<TCpredictorGlobal*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorGlobal->adjust_lease(-delta, min, max);
}
void pred_global_increase_lease(int delta, int min, int max) {
    TCpredictorGlobal* predictorGlobal = dynamic_cast<TCpredictorGlobal*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorGlobal->adjust_lease(delta, min, max);
}


/*
 * TC PerL2 predictor functions
 */
int get_pred_perL2_writeunexpired() { return CL_PRED_PERL2_WRITEUNEXPIRED_PENALTY; }
int get_pred_perL2_l2evict() { return CL_PRED_PERL2_L2EVICT_PENALTY; }
int get_pred_perL2_expirehit() { return CL_PRED_PERL2_EXPIREHIT_CREDIT; }

Time get_pred_perL2_lifetime(NodeID L2BankId) {
   TCpredictorPerL2* predictorPerL2 = dynamic_cast<TCpredictorPerL2*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
   return predictorPerL2->get_pred_perL2_lifetime(L2BankId);
}

void pred_perL2_decrease_lease(NodeID L2BankId, int delta, int min, int max) {
    TCpredictorPerL2* predictorPerL2 = dynamic_cast<TCpredictorPerL2*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorPerL2->adjust_lease(L2BankId, -delta, min, max);
}
void pred_perL2_increase_lease(NodeID L2BankId, int delta, int min, int max) {
    TCpredictorPerL2* predictorPerL2 = dynamic_cast<TCpredictorPerL2*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorPerL2->adjust_lease(L2BankId, delta, min, max);
}
void pred_perL2_decrease_lease_all(int delta, int min, int max) {
    TCpredictorPerL2* predictorPerL2 = dynamic_cast<TCpredictorPerL2*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorPerL2->adjust_lease_all(-delta, min, max);
}
void pred_perL2_increase_lease_all(int delta, int min, int max) {
    TCpredictorPerL2* predictorPerL2 = dynamic_cast<TCpredictorPerL2*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorPerL2->adjust_lease_all(delta, min, max);
}

/*
 * TC PC-Adaptive predictor functions
 */

Time get_pred_pcadaptive_lifetime(int pc, int core_id) {
    TCpredictorPCAdaptive* predictorPCAdaptive = dynamic_cast<TCpredictorPCAdaptive*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
   return predictorPCAdaptive->get_pred_pcadaptive_lifetime(pc, core_id);
}

void pred_pcadaptive_expiredhit(IntPairSet pcsharers, IntPair reader) {
    TCpredictorPCAdaptive* predictorPCAdaptive = dynamic_cast<TCpredictorPCAdaptive*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    predictorPCAdaptive->expiredhit(pcsharers,reader);
}
void pred_pcadaptive_unexpiredwrite(IntPairSet pcsharers, IntPair writer) {
    TCpredictorPCAdaptive* predictorPCAdaptive = dynamic_cast<TCpredictorPCAdaptive*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    predictorPCAdaptive->unexpiredwrite(pcsharers,writer);
}
void pred_pcadaptive_unexpiredatomic(IntPairSet pcsharers, IntPair writer) {
    TCpredictorPCAdaptive* predictorPCAdaptive = dynamic_cast<TCpredictorPCAdaptive*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    predictorPCAdaptive->unexpiredatomic(pcsharers,writer);
}
void pred_pcadaptive_l2evict(IntPairSet pcsharers) {
    TCpredictorPCAdaptive* predictorPCAdaptive = dynamic_cast<TCpredictorPCAdaptive*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    predictorPCAdaptive->l2evict(pcsharers);
}


/*
 * TC PCfixed predictor functions
 */
Time get_pred_PCfixed_lifetime(int pc) {
    TCpredictorPCfixed* predictorPCfixed = dynamic_cast<TCpredictorPCfixed*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorPCfixed->get_pred_PCfixed_lifetime(pc);
}

/*
 * TC Addrfixed predictor functions
 */
Time get_pred_Addrfixed_lifetime(int addr) {
    TCpredictorAddrfixed* predictorAddrfixed = dynamic_cast<TCpredictorAddrfixed*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
    return predictorAddrfixed->get_pred_Addrfixed_lifetime(addr);
}

/*
 * TC retroactive oracle predictor functions
 */
Time get_pred_oracle_lifetime() {
    return (Time) 1<<20;    // really large lifetime
}

NetDest pred_oracle_get_sharers(IntPairSet pcsharers) {
    NetDest sharers;
    for(IntPairSet::iterator it=pcsharers.begin(); it!=pcsharers.end(); it++) {
        assert(it->second < RubyConfig::numberOfL1Cache());
        sharers.add(getL1MachineID(it->second));
    }
    return sharers;
}


/*
 * TC PC-Sampler predictor functions
 */

Time get_pred_pcsampler_lifetime(int pc) {
    TCpredictorPCSampler* predictorPCSampler = dynamic_cast<TCpredictorPCSampler*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
   return predictorPCSampler->get_pred_pcsampler_lifetime(pc);
}

void pred_pcsampler_addloadpc(Time access_time, int address, int pc) {
   TCpredictorPCSampler* predictorPCSampler = dynamic_cast<TCpredictorPCSampler*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
   return predictorPCSampler->addLoadPC(access_time, address, pc);
}

void pred_pcsampler_endlifetime(Time access_time, int address) {
   TCpredictorPCSampler* predictorPCSampler = dynamic_cast<TCpredictorPCSampler*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
   return predictorPCSampler->endLifetime(access_time, address);
}

void pred_pcsampler_expiredloadhit(Time access_time, int address, int pc) {
   TCpredictorPCSampler* predictorPCSampler = dynamic_cast<TCpredictorPCSampler*>(g_system_ptr->getTCpredicterContainer()->getPredictor());
   return predictorPCSampler->expiredLoadHit(access_time, address, pc);
}
