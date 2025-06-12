#include <linux/netdevice.h>

struct ifacesim_netdev_priv {
    struct net_device_stats stats;
};

int ifacesim_netdev_init(void);

void ifacesim_netdev_exit(void);

struct net_device_stats* ifacesim_netdev_get_stats(void);