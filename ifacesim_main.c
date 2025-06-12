#include <linux/module.h>
#include <linux/init.h>

#include "netdev.h"
#include "sysfs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ratmir Gusiakov");
MODULE_DESCRIPTION("A Linux kernel module for simulating error/dropped packets and stuff");

static int __init ifacesim_init(void) {
    int ret = ifacesim_netdev_init();
    if (ret != 0) {
        return ret;
    }
    ret = ifacesim_sysfs_init();
    if (ret != 0) {
        return ret;
    }
    return 0;
}

static void __exit ifacesim_exit(void) {
    ifacesim_netdev_exit();
    ifacesim_sysfs_exit();
}

module_init(ifacesim_init);
module_exit(ifacesim_exit);
