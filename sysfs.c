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

struct ifacesim_stat_op {
    enum {
        STAT_OP_INC,
        STAT_OP_DEC,
        STAT_OP_SET,
    } type;
    unsigned long val;
};

static int ifacesim_parse_stat_op(const char* s, struct ifacesim_stat_op* op) {
    if (s == NULL || op == NULL) {
        return -EINVAL;
    }
    if (*s == '+') {
        s++;
        op->type = STAT_OP_INC;
    } else if (*s == '-') {
        s++;
        op->type = STAT_OP_DEC;
    } else {
        op->type = STAT_OP_SET;
    }
    unsigned long val = 0;
    int res = kstrtoul(s, 10, &val);
    if (res != 0) {
        return res;
    }
    op->val = val;
    return 0;
}

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

static ssize_t ifacesim_sysfs_show(struct kobject *kobj, struct attribute *attr, char *buf) {
    struct net_device_stats* stats = ifacesim_netdev_get_stats();
    if (stats == NULL) {
        return 0;
    }
    for (size_t i = 0; i < IFACESIM_STATS_LEN; i ++) {
        if (sysfs_streq(attr->name, ifacesim_stats[i].name)) {
            size_t offset = ifacesim_stats[i].stat_offset;
            unsigned long* stat = (void*)((uintptr_t)stats + offset);
            return sysfs_emit(buf, "%lu\n", *stat);
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
        if (sysfs_streq(attr->name, ifacesim_stats[i].name)) {
            size_t offset = ifacesim_stats[i].stat_offset;
            struct ifacesim_stat_op operation;
            int res = ifacesim_parse_stat_op(buf, &operation);
            if (res < 0) {
                printk(KERN_ERR "%s: failed to parse size_t from string '%.*s'", ifacesim_driver_name, (int)count, buf);
                return (ssize_t)res;
            }
            
            unsigned long* stat = (void*)((uintptr_t)stats + offset);
            unsigned long tmp = 0;
            if (operation.type == STAT_OP_INC) {
                if (check_add_overflow(*stat, operation.val, &tmp)) {
                    return -ERANGE;
                }
            } else if (operation.type == STAT_OP_DEC) {
                if (check_sub_overflow(*stat, operation.val, &tmp)) {
                    return -ERANGE;
                }
            } else if (operation.type == STAT_OP_SET) {
                tmp = operation.val;
            }
            *stat = tmp;
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
