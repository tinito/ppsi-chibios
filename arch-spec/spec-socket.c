/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#include <pptp/pptp.h>
#include "spec.h"
#include "include/gpio.h"

int spec_errno;

/* This function should init the minic and get the mac address */
int spec_open_ch(struct pp_instance *ppi)
{
	uint8_t fake_addr[] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50};

	ep_init(fake_addr);
	ep_enable(1, 1);
	minic_init();

	memcpy(NP(ppi)->ch[PP_NP_GEN].addr, fake_addr, 6);
	memcpy(NP(ppi)->ch[PP_NP_EVT].addr, fake_addr, 6);

	return 0;
}

/* To receive and send packets, we call the minic low-level stuff */
int spec_recv_packet(struct pp_instance *ppi, void *pkt, int len,
		     TimeInternal *t)
{
	static int led;

	led ^= 1; /* blink one led at each rx event */
	gpio_out(GPIO_PIN_LED_LINK, led);
	return minic_rx_frame(pkt, pkt + 14, len - 14, NULL);
}

int spec_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype)
{
	static int led;

	led ^= 1; /* blink the other led at each tx event */
	gpio_out(GPIO_PIN_LED_STATUS, led);
	return minic_tx_frame(pkt, pkt + 14, len, NULL);
}

int pp_recv_packet(struct pp_instance *ppi, void *pkt, int len, TimeInternal *t)
	__attribute__((alias("spec_recv_packet")));
int pp_send_packet(struct pp_instance *ppi, void *pkt, int len, int chtype)
	__attribute__((alias("spec_send_packet")));
