/*
 * Copyright (C) 2017 Group XX
 * Initial release: Dengcai Xu <dxu@cnexlabs.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * fscftl initialization.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#include "../nvme/host/nvme.h"
#include "hwcfg/cfg/flash_cfg.h"
#include "hwcfg/regrw.h"
#include "build_recovery/power.h"
#include "fscftl.h"
#include "datapath/ppa-ops.h"

static bool mcp = false;
module_param(mcp, bool, 0644);

static char exdev_name[DISK_NAME_LEN] = "nvme0n1";
module_param_string(bdev, exdev_name, 8, 0);   //basedev name

extern struct nvme_ppa_ops exdev_ppa_ops;

int fscftl_setup(void)
{
    return 0;
}

void fscftl_cleanup(void)
{
    return;
}

static int __init fscftl_module_init(void)
{
    int ret = 0;
    struct nvm_exdev *exdev;

	printk("NandFlash type:%s\n", FLASH_TYPE);

	exdev = nvm_find_exdev(exdev_name);
    if (exdev == NULL)
        return -ENODEV;

	printk("find exdev:%s  magic_dw:0x%x\n", exdev->bdiskname, exdev->magic_dw);

    exdev->ops = &exdev_ppa_ops;

    ctrl_reg_setup(exdev->ctrl);

	ret = nvm_exdev_setup_pool(exdev, "prp-ppa-list");
	if (ret) 
		return ret;

    ret = fscftl_setup();
    if (ret)
        goto release_pool;

    if (mcp) {
        if (do_manufactory_init(exdev))
            goto err_cleanup;
    } else {
        if (try_recovery_systbl())
            goto err_cleanup;
    }

	ret = nvm_create_exns(exdev);
    if (ret)
        goto err_cleanup;

	/* testcase */
	run_testcase(exdev);

	return 0;

err_cleanup:
    fscftl_cleanup();
release_pool:
	nvm_exdev_release_pool(exdev);
    return -EFAULT;
}

static void __exit fscftl_module_exit(void)
{
	struct nvm_exdev *exdev = nvm_find_exdev(exdev_name);

	nvm_delete_exns(exdev);
    flush_down_systbl();
    fscftl_cleanup();
	nvm_exdev_release_pool(exdev);
    
	return;
}

module_init(fscftl_module_init);
module_exit(fscftl_module_exit);
MODULE_AUTHOR("Dengcai Xu <dxu@cnexlabs.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Host-Based Full Stack Control FTL for NVMe SSDs");

