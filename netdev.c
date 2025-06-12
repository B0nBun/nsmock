#include <linux/netdevice.h>

#include "netdev.h"

static char* ifacesim_driver_name = "ifacesim_netdev";

static int ifacesim_netdev_open(struct net_device *dev) {
    netif_start_queue(dev);
    printk(KERN_INFO "%s: opened\n", ifacesim_driver_name);
    return 0;
}

static int ifacesim_netdev_stop(struct net_device *dev) {
    netif_stop_queue(dev);
    printk(KERN_INFO "%s: stopped\n", ifacesim_driver_name);
    return 0;
}

// Transmit function (not implemented)
static netdev_tx_t ifacesim_netdev_start_xmit(struct sk_buff *skb, struct net_device *dev) {
    dev_kfree_skb(skb);
    return NETDEV_TX_OK;
}

static struct net_device* get_ifacesim_netdev(void) {
    struct net_device *net_dev = first_net_device(&init_net);
    
    while (net_dev) {
        if (strcmp(net_dev->name, "ifacesim0") == 0) {
            return net_dev;
        }
        net_dev = next_net_device(net_dev);
    }
    return NULL;
}

static struct net_device_stats* ifacesim_netdev_get_stats_from(struct net_device *dev) {
    struct ifacesim_netdev_priv *priv = netdev_priv(dev);
    if (priv == NULL) {
        return NULL;
    }
    return &priv->stats;
}

struct net_device_stats* ifacesim_netdev_get_stats(void) {
    struct net_device *dev = get_ifacesim_netdev();
    if (dev == NULL) {
        printk(KERN_NOTICE "%s: tried to get netdev stats and received NULL device\n", ifacesim_driver_name);
        return NULL;
    }
    return ifacesim_netdev_get_stats_from(dev);
}

static const struct net_device_ops ifacesim_netdev_ops = {
    .ndo_open = ifacesim_netdev_open,
    .ndo_stop = ifacesim_netdev_stop,
    .ndo_start_xmit = ifacesim_netdev_start_xmit,
    .ndo_get_stats = ifacesim_netdev_get_stats_from,
};

int ifacesim_netdev_init(void) {
    struct net_device *net_dev;

    net_dev = alloc_netdev(sizeof(struct ifacesim_netdev_priv), "ifacesim%d", NET_NAME_UNKNOWN, ether_setup);
    if (!net_dev) {
        printk(KERN_ERR "%s: failed to allocate net device\n", ifacesim_driver_name);
        return -ENOMEM;
    }

    net_dev->netdev_ops = &ifacesim_netdev_ops;

    if (register_netdev(net_dev)) {
        printk(KERN_ERR "%s: failed to register net device\n", ifacesim_driver_name);
        free_netdev(net_dev);
        return -ENODEV;
    }

    printk(KERN_INFO "%s: registered network device\n", ifacesim_driver_name);

    // TODO
    struct ifacesim_netdev_priv *priv = netdev_priv(net_dev);
    priv->stats.rx_packets = 111;
    priv->stats.tx_packets = 222;
    priv->stats.rx_errors = 11;
    priv->stats.tx_errors = 22;
    return 0;
}

void ifacesim_netdev_exit(void) {
    struct net_device* net_dev = get_ifacesim_netdev();
    if (net_dev == NULL) {
        return;
    }
    unregister_netdev(net_dev);
    free_netdev(net_dev);
    printk(KERN_INFO "%s: unregistered network device\n", ifacesim_driver_name);
}
