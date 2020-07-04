/*
 *    This file has definion for PCI DMA transfer
 *
 *    Copyright (C) 2008 hisilico , c42025@hauwei.com
*
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/ioctl.h>

#ifndef __PCI_TRANS_H__
#define __PCI_TRANS_H__

#define HI_PCIT_MAX_BUS 2
#define HI_PCIT_MAX_SLOT 5

#define HI_PCIT_DMA_READ  0
#define HI_PCIT_DMA_WRITE 1

#define HI_PCIT_HOST_BUSID  0
#define HI_PCIT_HOST_SLOTID 0

#define HI_PCIT_NOSLOT  (-1)

#define HI_PCIT_INVAL_SUBSCRIBER_ID -1

#define HI_PCIT_DEV2BUS(dev) (dev >> 16)
#define HI_PCIT_DEV2SLOT(dev) (dev & 0xff)
#define HI_PCIT_MKDEV(bus, slot) (slot | (bus << 16))

struct pcit_dma_req
{
    unsigned int bus;   /* bus and slot will be ignored on device */
	unsigned int slot;
	unsigned long dest;
	unsigned long source;
	unsigned int len;
};

struct pcit_dev_cfg
{
	unsigned int slot;
	unsigned short vendor_id;
	unsigned short device_id;

	unsigned long np_phys_addr;
    unsigned long np_size;

	unsigned long pf_phys_addr;
    unsigned long pf_size;

	unsigned long cfg_phys_addr;
    unsigned long cfg_size;
};

struct pcit_bus_dev
{
	unsigned int bus_nr; /* input argument */
	struct pcit_dev_cfg devs[HI_PCIT_MAX_SLOT];
};

#define HI_PCIT_EVENT_DOORBELL_0 0
#define HI_PCIT_EVENT_DOORBELL_1 1
#define HI_PCIT_EVENT_DOORBELL_2 2
#define HI_PCIT_EVENT_DOORBELL_3 3
#define HI_PCIT_EVENT_DOORBELL_4 4
#define HI_PCIT_EVENT_DOORBELL_5 5
#define HI_PCIT_EVENT_DOORBELL_6 6
#define HI_PCIT_EVENT_DOORBELL_7 7
#define HI_PCIT_EVENT_DOORBELL_8 8
#define HI_PCIT_EVENT_DOORBELL_9 9
#define HI_PCIT_EVENT_DOORBELL_10 10
#define HI_PCIT_EVENT_DOORBELL_11 11
#define HI_PCIT_EVENT_DOORBELL_12 12
#define HI_PCIT_EVENT_DOORBELL_13 13
#define HI_PCIT_EVENT_DOORBELL_14 14
#define HI_PCIT_EVENT_DOORBELL_15 15 //reserved by mmc


#define HI_PCIT_EVENT_DMARD_0 16
#define HI_PCIT_EVENT_DMAWR_0 17

#define HI_PCIT_PCI_NP 1
#define HI_PCIT_PCI_PF 2
#define HI_PCIT_PCI_CFG 3
#define HI_PCIT_AHB_PF 4


struct pcit_event
{
	unsigned int event_mask;
    unsigned long long pts;
};

#define	HI_IOC_PCIT_BASE    'H'

/* Only used in host, you can get information of all devices on each bus.*/
#define	HI_IOC_PCIT_INQUIRE _IOR(HI_IOC_PCIT_BASE, 1, struct pcit_bus_dev)

/* Only used in device, these tow command will block until DMA compeleted. */
#define	HI_IOC_PCIT_DMARD  _IOW(HI_IOC_PCIT_BASE, 2, struct pcit_dma_req)
#define	HI_IOC_PCIT_DMAWR  _IOW(HI_IOC_PCIT_BASE, 3, struct pcit_dma_req)

/* Only used in host, you can bind a fd returned by open() to a device,
 * then all operation by this fd is orient to this device.
 */
#define	HI_IOC_PCIT_BINDDEV  _IOW(HI_IOC_PCIT_BASE, 4, int)

/* Used in host and device.
 * on host, you should specify which device doorbell will be send to
 * by the parameter, but in device, the parameters will be ingored,and
 * a doorbell will be send to host.
 */
#define	HI_IOC_PCIT_DOORBELL  _IOW(HI_IOC_PCIT_BASE, 5, int)

/* Used in host and device.
 * you can subscribe more than one event using this command.
 * on host, fd passed to ioctl() indicates target device. so you should
 * use "HI_IOC_PCIT_BINDDEV" bind a fd first, otherwise an error will
 * be met. but on device, all events are triggered by host, so it NOT
 * needed to bind a fd.
 */
#define	HI_IOC_PCIT_SUBSCRIBE  _IOW(HI_IOC_PCIT_BASE, 6, int)

/* If nscribled all events, diriver will return NONE event to all listeners. */
#define	HI_IOC_PCIT_UNSUBSCRIBE  _IOW(HI_IOC_PCIT_BASE, 7, int)

/* On host, this command will listen the device specified by parameter.*/
#define	HI_IOC_PCIT_LISTEN  _IOR(HI_IOC_PCIT_BASE, 8, struct pcit_event)


#ifdef __KERNEL__
#include <linux/list.h>

//#define HI_PCIT_IS_HOST hi3511_bridge_ahb_readl((CPU_VERSION))&0x00002000

struct pcit_dma_task
{
	struct list_head list;   /* internal data, don't touch */
    unsigned int state;      /* internal data, don't touch.
                                0: todo, 1: doing, 2: finished */

	unsigned long dir; /* read or write */
	unsigned long src;
	unsigned long dest;
	unsigned int len;
	void *private_data;
	void (*finish)(struct pcit_dma_task *task);
};

/* only used in device */
int pcit_create_task(struct pcit_dma_task *task);

/* only used in host*/
struct pcit_subscriber
{
    char name[16];
    void (*notify)(unsigned int bus, unsigned int slot, struct pcit_event *, void *);
    void *data;
};

typedef int pcit_subscriber_id_t;
pcit_subscriber_id_t pcit_subscriber_register(struct pcit_subscriber *user);
int pcit_subscriber_deregister(pcit_subscriber_id_t id);
int pcit_subscribe_event(pcit_subscriber_id_t id, unsigned int bus,
    unsigned int slot, struct pcit_event *pevent);
int pcit_unsubscribe_event(pcit_subscriber_id_t id, unsigned int bus,
    unsigned int slot, struct pcit_event *pevent);
void ss_doorbell_triggle(unsigned int addr, unsigned int value);
//unsigned long pcit_get_window_base(int slot, int flag);
unsigned long get_pf_window_base(int slot);

#endif

#endif

