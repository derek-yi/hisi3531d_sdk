/*
 * Sys Config Driver for HiSilicon BVT SoCs
 *
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <mach/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/pinctrl/consumer.h>

#define SYS_CTRL_NAME "sysctrl-ddr,pins"
char Pad_Ctrl_Name[50] = "padctrl-ability,";

struct reg_vals {     
	unsigned int reg_phy_addr;
	unsigned int value;
};    
struct drv_data {
	struct pinctrl *p;
	struct pinctrl_state *state;
};
static const struct of_device_id hibvt_pinctrl_of_match[] = {
	{ .compatible = "hisilicon,sys_config_ctrl" },
	//{ .compatible = "hisilicon,user_define_pinmux" },
	{  },
};

char * mode = "demo";
module_param(mode, charp, S_IRUGO);

static int hibvt_pinmux_init(struct platform_device *pdev,const char* p_board_name)
{   
    int ret;
    struct pinctrl* p;
    struct pinctrl_state* state;
    struct drv_data* drv_priv_data;

    
    drv_priv_data = devm_kzalloc(&pdev->dev, sizeof(*drv_priv_data), GFP_KERNEL);

    if (!drv_priv_data)
    {
        dev_err(&pdev->dev, "could not allocate vou driver private data.\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, drv_priv_data);

    p = (struct pinctrl*)devm_pinctrl_get(&pdev->dev);
    if (IS_ERR(p))
    {
        dev_err(&pdev->dev, "no pinctrl handle\n");
        return -ENOENT;
    }

    state = (struct pinctrl_state*)pinctrl_lookup_state(p, p_board_name);
    if (IS_ERR(state))
    {
        dev_err(&pdev->dev, "no %s pinctrl state\n", p_board_name);
        devm_pinctrl_put(p);
        return -ENOENT;
    }

    ret = pinctrl_select_state(p, state);
    if (ret)
    {
        dev_err(&pdev->dev, "failed to activate %s pinctrl state\n", p_board_name);
        devm_pinctrl_put(p);
        return -ENOENT;
    }

    drv_priv_data->p = p;
    drv_priv_data->state = state;

    return 0;
}

static int hibvt_pin_ctrl(struct platform_device *pdev,const char* p_node_name)
{	
	const __be32 *mux;
	unsigned int size, rows, index = 0, found = 0;
	void __iomem *reg_vir_addr;	
    struct reg_vals *vals;
    struct device_node *np = pdev->dev.of_node;
	mux = of_get_property(np, p_node_name, &size);  
	if ((!mux) || (size < sizeof(*mux) * 2)) {
		return -EINVAL;
	}

	size /= sizeof(*mux);	/* Number of elements in array */
	rows = size / 2;

	vals = (struct reg_vals *)kmalloc(sizeof(*vals) * rows, GFP_KERNEL);
	if (!vals)
		return -ENOMEM;
	while (index < size) {
        
		vals[found].reg_phy_addr = be32_to_cpup(mux + index++);
		vals[found].value = be32_to_cpup(mux + index++);

		reg_vir_addr = ioremap_nocache(vals[found].reg_phy_addr, sizeof(*mux));
        if(reg_vir_addr == NULL)
        {
            printk("ioremap_nocache error\n");
            continue;
        }

		*((volatile int *)(reg_vir_addr)) = vals[found].value;

		iounmap(reg_vir_addr);

		found++;
	}

	kfree(vals);

	return 0;
}

static int hibvt_sys_init(struct platform_device *pdev)
{
    int ret;
    char* temp_mode;

    if (!strncmp(mode, "demo", 10)
        || !strncmp(mode, "demopro", 10)
        || !strncmp(mode, "slave1", 10)
        || !strncmp(mode, "slave2", 10)
        || !strncmp(mode, "sck", 10))
    {
        if (!strncmp(mode, "demopro", 10))
        {
            temp_mode = "demo";
        }
        else
        {
            temp_mode = mode;
        }
        strncat(Pad_Ctrl_Name, temp_mode, 10);
        //printk("Pad_Ctrl_Name %s \n", Pad_Ctrl_Name);

        if (hibvt_pin_ctrl(pdev, SYS_CTRL_NAME) != 0
            || hibvt_pin_ctrl(pdev, Pad_Ctrl_Name) != 0)
        {
            dev_err(&pdev->dev, "hibvt_sys_init failure\n");
            return -ENOMEM;
        }

        ret = hibvt_pinmux_init(pdev, mode);

        if (ret)
        {
            dev_err(&pdev->dev, "hibvt_pinmux_init failure\n");
            return -ENOENT;
        }

        printk("load sys_config.ko for ...OK!\n");
        return 0;

    }
    else
    {
        printk("mode is invalid %s...!\n",mode);
        return -ENOENT;
    }

    return ret;
}



#ifdef CONFIG_PM
static int hibvt_pinctrl_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int hibvt_pinctrl_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

MODULE_DEVICE_TABLE(of, hibvt_pinctrl_of_match);

static int hibvt_pinctrl_probe(struct platform_device *pdev)
{
    int ret;
    ret = hibvt_sys_init(pdev);
    return ret;
}

static int hibvt_pinctrl_remove(struct platform_device *pdev)
{
    printk("unload sys_config.ko for ...OK!\n");
	return 0;
}

static struct platform_driver hibvt_pinctrl_driver = {
	.driver = {
		.name = "hibvt-pinctrl",
		.of_match_table = hibvt_pinctrl_of_match,
	},
	.probe = hibvt_pinctrl_probe,
	.remove	= hibvt_pinctrl_remove,
#ifdef CONFIG_PM
	.suspend        = hibvt_pinctrl_suspend,
	.resume         = hibvt_pinctrl_resume,
#endif
};
module_platform_driver(hibvt_pinctrl_driver);
MODULE_AUTHOR("HiMPP GRP");
MODULE_DESCRIPTION("HiSilicon BVT Sys Config Driver");
MODULE_LICENSE("GPL");
