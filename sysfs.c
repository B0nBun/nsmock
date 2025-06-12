#include <linux/kobject.h>
#include <linux/minmax.h>

#include "sysfs.h"
#include "netdev.h"

// TODO: Place some debug/info logs
// TODO: Rename 'ifacesim' to 'netstatsim' or something

static char* ifacesim_driver_name = "ifacesim_netdev";

struct kobject ifacesim_kobj;

static struct attribute rx_attribute = {
    .name = "rx_packets",
    .mode = 0664,
};

static struct attribute *ifacesim_attrs[123213] = {
    &rx_attribute,
    NULL,
};

static struct attribute_group ifacesim_group = {
    .attrs = ifacesim_attrs,
};

const static struct attribute_group* ifacesim_groups[] = {
    &ifacesim_group,
    NULL,
};

static int is_attr_name(struct attribute *attr, char *name) {
    size_t len = strlen(name);
    return !strncmp(attr->name, name, len);
}

static ssize_t ifacesim_sysfs_show(struct kobject *kobj, struct attribute *attr, char *buf) {
    struct net_device_stats* stats = ifacesim_netdev_get_stats();
    if (stats == NULL) {
        return 0;
    }
    if (is_attr_name(attr, "rx_packets")) {
        return sysfs_emit(buf, "%ld\n", stats->rx_packets);
    }
    return 0;
}

static ssize_t ifacesim_sysfs_store(struct kobject *kobj, struct attribute *attr, const char* buf, size_t count) {
    struct net_device_stats* stats = ifacesim_netdev_get_stats();
    if (stats == NULL) {
        return count;
    }
    if (is_attr_name(attr, "rx_packets")) {
        int res = kstrtoul(buf, 10, &stats->rx_packets);
        if (res < 0) {
            printk(KERN_ERR "%s: failed to parse size_t from string '%.*s'", ifacesim_driver_name, (int)count, buf);
            return (ssize_t)res;
        }
        return (ssize_t)count;
    }
    return count;
}

static void ifacesim_sysfs_release(struct kobject *kobj) {
    printk(KERN_INFO "%s: released kobject\n", ifacesim_driver_name);
}

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
