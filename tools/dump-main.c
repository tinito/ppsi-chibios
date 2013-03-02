#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>

#include <ieee1588_types.h> /* from ../include/ppsi */
#include "decent_types.h"
#include "ptpdump.h"

#ifndef ETH_P_1588
#define ETH_P_1588     0x88F7
#endif

int main(int argc, char **argv)
{
	int sock;
	struct packet_mreq req;
	struct ifreq ifr;
	char *ifname = "eth0";

	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0) {
		fprintf(stderr, "%s: socket(): %s\n", argv[0], strerror(errno));
		exit(1);
	}
	if (argc > 1)
		ifname = argv[1];

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		fprintf(stderr, "%s: ioctl(GETIFINDEX(%s)): %s\n", argv[0],
			ifname, strerror(errno));
		exit(1);
	}
	fprintf(stderr, "Dumping PTP packets from interface \"%s\"\n", ifname);

	memset(&req, 0, sizeof(req));
	req.mr_ifindex = ifr.ifr_ifindex;
	req.mr_type = PACKET_MR_PROMISC;

	if (setsockopt(3, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
		       &req, sizeof(req)) < 0) {
		fprintf(stderr, "%s: set promiscuous(%s): %s\n", argv[0],
			ifname, strerror(errno));
		exit(1);
	}

	/* Ok, now we are promiscuous. Just read stuff forever */
	while(1) {
		struct ethhdr *eth;
		struct iphdr *ip;
		static unsigned char prev[1500];
		static int prevlen;
		unsigned char buf[1500];

		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);
		int len;

		len = recvfrom(sock, buf, sizeof(buf), MSG_TRUNC,
			       (struct sockaddr *) &from, &fromlen);
		if (len > sizeof(buf))
			len = sizeof(buf);
		/* for some reasons, we receive it three times, check dups */
		if (len == prevlen && !memcmp(buf, prev, len))
			continue;
		memcpy(prev, buf, len); prevlen = len;

		/* now only print ptp packets */
		if (len < ETH_HLEN)
			continue;
		eth = (void *)buf;
		ip = (void *)(buf + ETH_HLEN);
		switch(ntohs(eth->h_proto)) {
		case ETH_P_IP:
			if (len < ETH_HLEN + sizeof(*ip))
				continue;
			if (ip->protocol != IPPROTO_UDP)
				continue;
			dump_udppkt(buf, len);
			break;
		case ETH_P_1588:
			dump_1588pkt(buf, len);
			break;
		default:
			continue;
		}
		fflush(stdout);
	}

	return 0;
}












