#include "NetworkPowerArea.h"

#include <cstring>

#include "GarnetNetwork_d.h"
#include "Router_d.h"
#include "NetworkLink_d.h"

#include "SIM_util.h"
#include "SIM_clock.h"

RouterPowerArea_d::RouterPowerArea_d() : 
	m_p_dyn_in_buf(0), m_p_static_in_buf(0),
	m_p_dyn_xbar(0), m_p_static_xbar(0),
	m_p_dyn_vc(0), m_p_static_vc(0),
	m_p_dyn_sw(0), m_p_static_sw(0),
	m_p_dyn_clock(0), m_p_static_clock(0),
	m_p_dyn_total(0), m_p_static_total(0),
	m_a_in_buf(0), m_a_xbar(0),
	m_a_vc(0), m_a_sw(0)
{}

RouterPowerArea_d::RouterPowerArea_d(const RouterPowerArea_d &rp) :
	m_p_dyn_in_buf(rp.m_p_dyn_in_buf), m_p_static_in_buf(rp.m_p_static_in_buf),
	m_p_dyn_xbar(rp.m_p_dyn_xbar), m_p_static_xbar(rp.m_p_static_xbar),
	m_p_dyn_vc(rp.m_p_dyn_vc), m_p_static_vc(rp.m_p_static_vc),
	m_p_dyn_sw(rp.m_p_dyn_sw), m_p_static_sw(rp.m_p_static_sw),
	m_p_dyn_clock(rp.m_p_dyn_clock), m_p_static_clock(rp.m_p_static_clock),
	m_p_dyn_total(rp.m_p_dyn_total), m_p_static_total(rp.m_p_static_total),
	m_a_in_buf(rp.m_a_in_buf), m_a_xbar(rp.m_a_xbar), m_a_vc(rp.m_a_vc), m_a_sw(rp.m_a_sw)
{}

void RouterPowerArea_d::AddPower(const RouterPowerArea_d rp)
{
	m_p_dyn_in_buf += rp.m_p_dyn_in_buf;
	m_p_static_in_buf += rp.m_p_static_in_buf;
	m_p_dyn_xbar += rp.m_p_dyn_xbar;
	m_p_static_xbar += rp.m_p_static_xbar;
	m_p_dyn_vc += rp.m_p_dyn_vc;
	m_p_static_vc += rp.m_p_static_vc;
	m_p_dyn_sw += rp.m_p_dyn_sw;
	m_p_static_sw += rp.m_p_static_sw;
	m_p_dyn_clock += rp.m_p_dyn_clock;
	m_p_static_clock += rp.m_p_static_clock;
	m_p_dyn_total += rp.m_p_dyn_total;
	m_p_static_total += rp.m_p_static_total;
	return;
}

void RouterPowerArea_d::AddArea(const RouterPowerArea_d ra)
{
	m_a_in_buf += ra.m_a_in_buf;
	m_a_xbar += ra.m_a_xbar;
	m_a_vc += ra.m_a_vc;
	m_a_sw += ra.m_a_sw;
	return;
}

void garnet_router_init(SIM_router_info_t *info, SIM_router_power_t *router_power, SIM_router_area_t *router_area, const int n_in, const int n_out, const int flit_width, const int n_v_channel, const int n_v_class, const int in_buf_set)
{
	memset(info, 0, sizeof(SIM_router_info_t));
	u_int line_width;
	int share_buf, outdrv;

	/* PHASE 1: set parameters */
	/* general parameters */
	info->n_in = n_in; //m_input_unit.size();
	info->n_total_in = info->n_in;
	info->n_out = n_out; //m_output_unit.size();
	info->n_total_out = info->n_out;
	info->flit_width = flit_width; //NetworkConfig::getFlitSize() * 8; //m_flit_width * 8; // flit width in bits

	/* virtual channel parameters */
	info->n_v_channel = n_v_channel; //NetworkConfig::getVCsPerClass(); //m_vc_per_vnet;
	//HACK, since we are only using 3 vnets now
	info->n_v_class = n_v_class; //3; //m_virtual_networks;
	//HACK, since we don't want it to be set by SIM_port.h
	//(info->in_buf_info).n_set = NetworkConfig::getBufferSize();

	/* shared buffer implies buffer has tags, so no virtual class -> no sharing */
	/* separate buffer & shared switch implies buffer has tri-state output driver, so no v class -> no sharing */
	if (info->n_v_class * info->n_v_channel > 1) {
		info->in_share_buf = PARM(in_share_buf);
		info->out_share_buf = PARM(out_share_buf);
		info->in_share_switch = PARM(in_share_switch);
		info->out_share_switch = PARM(out_share_switch);
	}
	else {
		info->in_share_buf = 0;
		info->out_share_buf = 0;
		info->in_share_switch = 0;
		info->out_share_switch = 0;
	}

	/* crossbar */
	info->crossbar_model = PARM(crossbar_model);
	info->degree = PARM(crsbar_degree);
	info->connect_type = PARM(connect_type);
	info->trans_type = PARM(trans_type);
	info->xb_in_seg = PARM(xb_in_seg);
	info->xb_out_seg = PARM(xb_out_seg);
	info->crossbar_in_len = PARM(crossbar_in_len);
	info->crossbar_out_len = PARM(crossbar_out_len);
	/* HACK HACK HACK */
	info->exp_xb_model = PARM(exp_xb_model);
	info->exp_in_seg = PARM(exp_in_seg);
	info->exp_out_seg = PARM(exp_out_seg);

	/* input buffer */
	info->in_buf = 1; //PARM(in_buf);
	info->in_buffer_model = PARM(in_buffer_type);
	outdrv = !info->in_share_buf && info->in_share_switch;
	SIM_array_init(&info->in_buf_info, 1, PARM(in_buf_rport), 1, in_buf_set, info->flit_width, outdrv, info->in_buffer_model);

	/* switch allocator input port arbiter */
	if ((info->n_v_class * info->n_v_channel) > 1) {
		if (info->sw_in_arb_model = PARM(sw_in_arb_model)) {
			if (PARM(sw_in_arb_model) == QUEUE_ARBITER) {
				SIM_array_init(&info->sw_in_arb_queue_info, 1, 1, 1, info->n_v_class*info->n_v_channel, SIM_logtwo(info->n_v_class*info->n_v_channel), 0, REGISTER);
				info->sw_in_arb_ff_model = SIM_NO_MODEL;
			}
			else
				info->sw_in_arb_ff_model = PARM(sw_in_arb_ff_model);
		}
		else
			info->sw_in_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		info->sw_in_arb_model = SIM_NO_MODEL;
		info->sw_in_arb_ff_model = SIM_NO_MODEL;
	}
	/* switch allocator output port arbiter */
	if (info->n_total_in > 2) {
		if (info->sw_out_arb_model = PARM(sw_out_arb_model)) {
			if (PARM(sw_out_arb_model) == QUEUE_ARBITER) {
				line_width = SIM_logtwo(info->n_total_in - 1);
				SIM_array_init(&info->sw_out_arb_queue_info, 1, 1, 1, info->n_total_in - 1, line_width, 0, REGISTER);
				info->sw_out_arb_ff_model = SIM_NO_MODEL;
			}
			else
				info->sw_out_arb_ff_model = PARM(sw_out_arb_ff_model);
		}
		else
			info->sw_out_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		info->sw_out_arb_model = SIM_NO_MODEL;
		info->sw_out_arb_ff_model = SIM_NO_MODEL;
	}

	/* virtual channel allocator type */
	if (info->n_v_channel > 1) {
		info->vc_allocator_type = PARM(vc_allocator_type);
	} 
	else
		info->vc_allocator_type = SIM_NO_MODEL;

	/* virtual channel allocator input port arbiter */
	if ( (info->n_v_channel > 1) && (info->n_in > 1)) {
		if (info->vc_in_arb_model = PARM(vc_in_arb_model)) {
			if (PARM(vc_in_arb_model) == QUEUE_ARBITER) { 
				SIM_array_init(&info->vc_in_arb_queue_info, 1, 1, 1, info->n_v_channel, SIM_logtwo(info->n_v_channel), 0, REGISTER);
				info->vc_in_arb_ff_model = SIM_NO_MODEL;
			}
			else
				info->vc_in_arb_ff_model = PARM(vc_in_arb_ff_model);
		}
		else
			info->vc_in_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		info->vc_in_arb_model = SIM_NO_MODEL;
		info->vc_in_arb_ff_model = SIM_NO_MODEL;
	}

	/* virtual channel allocator output port arbiter */
	if ((info->n_in > 1) && (info->n_v_channel > 1)) {
		if (info->vc_out_arb_model = PARM(vc_out_arb_model)) {
			if (PARM(vc_out_arb_model) == QUEUE_ARBITER) {
				line_width = SIM_logtwo((info->n_total_in - 1)*info->n_v_channel);
				SIM_array_init(&info->vc_out_arb_queue_info, 1, 1, 1, (info->n_total_in -1) * info->n_v_channel, line_width, 0, REGISTER);
				info->vc_out_arb_ff_model = SIM_NO_MODEL;
			}
			else
				info->vc_out_arb_ff_model = PARM(vc_out_arb_ff_model);
		}
		else
			info->vc_out_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		info->vc_out_arb_model = SIM_NO_MODEL;
		info->vc_out_arb_ff_model = SIM_NO_MODEL;
	}

	/*virtual channel allocation vc selection model */
	info->vc_select_buf_type = PARM(vc_select_buf_type);
	if (PARM(vc_allocator_type) == VC_SELECT && info->n_v_channel > 1 && info->n_in > 1) {
		info->vc_select_buf_type = PARM(vc_select_buf_type);
		SIM_array_init(&info->vc_select_buf_info, 1, 1, 1, info->n_v_channel, SIM_logtwo(info->n_v_channel), 0, info->vc_select_buf_type);
	}
	else {
		info->vc_select_buf_type = SIM_NO_MODEL;
	}


	/* redundant fields */
	if (info->in_buf) {
		if (info->in_share_buf)
			info->in_n_switch = info->in_buf_info.read_ports;
		else if (info->in_share_switch)
			info->in_n_switch = 1;
		else
			info->in_n_switch = info->n_v_class * info->n_v_channel;
	}
	else
		info->in_n_switch = 1;

	info->n_switch_in = info->n_in * info->in_n_switch;

	/* no buffering for local output ports */
	info->n_switch_out = info->n_out;

	/* clock related parameters */	
	info->pipeline_stages = PARM(pipeline_stages);
	info->H_tree_clock = PARM(H_tree_clock);
	info->router_diagonal = PARM(router_diagonal);

	/* PHASE 2: initialization */
	if(router_power){
		SIM_router_power_init(info, router_power);
	}

	if(router_area){
		SIM_router_area_init(info, router_area);
	}

	return;
}

double garnet_router_stat_energy(SIM_router_info_t *info, SIM_router_power_t *router, RouterPerformance_d *perf, RouterPowerArea_d *power, const double freq)
{
	double Eavg = 0, Eatomic, Estruct, Estatic = 0;
	double Pbuf = 0, Pxbar = 0, Pvc_arbiter = 0, Psw_arbiter = 0, Pclock = 0, Ptotal = 0;
	double Pbuf_static = 0, Pxbar_static = 0, Pvc_arbiter_static = 0, Psw_arbiter_static = 0, Pclock_static = 0;
	double Pbuf_dyn = 0, Pxbar_dyn = 0, Pvc_arbiter_dyn = 0, Psw_arbiter_dyn = 0, Pclock_dyn = 0;

	double e_in_buf_r, e_in_buf_w, e_xbar, e_vc_in_arb, e_vc_out_arb, e_sw_in_arb, e_sw_out_arb;
	int vc_allocator_enabled = 1;

	/* expected value computation */
	e_in_buf_r       = perf->e_in_buf_r;
	e_in_buf_w       = perf->e_in_buf_w;
	e_xbar           = perf->e_xbar;
	e_vc_in_arb      = perf->e_vc_local_arb;
	e_vc_out_arb     = perf->e_vc_global_arb;
	e_sw_in_arb      = perf->e_sw_local_arb;
	e_sw_out_arb     = perf->e_sw_global_arb;

	/* input buffers */
	if (info->in_buf) {
		Eavg += SIM_array_stat_energy(&info->in_buf_info, &router->in_buf, e_in_buf_r, e_in_buf_w, 0, NULL, 0); 
	}

	Pbuf_dyn = Eavg * freq;
	Pbuf_static = router->I_buf_static * Vdd * SCALE_S;
	Pbuf = Pbuf_dyn + Pbuf_static;

	/* main crossbar */
	if (info->crossbar_model) {
		Eavg += SIM_crossbar_stat_energy(&router->crossbar, 0, NULL, 0, e_xbar);
	}

	Pxbar_dyn = (Eavg * freq - Pbuf_dyn);
	Pxbar_static = router->I_crossbar_static * Vdd * SCALE_S;
	Pxbar = Pxbar_dyn + Pxbar_static;

	/* switch allocation (arbiter energy only) */
	/* input (local) arbiter for switch allocation*/
	if (info->sw_in_arb_model) {
		/* assume (info->n_v_channel*info->n_v_class)/2 vcs are making request at each arbiter */

		Eavg += SIM_arbiter_stat_energy(&router->sw_in_arb, &info->sw_in_arb_queue_info, (info->n_v_channel*info->n_v_class)/2, 0, NULL, 0) * e_sw_in_arb;
	}

	/* output (global) arbiter for switch allocation*/
	if (info->sw_out_arb_model) {
		/* assume (info->n_in)/2 request at each output arbiter */

		Eavg += SIM_arbiter_stat_energy(&router->sw_out_arb, &info->sw_out_arb_queue_info, info->n_in / 2, 0, NULL, 0) * e_sw_out_arb;
	}

	if(info->sw_out_arb_model || info->sw_out_arb_model){
		Psw_arbiter_dyn = Eavg * freq - Pbuf_dyn - Pxbar_dyn;
		Psw_arbiter_static = router->I_sw_arbiter_static * Vdd * SCALE_S;
		Psw_arbiter = Psw_arbiter_dyn + Psw_arbiter_static;
	}

	/* virtual channel allocation (arbiter energy only) */
	/* HACKs:
	 *   - assume 1 header flit in every 5 flits for now, hence * 0.2  */

	if(info->vc_allocator_type == ONE_STAGE_ARB && info->vc_out_arb_model  ){
		/* one stage arbitration (vc allocation)*/

		/* assume for each active arbiter, there is 2 requests on average (should use expected value from simulation) */	
		Eavg += SIM_arbiter_stat_energy(&router->vc_out_arb, &info->vc_out_arb_queue_info, 1, 0, NULL, 0) * e_vc_in_arb;
	}
	else if(info->vc_allocator_type == TWO_STAGE_ARB && info->vc_in_arb_model && info->vc_out_arb_model){
		/* first stage arbitration (vc allocation)*/
		if (info->vc_in_arb_model) {
			/* assume an active arbiter has n_v_channel/2 requests on average (should use expected value from simulation) */
			Eavg += SIM_arbiter_stat_energy(&router->vc_in_arb, &info->vc_in_arb_queue_info, info->n_v_channel/2, 0, NULL, 0) * e_vc_in_arb; 
		}

		/* second stage arbitration (vc allocation)*/
		if (info->vc_out_arb_model) {
			/* assume for each active arbiter, there is 2 requests on average (should use expected value from simulation) */
			Eavg += SIM_arbiter_stat_energy(&router->vc_out_arb, &info->vc_out_arb_queue_info, 2, 0, NULL, 0) * e_vc_out_arb;
		}
	}
	else if(info->vc_allocator_type == VC_SELECT && info->n_v_channel > 1 && info->n_in > 1){
		double n_read = e_vc_in_arb;
		double n_write = e_vc_in_arb;
		Eavg += SIM_array_stat_energy(&info->vc_select_buf_info, &router->vc_select_buf, n_read , n_write, 0, NULL, 0);

	}
	else{
		vc_allocator_enabled = 0; //set to 0 means no vc allocator is used
	}

	if((info->n_v_channel > 1) && vc_allocator_enabled){
		Pvc_arbiter_dyn = Eavg * freq - Pbuf_dyn - Pxbar_dyn - Psw_arbiter_dyn; 
		Pvc_arbiter_static = router->I_vc_arbiter_static * Vdd * SCALE_S;
		Pvc_arbiter = Pvc_arbiter_dyn + Pvc_arbiter_static;
	}

	/*router clock power (supported for 90nm and below) */
	if(PARM(TECH_POINT) <=90){
		Eavg += SIM_total_clockEnergy(info, router);
		Pclock_dyn = Eavg * freq - Pbuf_dyn - Pxbar_dyn - Pvc_arbiter_dyn - Psw_arbiter_dyn;
		Pclock_static = router->I_clock_static * Vdd * SCALE_S;
		Pclock = Pclock_dyn + Pclock_static;
	}

	/* static power */
	Estatic = router->I_static * Vdd * Period * SCALE_S;
	Eavg += Estatic;
	Ptotal = Eavg * freq;

	power->SetPower(Pbuf_dyn, Pbuf_static, Pxbar_dyn, Pxbar_static, Pvc_arbiter_dyn, Pvc_arbiter_static, Psw_arbiter_dyn, Psw_arbiter_static, Pclock_dyn, Pclock_static);
	return Eavg;
}

double Router_d::calculate_offline_power(SIM_router_info_t *info, SIM_router_power_t *router)
{
	RouterPerformance_d perf;

	double Eavg;
	double sim_cycles;
	sim_cycles = g_eventQueue_ptr->getTime() - m_network_ptr->getRubyStartTime();
	calculate_performance_numbers();
	//counts obtained from perf. simulator 
	perf.e_in_buf_r = (double )(buf_read_count/sim_cycles);
	perf.e_in_buf_w = (double )(buf_write_count/sim_cycles);
	perf.e_xbar = (double )(crossbar_count/sim_cycles);  
	perf.e_vc_local_arb = (double)(vc_local_arbit_count/sim_cycles);
	perf.e_vc_global_arb = (double)(vc_global_arbit_count/sim_cycles);
	perf.e_sw_local_arb = (double )(sw_local_arbit_count/sim_cycles);
	perf.e_sw_global_arb = (double )(sw_global_arbit_count/sim_cycles);

	Eavg = garnet_router_stat_energy(info, router, &perf, &m_router_powerarea, PARM(Freq));

	return Eavg;
}

double Router_d::calculate_power()
{
	SIM_router_power_t router;
	SIM_router_info_t router_info;
	double total_energy, total_power;

	garnet_router_init(&router_info, &router, NULL, m_input_unit.size(), m_output_unit.size(), NetworkConfig::getFlitSize() * 8, NetworkConfig::getVCsPerClass(), m_virtual_networks, NetworkConfig::getBufferSize());
	//printf("n_in = %d, n_out = %d, flit_width = %d, n_v_channel = %d, Inbuf = %d\n", m_input_unit.size(), m_output_unit.size(), NetworkConfig::getFlitSize() * 8, NetworkConfig::getVCsPerClass(), NetworkConfig::getBufferSize());
	total_energy = calculate_offline_power(&router_info, &router);
	total_power = total_energy * PARM(Freq);
	return total_power;
}

double Router_d::calculate_area()
{
	SIM_router_info_t router_info;
	SIM_router_area_t router;
	double total_area;

	garnet_router_init(&router_info, NULL, &router, m_input_unit.size(), m_output_unit.size(), NetworkConfig::getFlitSize() * 8, NetworkConfig::getVCsPerClass(), m_virtual_networks, NetworkConfig::getBufferSize());
	m_router_powerarea.SetArea(router.buffer * (1e-12), router.crossbar * (1e-12), router.vc_allocator * (1e-12), router.sw_allocator * (1e-12));
	total_area = m_router_powerarea.GetAreaTotal();
	return total_area;
}

double NetworkLink_d::calculate_offline_power(double* static_power, double* dynamic_power)
{
	
	double sim_cycles = (double) (g_eventQueue_ptr->getTime() - m_net_ptr->getRubyStartTime());
	double e_link = (double) (m_link_utilized)/ sim_cycles;
    double E_link, P_link = 0;
    E_link = PARM_LINK_SWITCHING_FACTOR * LinkDynamicEnergyPerBitPerMeter(PARM_GENERIC_LINK_LENGTH, Vdd) * PARM_GENERIC_LINK_LENGTH * (m_flit_width * 8) * e_link;
    P_link = LinkLeakagePowerPerMeter(PARM_GENERIC_LINK_LENGTH, Vdd) * PARM_GENERIC_LINK_LENGTH * (m_flit_width * 8);
    *static_power = P_link;
    *dynamic_power = E_link * PARM_Freq;
    return (*static_power + *dynamic_power);
}

double NetworkLink_d::calculate_power(double* static_power, double* dynamic_power)
{
    double total_power;

    total_power = calculate_offline_power(static_power, dynamic_power);
    return total_power;
}

double NetworkLink_d::calculate_area()
{
    double larea = 0;
    larea = LinkArea(PARM_GENERIC_LINK_LENGTH, m_flit_width * 8);
    return larea;
}

