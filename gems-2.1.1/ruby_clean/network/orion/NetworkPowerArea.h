#ifndef _NETWORK_POWERAREA_H
#define _NETWORK_POWERAREA_H

#include <iostream>

using namespace std;

#include "SIM_parameter.h"
#include "SIM_router.h"
#include "SIM_link.h"

typedef struct
{
	double e_in_buf_r;
	double e_in_buf_w;
	double e_xbar;
	double e_vc_local_arb;
	double e_vc_global_arb;
	double e_sw_local_arb;
	double e_sw_global_arb;
} RouterPerformance_d;

class RouterPowerArea_d
{
public:
	RouterPowerArea_d();
	RouterPowerArea_d(const RouterPowerArea_d &rp);
	~RouterPowerArea_d(){};

	void AddPower(const RouterPowerArea_d rp);
	void AddArea(const RouterPowerArea_d ra);

	inline void SetPower(const double dib, const double sib, const double dxb, const double sxb, const double dvc, const double svc, const double dsw, const double ssw, const double dc, const double sc)
	{
		m_p_dyn_in_buf = dib;
		m_p_static_in_buf = sib;
		m_p_dyn_xbar = dxb;
		m_p_static_xbar = sxb;
		m_p_dyn_vc = dvc;
		m_p_static_vc = svc;
		m_p_dyn_sw = dsw;
		m_p_static_sw = ssw;
		m_p_dyn_clock = dc,
		m_p_static_clock = sc,
		m_p_dyn_total = dib + dxb + dvc + dsw + dc;
		m_p_static_total = sib + sxb + svc + ssw + sc;
		return;
	}

	inline void SetArea(const double inbuf, const double xbar, const double vc, const double sw)
	{
		m_a_in_buf = inbuf;
		m_a_xbar = xbar;
		m_a_vc = vc;
		m_a_sw = sw;
		m_a_total = m_a_in_buf + m_a_xbar + m_a_vc + m_a_sw;
		return;
	}

	inline double GetPowerDyn(){ return m_p_dyn_total; }
	inline double GetPowerStatic(){ return m_p_static_total; }
	inline double GetPowerTotal(){ return (m_p_dyn_total + m_p_static_total); }
	inline double GetAreaInBuf(){ return m_a_in_buf; }
	inline double GetAreaXbar(){ return m_a_xbar; }
	inline double GetAreaVC(){ return m_a_vc; }
	inline double GetAreaSW(){ return m_a_sw; }
	inline double GetAreaTotal(){ return m_a_total; }

	inline void PrintPower(ostream &out, double* total_power, double* static_power, double* dynamic_power)
	{
		out << "Router Dynamic Power:" << endl;
		out << "       Input buffer: " << m_p_dyn_in_buf << ", Crossbar: " << m_p_dyn_xbar << ", VC allocator: " << m_p_dyn_vc << ", SW allocator: " << m_p_dyn_sw << ", Clock: " << m_p_dyn_clock << ", Total: " << m_p_dyn_total << endl;
		out << "Router Static Power:" << endl;
		out << "       Input buffer: " << m_p_static_in_buf << ", Crossbar: " << m_p_static_xbar << ", VC allocator: " << m_p_static_vc << ", SW allocator: " << m_p_static_sw << ", Clock: " << m_p_static_clock << ", Total: " << m_p_static_total << endl;
		out << "Router Total Power: " << (m_p_dyn_total + m_p_static_total) << endl;
		*static_power = m_p_static_total;
		*dynamic_power = m_p_dyn_total;
		*total_power = m_p_dyn_total + m_p_static_total;
	}

	inline void PrintArea(ostream &out)
	{
		out << "Area:" << endl;
		out << "Input buffer: " << m_a_in_buf << ", Crossbar: " << m_a_xbar << ", VC allocator: " << m_a_vc << ", SW allocator: " << m_a_sw << endl;
	}

private:
	double m_p_dyn_in_buf;
	double m_p_static_in_buf;
	double m_p_dyn_xbar;
	double m_p_static_xbar;
	double m_p_dyn_vc;
	double m_p_static_vc;
	double m_p_dyn_sw;
	double m_p_static_sw;
	double m_p_dyn_clock;
	double m_p_static_clock;
	double m_p_dyn_total;
	double m_p_static_total;
private:
	double m_a_in_buf;
	double m_a_xbar;
	double m_a_vc;
	double m_a_sw;
	double m_a_total;
};

void garnet_router_init(SIM_router_info_t *info, SIM_router_power_t *router_power, SIM_router_area_t *router_area, const int n_in, const int n_out, const int flit_width, const int n_v_channel, const int n_v_class, const int in_buf_set);

double garnet_router_stat_energy(SIM_router_info_t *info, SIM_router_power_t *router, RouterPerformance_d *perf, RouterPowerArea_d *power, const double freq);

#endif

