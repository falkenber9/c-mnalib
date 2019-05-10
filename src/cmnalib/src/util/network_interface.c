#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "cmnalib/network_interface.h"

#include "cmnalib/logger.h"

int set_if_flags(char *ifname, short flags, int skfd)
{
	struct ifreq ifr;
	int res = 0;

	ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ-1] = 0;

	res = ioctl(skfd, SIOCSIFFLAGS, &ifr);
	if (res < 0) {
        int saved_errno = errno;
        ERROR("Interface '%s': Error: SIOCSIFFLAGS failed: %s\n",
			ifname, strerror(saved_errno));
	} else {
        INFO("Interface '%s': flags set to %04X.\n", ifname, flags);
	}

	return res;
}

int set_if_up(char *ifname, short flags)
{
    int result = 0;
    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    result = set_if_flags(ifname, flags | IFF_UP, skfd);
    close(skfd);
    return result;
}

int set_if_down(char *ifname, short flags)
{
    int result = 0;
    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    result = set_if_flags(ifname, flags & ~IFF_UP, skfd);
    close(skfd);
    return result;
}
