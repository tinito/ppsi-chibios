#include <ppsi/ppsi.h>
#include "ptpdump.h"

static int chibios_open_ch(struct pp_instance *ppi, char *ifname)
{
	if (!ppi->ethernet_mode) {
		pp_printf("%s: can't init UDP channels yet\n", __func__);
		return -1;
	}
	pp_printf("%s: opening %s\n", __func__, ifname);

//	enc28_init();

//	memcpy(NP(ppi)->ch[PP_NP_GEN].addr, enc28_cfg.macaddr, 6);
//	memcpy(NP(ppi)->ch[PP_NP_EVT].addr, enc28_cfg.macaddr, 6);

//	NP(ppi)->ch[PP_NP_GEN].custom = &ETHD1;
//	NP(ppi)->ch[PP_NP_EVT].custom = &enc28_dev;

	return 0;
}

static int chibios_net_exit(struct pp_instance *ppi)
{
//	if (NP(ppi)->ch[PP_NP_EVT].custom)
//		enc28_exit();
	NP(ppi)->ch[PP_NP_EVT].custom = NULL;
	NP(ppi)->ch[PP_NP_GEN].custom = NULL;
	return 0;
}

/* This function must be able to be called twice, and clean-up internally */
static int chibios_net_init(struct pp_instance *ppi)
{

	chibios_net_exit(ppi);

	/* The buffer is inside ppi, but we need to set pointers and align */
	pp_prepare_pointers(ppi);

	if (!ppi->ethernet_mode) {
		pp_printf("%s: can't init UDP channels yet\n", __func__);
		return -1;
	}

	pp_diag(ppi, frames, 1, "%s: Ethernet mode\n", __func__);

	/* raw sockets implementation always use gen socket */
	return chibios_open_ch(ppi, ppi->iface_name);
}

static int chibios_net_recv(struct pp_instance *ppi, void *pkt, int len,
			    TimeInternal *t)
{
	int ret;

//	ret = enc28_recv(NP(ppi)->ch[PP_NP_GEN].custom, pkt, len);
	if (t && ret > 0)
		ppi->t_ops->get(ppi, t);

	if (ret > 0 && pp_diag_allow(ppi, frames, 2))
		dump_1588pkt("recv: ", pkt, ret, t);
	return ret;
}

static int chibios_net_send(struct pp_instance *ppi, void *pkt, int len,
			    TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct ethhdr {
		unsigned char   h_dest[6];
		unsigned char   h_source[6];
		uint16_t        h_proto;
	} __attribute__((packed)) *hdr = pkt;
	int ret;

	hdr->h_proto = htons(ETH_P_1588);
	memcpy(hdr->h_dest, PP_MCAST_MACADDRESS, 6);
	/* raw socket implementation always uses gen socket */
	memcpy(hdr->h_source, NP(ppi)->ch[PP_NP_GEN].addr, 6);

	if (t)
		ppi->t_ops->get(ppi, t);

//	ret = enc28_send(NP(ppi)->ch[chtype].custom, pkt, len);
	if (ret > 0 && pp_diag_allow(ppi, frames, 2))
		dump_1588pkt("send: ", pkt, len, t);
	return ret;
}

struct pp_network_operations chibios_net_ops = {
	.init = chibios_net_init,
	.exit = chibios_net_exit,
	.recv = chibios_net_recv,
	.send = chibios_net_send,
};
