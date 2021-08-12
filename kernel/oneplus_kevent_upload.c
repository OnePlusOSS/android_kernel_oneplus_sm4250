// SPDX-License-Identifier: GPL-2.0-only
/* oneplus_kevent_upload.c - for kevent action
 * upload,root action upload to user layer
 *  author by wangzhenhua,Plf.Framework
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <net/net_namespace.h>
#include <linux/proc_fs.h>
#include <net/sock.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/uaccess.h>

#include <linux/oneplus_kevent.h>

#ifdef CONFIG_ONEPLUS_KEVENT_UPLOAD

static struct sock *netlink_fd;
static u32 kevent_pid;

/* send to user space */
int kevent_send_to_user(struct kernel_packet_info *userinfo)
{
	int ret, size, size_use;
	unsigned int o_tail;
	struct sk_buff *skbuff;
	struct nlmsghdr *nlh;
	struct kernel_packet_info *packet;

	/* protect payload too long problem*/
	if (userinfo->payload_length >= 2048) {
		pr_info("s%: payload_length out of range\n", __func__);
		return -EFAULT;
	}

	size = NLMSG_SPACE(sizeof(struct kernel_packet_info) +
			userinfo->payload_length);

	/*allocate new buffer cache */
	skbuff = alloc_skb(size, GFP_ATOMIC);
	if (skbuff == NULL) {
		//pr_err("s%: skbuff alloc_skb failed!\n", __func__);
		return -ENOMEM;
	}

	/* fill in the data structure */
	nlh = nlmsg_put(skbuff, 0, 0, 0, size - sizeof(*nlh), 0);
	if (nlh == NULL) {
		//pr_err("s%: nlmsg_put failaure!\n", __func__);
		nlmsg_free(skbuff);
		return -ENOMEM;
	}

	o_tail = skbuff->tail;

	size_use = sizeof(struct kernel_packet_info) + userinfo->payload_length;

	/* use struct kernel_packet_info for data of nlmsg */
	packet = NLMSG_DATA(nlh);
	memset(packet, 0, size_use);

	/* copy the payload content */
	memcpy(packet, userinfo, size_use);

	//compute nlmsg length
	nlh->nlmsg_len = skbuff->tail - o_tail;

	/* set control field,sender's pid */
#if (KERNEL_VERSION(3, 7, 0) > LINUX_VERSION_CODE)
	NETLINK_CB(skbuff).pid = 0;
#else
	NETLINK_CB(skbuff).portid = 0;
#endif

	NETLINK_CB(skbuff).dst_group = 0;

	/* send data */
	ret = netlink_unicast(netlink_fd, skbuff, kevent_pid, MSG_DONTWAIT);
	if (ret < 0) {
		return -EFAULT;
	}

	return 0;
}
EXPORT_SYMBOL(kevent_send_to_user);

/* kernel receive message from user space */
void kernel_kevent_receive(struct sk_buff *__skbbr)
{
	struct sk_buff *skbu;
	struct nlmsghdr *nlh = NULL;

	skbu = skb_get(__skbbr);

	if (skbu->len >= sizeof(struct nlmsghdr)) {
		nlh = (struct nlmsghdr *)skbu->data;

		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr))
				&& (__skbbr->len >= nlh->nlmsg_len))
			kevent_pid = nlh->nlmsg_pid;
	}

	kfree_skb(skbu);
}

int __init netlink_kevent_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.groups = 0,
		.input  = kernel_kevent_receive,
	};

	netlink_fd = netlink_kernel_create(&init_net,
		NETLINK_ONEPLUS_KEVENT, &cfg);
	if (!netlink_fd) {
		pr_info("s%: Can not create a netlink socket\n", __func__);
		return -EFAULT;
	}

	return 0;
}

void __exit netlink_kevent_exit(void)
{
	sock_release(netlink_fd->sk_socket);
}

module_init(netlink_kevent_init);
module_exit(netlink_kevent_exit);
MODULE_LICENSE("GPL");

#endif /* CONFIG_ONEPLUS_KEVENT_UPLOAD */

