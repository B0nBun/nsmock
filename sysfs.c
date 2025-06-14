#include <linux/kobject.h>
#include <linux/minmax.h>

#include "sysfs.h"
#include "netdev.h"

#undef  pr_fmt // Supressing GCC warning
#define pr_fmt(fmt) "nsmock: " fmt

struct kobject nsmock_kobj;

struct nsmock_stat {
    char* name;
    size_t stat_offset;
    struct attribute attr;
};

#define STAT(member) { .name = #member, .stat_offset = offsetof(struct net_device_stats, member) }

static struct nsmock_stat nsmock_stats[] = {
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

#define nsmock_STATS_LEN (sizeof(nsmock_stats) / sizeof(nsmock_stats[0]))

struct nsmock_stat_op {
    enum {
        STAT_OP_INC,
        STAT_OP_DEC,
        STAT_OP_SET,
    } type;
    unsigned long val;
};

// Filled out during module init
static struct attribute *nsmock_attrs[nsmock_STATS_LEN + 1] = {};

static struct attribute_group nsmock_group = {
    .attrs = nsmock_attrs,
};

const static struct attribute_group* nsmock_groups[] = {
    &nsmock_group,
    NULL,
};

static ssize_t nsmock_sysfs_show(struct kobject *kobj, struct attribute *attr, char *buf);
static ssize_t nsmock_sysfs_store(struct kobject *kobj, struct attribute *attr, const char* buf, size_t count);

static struct sysfs_ops nsmock_sysfs_ops = {
    .show = nsmock_sysfs_show,
    .store = nsmock_sysfs_store,
};

static void nsmock_sysfs_release(struct kobject *kobj);

static const struct kobj_type nsmock_ktype = {
    .sysfs_ops = &nsmock_sysfs_ops,
    .release = nsmock_sysfs_release,
    .default_groups = nsmock_groups,
};

int nsmock_sysfs_init(void) {
    pr_debug("sysfs init()\n");
    for (size_t i = 0; i < nsmock_STATS_LEN; i ++) {
        struct attribute* attr = &nsmock_stats[i].attr;
        attr->name = nsmock_stats[i].name;
        attr->mode = 0664;
        nsmock_attrs[i] = attr;
    }
    nsmock_attrs[nsmock_STATS_LEN] = NULL;

    int ret = kobject_init_and_add(&nsmock_kobj, &nsmock_ktype, kernel_kobj, "%s", "nsmock");
    if (!ret) {
        pr_err("failed to init and add kobject\n");
        return ret;
    }
    return 0;
}

void nsmock_sysfs_exit(void) {
    kobject_put(&nsmock_kobj);
    pr_info("exit\n");
}

static void nsmock_sysfs_release(struct kobject *kobj) {
    pr_info("released kobject\n");
}

static ssize_t nsmock_sysfs_show(struct kobject *kobj, struct attribute *attr, char *buf) {
    if (attr == NULL) {
        return 0;
    }
    pr_debug("sysfs show('%s')\n", attr->name);
    struct net_device_stats* stats = nsmock_netdev_get_stats();
    if (stats == NULL) {
        pr_notice("tried to find net device stats, but received NULL\n");
        return 0;
    }
    for (size_t i = 0; i < nsmock_STATS_LEN; i ++) {
        if (sysfs_streq(attr->name, nsmock_stats[i].name)) {
            size_t offset = nsmock_stats[i].stat_offset;
            pr_debug("found attr with name '%s' and offset %ld, emitting\n", attr->name, offset);
            unsigned long* stat = (void*)((uintptr_t)stats + offset);
            return sysfs_emit(buf, "%lu\n", *stat);
        }
    }
    return 0;
}

static int nsmock_parse_stat_op(const char* s, struct nsmock_stat_op* op);

static ssize_t nsmock_sysfs_store(struct kobject *kobj, struct attribute *attr, const char* buf, size_t count) {
    if (attr == NULL) {
        return 0;
    }
    pr_debug("sysfs store('%s', '%*pEps')\n", attr->name, (int)count, buf);
    struct net_device_stats* stats = nsmock_netdev_get_stats();
    if (stats == NULL) {
        pr_notice("tried to find net device stats, but received NULL\n");
        return count;
    }
    for (size_t i = 0; i < nsmock_STATS_LEN; i ++) {
        if (sysfs_streq(attr->name, nsmock_stats[i].name)) {
            size_t offset = nsmock_stats[i].stat_offset;
            struct nsmock_stat_op operation;
            int res = nsmock_parse_stat_op(buf, &operation);
            if (res < 0) {
                pr_err("failed to parse size_t from string '%.*s'", (int)count, buf);
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
            pr_debug("found attr with name '%s' and offset %ld, storing %lu\n", attr->name, offset, tmp);
            *stat = tmp;
            return count;
        }
    }
    return count;
}

static int nsmock_parse_stat_op(const char* s, struct nsmock_stat_op* op) {
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
