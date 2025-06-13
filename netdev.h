#include <linux/netdevice.h>

struct nsmock_netdev_priv {
    struct net_device_stats stats;
};

int nsmock_netdev_init(void);

void nsmock_netdev_exit(void);

struct net_device_stats* nsmock_netdev_get_stats(void);