#include <linux/kobject.h>
#include <linux/minmax.h>

#include "sysfs.h"
#include "netdev.h"

// TODO: Place some debug/info logs
// TODO: Place some BUG_ON, WARN_ON macros
// TODO: Rename 'ifacesim' to 'nsmock' or something
// TODO: Move the sysfs from /sys/kernel to /sys/net or something even more appropriate

static char* ifacesim_driver_name = "ifacesim_netdev";

struct kobject ifacesim_kobj;

struct ifacesim_stat {
    char* name;
    size_t stat_offset;
    struct attribute attr;
};

#define STAT(member) { .name = #member, .stat_offset = offsetof(struct net_device_stats, member) }

static struct ifacesim_stat ifacesim_stats[] = {
    STAT(rx_packets),
	STAT(tx_packets),
	STAT(rx_bytes),
	STAT(tx_bytes),
	STAT(rx_errors),
	STAT(tx_errors),
	STAT(rx_dropped),
	STAT(tx_dropped),
	STAT(multicast),
	STAT(collisions),
	STAT(rx_length_errors),
	STAT(rx_over_errors),
	STAT(rx_crc_errors),
	STAT(rx_frame_errors),
	STAT(rx_fifo_errors),
	STAT(rx_missed_errors),
	STAT(tx_aborted_errors),
	STAT(tx_carrier_errors),
	STAT(tx_fifo_errors),
	STAT(tx_heartbeat_errors),
	STAT(tx_window_errors),
	STAT(rx_compressed),
	STAT(tx_compressed),
};

#undef STAT

#define IFACESIM_STATS_LEN (sizeof(ifacesim_stats) / sizeof(ifacesim_stats[0]))

static int is_attr_name(struct attribute *attr, char *name) {
    size_t len = strlen(name);
    return !strncmp(attr->name, name, len);
}

static ssize_t ifacesim_sysfs_show(struct kobject *kobj, struct attribute *attr, char *buf) {
    struct net_device_stats* stats = ifacesim_netdev_get_stats();
    if (stats == NULL) {
        return 0;
    }
    for (size_t i = 0; i < IFACESIM_STATS_LEN; i ++) {
        if (is_attr_name(attr, ifacesim_stats[i].name)) {
            size_t offset = ifacesim_stats[i].stat_offset;
            unsigned long* stat = (void*)((uintptr_t)stats + offset);
            return sysfs_emit(buf, "%ld\n", *stat);
        }
    }
    return 0;
}

static ssize_t ifacesim_sysfs_store(struct kobject *kobj, struct attribute *attr, const char* buf, size_t count) {
    struct net_device_stats* stats = ifacesim_netdev_get_stats();
    if (stats == NULL) {
        return count;
    }
    for (size_t i = 0; i < IFACESIM_STATS_LEN; i ++) {
        if (is_attr_name(attr, ifacesim_stats[i].name)) {
            size_t offset = ifacesim_stats[i].stat_offset;
            unsigned long* stat = (void*)((uintptr_t)stats + offset);
            int res = kstrtoul(buf, 10, stat);
            if (res < 0) {
                printk(KERN_ERR "%s: failed to parse size_t from string '%.*s'", ifacesim_driver_name, (int)count, buf);
                return (ssize_t)res;
            }
            return count;
        }
    }
    return count;
}

static void ifacesim_sysfs_release(struct kobject *kobj) {
    printk(KERN_INFO "%s: released kobject\n", ifacesim_driver_name);
}

// Filled out during module init
static struct attribute *ifacesim_attrs[IFACESIM_STATS_LEN + 1] = {};

static struct attribute_group ifacesim_group = {
    .attrs = ifacesim_attrs,
};

const static struct attribute_group* ifacesim_groups[] = {
    &ifacesim_group,
    NULL,
};

static struct sysfs_ops ifacesim_sysfs_ops = {
    .show = ifacesim_sysfs_show,
    .store = ifacesim_sysfs_store,
};

static const struct kobj_type ifacesim_ktype = {
    .sysfs_ops = &ifacesim_sysfs_ops,
    .release = ifacesim_sysfs_release,
    .default_groups = ifacesim_groups,
};

int ifacesim_sysfs_init(void) {
    for (size_t i = 0; i < IFACESIM_STATS_LEN; i ++) {
        struct attribute* attr = &ifacesim_stats[i].attr;
        attr->name = ifacesim_stats[i].name;
        attr->mode = 0664;
        ifacesim_attrs[i] = attr;
    }
    ifacesim_attrs[IFACESIM_STATS_LEN] = NULL;

    int ret = kobject_init_and_add(&ifacesim_kobj, &ifacesim_ktype, kernel_kobj, "%s", "ifacesim");
    if (!ret) {
        printk(KERN_ERR "%s: failed to init and add kobject\n", ifacesim_driver_name);
        return ret;
    }
    return 0;
}

void ifacesim_sysfs_exit(void) {
    kobject_put(&ifacesim_kobj);
    printk(KERN_INFO "%s: exit\n", ifacesim_driver_name);
}
