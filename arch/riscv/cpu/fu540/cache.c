// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 SiFive, Inc
 *
 * Authors:
 *   Pragnesh Patel <pragnesh.patel@sifive.com>
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/bitops.h>

/* Register offsets */
#define L2_CACHE_CONFIG	0x000
#define L2_CACHE_ENABLE	0x008

#define MASK_NUM_WAYS	GENMASK(15, 8)
#define NUM_WAYS_SHIFT	8

DECLARE_GLOBAL_DATA_PTR;

static fdt_addr_t l2cc_get_base_addr(void)
{
	const void *blob = gd->fdt_blob;
	int node = fdt_node_offset_by_compatible(blob, -1,
					     "sifive,fu540-c000-ccache");

	if (node < 0)
		return FDT_ADDR_T_NONE;

	return fdtdec_get_addr_size_auto_parent(blob, 0, node, "reg", 0,
						NULL, false);
}

int cache_enable_ways(void)
{
	fdt_addr_t base;
	u32 config;
	u32 ways;

	volatile u32 *enable;

	base = l2cc_get_base_addr();
	if (base == FDT_ADDR_T_NONE)
		return FDT_ADDR_T_NONE;

	config = readl((volatile u32 *)base + L2_CACHE_CONFIG);
	ways = (config & MASK_NUM_WAYS) >> NUM_WAYS_SHIFT;

	enable = (volatile u32 *)(base + L2_CACHE_ENABLE);

	/* memory barrier */
	mb();
#if CONFIG_IS_ENABLED(SIFIVE_FU540_L2CC_WAYENABLE_DIY)
	if (CONFIG_SIFIVE_FU540_L2CC_WAYENABLE_DIY_NUM >= ways)
		(*enable) = ways;
	(*enable) = CONFIG_SIFIVE_FU540_L2CC_WAYENABLE_DIY_NUM;
#else
	(*enable) = ways - 1;
#endif //SIFIVE_FU540_L2CC_WAYENABLE_DIY
	/* memory barrier */
	mb();
	return 0;
}

u32 cache_enable_ways_debug(u32 ways_input)
{
	fdt_addr_t base = l2cc_get_base_addr();
	volatile u32 *enable = (volatile u32 *)(base + L2_CACHE_ENABLE);

	if (base == FDT_ADDR_T_NONE)
		return 0xffffffff;

	printf("cache config(0x%p): 0x%x --> ",	enable, readl(enable));
	mb();
	(*enable) = ways_input;
	mb();
	printf("0x%x \n", readl(enable));
	mb();

	return readl(enable);
}

#if CONFIG_IS_ENABLED(SIFIVE_FU540_L2CC_FLUSH)
#define L2_CACHE_FLUSH64	0x200

void flush_dcache_range(unsigned long start, unsigned long end)
{
	fdt_addr_t base;
	unsigned long line;
	volatile unsigned long *flush64;

	/* make sure the address is in the range */
	if(start > end ||
	   start < CONFIG_SIFIVE_FU540_L2CC_FLUSH_START ||
	   end > (CONFIG_SIFIVE_FU540_L2CC_FLUSH_START +
		  CONFIG_SIFIVE_FU540_L2CC_FLUSH_SIZE))
		return;

	base = l2cc_get_base_addr();
	if (base == FDT_ADDR_T_NONE)
		return;

	flush64 = (volatile unsigned long *)(base + L2_CACHE_FLUSH64);

	/* memory barrier */
	mb();
	for (line = start; line < end; line += CONFIG_SYS_CACHELINE_SIZE)
		(*flush64) = line;
	/* memory barrier */
	mb();

	return;
}
#endif //SIFIVE_FU540_L2CC_FLUSH
