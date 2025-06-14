#include <linux/netdevice.h>

#include "netdev.h"

#define NSMOCK_MODULE "nsmock: "
#define NSMOCK_IFACE_NAME "nsmock"

static struct net_device* get_nsmock_netdev(void);
static int nsmock_netdev_open(struct net_device *dev);
static int nsmock_netdev_stop(struct net_device *dev);
static struct net_device_stats* nsmock_netdev_get_stats_from(struct net_device *dev);

static const struct net_device_ops nsmock_netdev_ops = {
    .ndo_open = nsmock_netdev_open,
    .ndo_stop = nsmock_netdev_stop,
    .ndo_get_stats = nsmock_netdev_get_stats_from,
};

int nsmock_netdev_init(void) {
    printk(KERN_DEBUG NSMOCK_MODULE "netdev init()\n");
    struct net_device *net_dev;

    net_dev = alloc_netdev(sizeof(struct nsmock_netdev_priv), NSMOCK_IFACE_NAME "%d", NET_NAME_UNKNOWN, ether_setup);
    if (!net_dev) {
        printk(KERN_ERR NSMOCK_MODULE "failed to allocate net device\n");
        return -ENOMEM;
    }

    net_dev->netdev_ops = &nsmock_netdev_ops;

    if (register_netdev(net_dev)) {
        printk(KERN_ERR NSMOCK_MODULE "failed to register net device\n");
        free_netdev(net_dev);
        return -ENODEV;
    }

    printk(KERN_INFO NSMOCK_MODULE "registered network device\n");
    return 0;
}

void nsmock_netdev_exit(void) {
    printk(KERN_DEBUG NSMOCK_MODULE "netdev exit()\n");
    struct net_device* net_dev = get_nsmock_netdev();
    if (net_dev == NULL) {
        printk(KERN_DEBUG NSMOCK_MODULE "found nsmock net device is NULL\n");
        return;
    }
    unregister_netdev(net_dev);
    free_netdev(net_dev);
    printk(KERN_INFO NSMOCK_MODULE "unregistered network device\n");
}

static int nsmock_netdev_open(struct net_device *dev) {
    netif_start_queue(dev);
    printk(KERN_INFO NSMOCK_MODULE "opened\n");
    return 0;
}

static int nsmock_netdev_stop(struct net_device *dev) {
    netif_stop_queue(dev);
    printk(KERN_INFO NSMOCK_MODULE "stopped\n");
    return 0;
}

static bool starts_with(const char *str, const char *prefix)
{
	return !strncmp(prefix, str, strlen(prefix));
}

static struct net_device* get_nsmock_netdev(void) {
    struct net_device *net_dev = first_net_device(&init_net);
    
    while (net_dev) {
        if (starts_with(net_dev->name, NSMOCK_IFACE_NAME)) {
            return net_dev;
        }
        net_dev = next_net_device(net_dev);
    }
    return NULL;
}

static struct net_device_stats* nsmock_netdev_get_stats_from(struct net_device *dev) {
    struct nsmock_netdev_priv *priv = netdev_priv(dev);
    if (priv == NULL) {
        printk(KERN_NOTICE NSMOCK_MODULE "tried to get netdev_priv and received NULL\n");
        return NULL;
    }
    return &priv->stats;
}

struct net_device_stats* nsmock_netdev_get_stats(void) {
    struct net_device *dev = get_nsmock_netdev();
    if (dev == NULL) {
        printk(KERN_NOTICE NSMOCK_MODULE "tried to get netdev stats and received NULL device\n");
        return NULL;
    }
    return nsmock_netdev_get_stats_from(dev);
}
