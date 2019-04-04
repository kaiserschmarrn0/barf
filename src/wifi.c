#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "wifi.h"

typedef struct nl_req_s {
	struct nlmsghdr hdr;
	struct rtgenmsg gen;
} nl_req_t;

static component *this;

static char buf[8192];
static struct msghdr msg;
static struct sockaddr_nl local;
static struct iovec iov;

int wifi_init(component *ref) {
	this = ref;

	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if (fd < 0) {
		fprintf(stderr, "Wifi: couldn't open netlink socket: %s\n", strerror(errno));
		return 1;
	}

	int pid = getpid();

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups = RTMGRP_LINK;
	local.nl_pid = pid;

	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	msg.msg_name = &local;
	msg.msg_namelen = sizeof(local);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (bind(fd, (struct sockaddr*)&local, sizeof(local)) < 0) {
		fprintf(stderr, "Wifi: couldn't bind netlink socket: %s\n", strerror(errno));
		close(fd);
		return 1;
	}

	/* request message about current state */

	struct sockaddr_nl kernel;
	memset(&kernel, 0, sizeof(kernel));
	kernel.nl_family = AF_NETLINK;

	nl_req_t req;
	memset(&req, 0, sizeof(req));
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
	req.hdr.nlmsg_type = RTM_GETLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_pid = pid;
	req.gen.rtgen_family = AF_PACKET;

	struct iovec io;
	io.iov_base = &req;
	io.iov_len = req.hdr.nlmsg_len;

	struct msghdr kernel_msg;
	memset(&kernel_msg, 0, sizeof(kernel_msg));
	kernel_msg.msg_iov = &io;
	kernel_msg.msg_iovlen = 1;
	kernel_msg.msg_name = &kernel;
	kernel_msg.msg_namelen = sizeof(kernel);

	if (sendmsg(fd, (struct msghdr *)&kernel_msg, 0) <= 0) {
		fprintf(stderr, "Wifi: couldn't sent rtnl message: %s\n", strerror(errno));
		return 1;
	}

	this->fd = fd;

	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	draw_block(this, "\uf08a", "\0");

	return 0;
}

int wifi_run(void) {
	ssize_t status = recvmsg(this->fd, &msg, MSG_DONTWAIT);

	if (status < 0) {
		fprintf(stderr, "Wifi: bad message from netlink socket: %s\n", strerror(errno));
		return 1;
	}

	if (msg.msg_namelen != sizeof(local)) {
		fprintf(stderr, "Wifi: bad name from netlink socket: %s\n", strerror(errno));
		return 1;
	}

	struct nlmsghdr *h;

	for (h = (struct nlmsghdr *)buf; status >= (ssize_t)sizeof(*h);) {
		int len = h->nlmsg_len;
		int l = len - sizeof(*h);

		if ((l < 0) || (len > status)) {
			fprintf(stderr, "Wifi: invalid message length: %i\n", len);
			return 1;
		}

		struct ifinfomsg *ifi = (struct ifinfomsg *) NLMSG_DATA(h);

		struct rtattr *tb[IFLA_MAX + 1];
		struct rtattr *rta = IFLA_RTA(ifi);
		
		memset(tb, 0, sizeof(struct rtattr *) * (IFLA_MAX + 1));
		while (RTA_OK(rta, h->nlmsg_len)) {
			if (rta->rta_type <= IFLA_MAX) {
				tb[rta->rta_type] = rta;
			}
			rta = RTA_NEXT(rta, h->nlmsg_len);
		}

		if ((ifi->ifi_flags & IFF_UP) && (ifi->ifi_flags & IFF_RUNNING)) {
			draw_block(this, "\uf004", "\0");
		} else {
			draw_block(this, "\uf08a", "\0");
		}

		status -= NLMSG_ALIGN(len);

		h = (struct nlmsghdr *)((char *)h + NLMSG_ALIGN(len));
	}

	return 0;
}

void wifi_clean(void) {
	close(this->fd);
}
