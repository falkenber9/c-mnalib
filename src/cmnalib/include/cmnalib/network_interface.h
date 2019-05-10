#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int set_if_up(char *ifname, short flags);
int set_if_down(char *ifname, short flags);

#ifdef __cplusplus
}
#endif
