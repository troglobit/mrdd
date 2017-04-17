/* Simple test mrdisc solicitation agent
 *
 * Copyright (c) 2017  Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/socket.h>

#define MC_ALL_SNOOPERS      "224.0.0.106"
#define IGMP_MRDISC_ANNOUNCE 0x30
#define IGMP_MRDISC_SOLICIT  0x31
#define IGMP_MRDISC_TERM     0x32

unsigned short in_cksum(unsigned short *addr, int len);

static int open_socket(char *ifname)
{
	char loop;
	int sd, val, rc;
	struct ifreq ifr;
	unsigned char ra[4] = { IPOPT_RA, 0x04, 0x00, 0x00 };

	sd = socket(AF_INET, SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_IGMP);
	if (sd < 0)
		err(1, "Cannot open socket");

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
	if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
		if (ENODEV == errno) {
			warnx("Not a valid interface, %s, skipping ...", ifname);
			close(sd);
			return -1;
		}

		err(1, "Cannot bind socket to interface %s", ifname);
	}

	/* struct ip_mreqn mreq; */
	/* memset(&mreq, 0, sizeof(mreq)); */
	/* mreq.imr_multiaddr.s_addr = inet_addr(MC_ALL_SNOOPERS); */
	/* mreq.imr_ifindex = if_nametoindex(ifname); */
        /* if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) */
	/* 	err(1, "Failed joining group %s", MC_ALL_SNOOPERS); */

	val = 1;
	rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &val, sizeof(val));
	if (rc < 0)
		err(1, "Cannot set TTL");

	loop = 0;
	rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	if (rc < 0)
		err(1, "Cannot disable MC loop");

	rc = setsockopt(sd, IPPROTO_IP, IP_OPTIONS, &ra, sizeof(ra));
	if (rc < 0)
		err(1, "Cannot set IP OPTIONS");

	return sd;
}

static void compose_addr(struct sockaddr_in *sin, char *group)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = inet_addr(group);
}

static int send_message(int sd, uint8_t type, uint8_t interval)
{
	ssize_t num;
	struct igmp igmp;
	struct sockaddr dest;

	memset(&igmp, 0, sizeof(igmp));
	igmp.igmp_type = type;
	igmp.igmp_code = interval;
	igmp.igmp_cksum = in_cksum((unsigned short *)&igmp, sizeof(igmp));

	compose_addr((struct sockaddr_in *)&dest, MC_ALL_SNOOPERS);

	num = sendto(sd, &igmp, sizeof(igmp), 0, &dest, sizeof(dest));
	if (num < 0)
		err(1, "Cannot send IGMP control message");

	return 0;
}

int main(int argc, char *argv[])
{
	int sd;

	if (argc < 2)
		return 1;

	sd = open_socket(argv[1]);
	if (sd < 0)
		return 1;

	return send_message(sd, IGMP_MRDISC_SOLICIT, 0);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
