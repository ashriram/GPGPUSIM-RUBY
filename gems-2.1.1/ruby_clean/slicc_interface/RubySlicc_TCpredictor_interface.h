#ifndef RUBYSLICC_TCPREDICTOR_INTERFACE_H
#define RUBYSLICC_TCPREDICTOR_INTERFACE_H

#include "Global.h"
#include "TCpredictor.h"
#include "IntPairSet.h"
#include "NetDest.h"

/*
 * Cache Lease coherence specific functions
 */

Time get_lease_fixed();

bool is_pred_fixed();
bool is_pred_global();
bool is_pred_perL2();
bool is_pred_pcadaptive();
bool is_pred_PCfixed();
bool is_pred_Addrfixed();
bool is_pred_oracle();
bool is_pred_pcsampler();

int get_pred_global_writeunexpired();
int get_pred_global_l2evict();
int get_pred_global_expirehit();
Time get_pred_global_lifetime();
void pred_global_decrease_lease(int delta, int min, int max);
void pred_global_increase_lease(int delta, int min, int max);


int get_pred_perL2_writeunexpired();
int get_pred_perL2_l2evict();
int get_pred_perL2_expirehit();
Time get_pred_perL2_lifetime(NodeID L2BankId);
void pred_perL2_decrease_lease(NodeID L2BankId, int delta, int min, int max);
void pred_perL2_increase_lease(NodeID L2BankId, int delta, int min, int max);
void pred_perL2_decrease_lease_all(int delta, int min, int max);
void pred_perL2_increase_lease_all(int delta, int min, int max);


Time get_pred_pcadaptive_lifetime(int pc, int core_id);
void pred_pcadaptive_expiredhit(IntPairSet pcsharers, IntPair reader);
void pred_pcadaptive_unexpiredwrite(IntPairSet pcsharers, IntPair writer);
void pred_pcadaptive_unexpiredatomic(IntPairSet pcsharers, IntPair writer);
void pred_pcadaptive_l2evict(IntPairSet pcsharers);


Time get_pred_PCfixed_lifetime(int pc);

Time get_pred_Addrfixed_lifetime(int addr);

Time get_pred_oracle_lifetime();
NetDest pred_oracle_get_sharers(IntPairSet pcsharers);

Time get_pred_pcsampler_lifetime(int pc);
void pred_pcsampler_addloadpc(Time access_time, int address, int pc);
void pred_pcsampler_endlifetime(Time access_time, int address);
void pred_pcsampler_expiredloadhit(Time access_time, int address, int pc);

#endif // RUBYSLICC_TCPREDICTOR_INTERFACE_H
