#include <linux/module.h>
#include <linux/init.h>

#include "netdev.h"
#include "sysfs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ratmir Gusiakov");
MODULE_DESCRIPTION("A Linux kernel module for mocking net_device_stats");

static int __init nsmock_init(void) {
    int ret = nsmock_netdev_init();
    if (ret != 0) {
        return ret;
    }
    ret = nsmock_sysfs_init();
    if (ret != 0) {
        return ret;
    }
    return 0;
}

static void __exit nsmock_exit(void) {
    nsmock_netdev_exit();
    nsmock_sysfs_exit();
}

module_init(nsmock_init);
module_exit(nsmock_exit);
