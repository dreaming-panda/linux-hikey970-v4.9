/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Author: TH <tsunghan_tsai@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef TCPM_H_
#define TCPM_H_

#include <linux/kernel.h>
#include <linux/notifier.h>

#include <linux/hisi/usb/pd/richtek/tcpci_config.h>

#include <linux/hisi/usb/hisi_pd_dev.h>
#include <linux/hisi/log/hisi_log.h>

#ifndef HISILOG_TAG
#define HISILOG_TAG hisi_pd
HISILOG_REGIST();
#endif

struct tcpc_device;

/*
 * Type-C Port Notify Chain
 */

enum typec_attach_type {
	TYPEC_UNATTACHED = 0,
	TYPEC_ATTACHED_SNK,
	TYPEC_ATTACHED_SRC,
	TYPEC_ATTACHED_AUDIO,
	TYPEC_ATTACHED_DEBUG,

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	TYPEC_ATTACHED_DBGACC_SNK,		/* Rp, Rp */
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	TYPEC_ATTACHED_CUSTOM_SRC,		/* Same Rp */
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */
};

enum dpm_request_state {
	DPM_REQ_NULL,
	DPM_REQ_QUEUE,
	DPM_REQ_RUNNING,
	DPM_REQ_SUCCESS,
	DPM_REQ_FAILED,

	/* Request failed */

	DPM_REQ_ERR_IDLE = DPM_REQ_FAILED,

	DPM_REQ_ERR_NOT_READY,
	DPM_REQ_ERR_WRONG_ROLE,

	DPM_REQ_ERR_RECV_HRESET,
	DPM_REQ_ERR_RECV_SRESET,
	DPM_REQ_ERR_SEND_HRESET,
	DPM_REQ_ERR_SEND_SRESET,
	DPM_REQ_ERR_SEND_BIST,

	/* Internal */
	DPM_REQ_SUCCESS_CODE,

	DPM_REQ_E_UVDM_ACK,
	DPM_REQ_E_UVDM_NAK,
};

/* Power role */
#define PD_ROLE_SINK   0
#define PD_ROLE_SOURCE 1

/* Data role */
#define PD_ROLE_UFP    0
#define PD_ROLE_DFP    1

/* Vconn role */
#define PD_ROLE_VCONN_OFF 0
#define PD_ROLE_VCONN_ON  1

enum {
	TCP_NOTIFY_DIS_VBUS_CTRL,
	TCP_NOTIFY_SOURCE_VCONN,
	TCP_NOTIFY_SOURCE_VBUS,
	TCP_NOTIFY_SINK_VBUS,
	TCP_NOTIFY_PR_SWAP,
	TCP_NOTIFY_DR_SWAP,
	TCP_NOTIFY_VCONN_SWAP,
	TCP_NOTIFY_ENTER_MODE,
	TCP_NOTIFY_EXIT_MODE,
	TCP_NOTIFY_AMA_DP_STATE,
	TCP_NOTIFY_AMA_DP_ATTENTION,
	TCP_NOTIFY_AMA_DP_HPD_STATE,

	TCP_NOTIFY_TYPEC_STATE,
	TCP_NOTIFY_PD_STATE,
};

struct tcp_ny_pd_state {
	u8 connected;
};

struct tcp_ny_swap_state {
	u8 new_role;
};

struct tcp_ny_enable_state {
	bool en;
};

struct tcp_ny_typec_state {
	u8 rp_level;
	u8 polarity;
	u8 old_state;
	u8 new_state;
};

enum {
	TCP_VBUS_CTRL_REMOVE = 0,
	TCP_VBUS_CTRL_TYPEC = 1,
	TCP_VBUS_CTRL_PD = 2,

	TCP_VBUS_CTRL_HRESET = TCP_VBUS_CTRL_PD,
	TCP_VBUS_CTRL_PR_SWAP = 3,
	TCP_VBUS_CTRL_REQUEST = 4,

	TCP_VBUS_CTRL_PD_DETECT = (1 << 7),

	TCP_VBUS_CTRL_PD_HRESET =
		TCP_VBUS_CTRL_HRESET | TCP_VBUS_CTRL_PD_DETECT,

	TCP_VBUS_CTRL_PD_PR_SWAP =
		TCP_VBUS_CTRL_PR_SWAP | TCP_VBUS_CTRL_PD_DETECT,

	TCP_VBUS_CTRL_PD_REQUEST =
		TCP_VBUS_CTRL_REQUEST | TCP_VBUS_CTRL_PD_DETECT,
};

struct tcp_ny_vbus_state {
	int mv;
	int ma;
	u8 type;
};

struct tcp_ny_mode_ctrl {
	u16 svid;
	u8 ops;
	u32 mode;
};

enum {
	SW_USB = 0,
	SW_DFP_D,
	SW_UFP_D,
};

struct tcp_ny_ama_dp_state {
	u8 sel_config;
	u8 signal;
	u8 pin_assignment;
	u8 polarity;
	u8 active;
};

enum {
	TCP_DP_UFP_U_MASK = 0x7C,
	TCP_DP_UFP_U_POWER_LOW = 1 << 2,
	TCP_DP_UFP_U_ENABLED = 1 << 3,
	TCP_DP_UFP_U_MF_PREFER = 1 << 4,
	TCP_DP_UFP_U_USB_CONFIG = 1 << 5,
	TCP_DP_UFP_U_EXIT_MODE = 1 << 6,
};

struct tcp_ny_ama_dp_attention {
	u8 state;
};

struct tcp_ny_ama_dp_hpd_state {
	bool irq : 1;
	bool state : 1;
};

struct tcp_ny_uvdm {
	bool ack;
	u8 uvdm_cnt;
	u16 uvdm_svid;
	u32 *uvdm_data;
};

struct tcp_notify {
	union {
		struct tcp_ny_enable_state en_state;
		struct tcp_ny_vbus_state vbus_state;
		struct tcp_ny_typec_state typec_state;
		struct tcp_ny_swap_state swap_state;
		struct tcp_ny_pd_state pd_state;
		struct tcp_ny_mode_ctrl mode_ctrl;
		struct tcp_ny_ama_dp_state ama_dp_state;
		struct tcp_ny_ama_dp_attention ama_dp_attention;
		struct tcp_ny_ama_dp_hpd_state ama_dp_hpd_state;
		struct tcp_ny_uvdm uvdm_msg;
	};
};

struct tcpc_device *tcpc_dev_get_by_name(const char *name);

int register_tcp_dev_notifier(
		struct tcpc_device *tcp_dev,
		struct notifier_block *nb);
int unregister_tcp_dev_notifier(
		struct tcpc_device *tcp_dev,
		struct notifier_block *nb);

struct tcpc_device *notify_tcp_dev_ready(const char *name);

/*
 * Type-C Port Control I/F
 */

enum tcpm_error_list {
	TCPM_SUCCESS = 0,
	TCPM_ERROR_UNKNOWN = -1,
	TCPM_ERROR_UNATTACHED = -2,
	TCPM_ERROR_PARAMETER = -3,
	TCPM_ERROR_PUT_EVENT = -4,
};

#define TCPM_PDO_MAX_SIZE	7

struct tcpm_power_cap {
	u8 cnt;
	u32 pdos[TCPM_PDO_MAX_SIZE];
};

/* Inquire TCPM status */

enum tcpc_cc_voltage_status {
	TYPEC_CC_VOLT_OPEN = 0,
	TYPEC_CC_VOLT_RA = 1,
	TYPEC_CC_VOLT_RD = 2,

	TYPEC_CC_VOLT_SNK_DFT = 5,
	TYPEC_CC_VOLT_SNK_1_5 = 6,
	TYPEC_CC_VOLT_SNK_3_0 = 7,

	TYPEC_CC_DRP_TOGGLING = 15,
};

enum tcpm_vbus_level {
#ifdef CONFIG_TCPC_VSAFE0V_DETECT
	TCPC_VBUS_SAFE0V = 0,
	TCPC_VBUS_INVALID,
	TCPC_VBUS_VALID,
#else
	TCPC_VBUS_INVALID = 0,
	TCPC_VBUS_VALID,
#endif
};

enum typec_role_defination {
	TYPEC_ROLE_UNKNOWN = 0,
	TYPEC_ROLE_SNK,
	TYPEC_ROLE_SRC,
	TYPEC_ROLE_DRP,
	TYPEC_ROLE_TRY_SRC,
	TYPEC_ROLE_TRY_SNK,
	TYPEC_ROLE_NR,
};

int tcpm_inquire_remote_cc(
		struct tcpc_device *tcpc_dev,
		u8 *cc1, u8 *cc2, bool from_ic);
int tcpm_inquire_vbus_level(struct tcpc_device *tcpc_dev, bool from_ic);
bool tcpm_inquire_cc_polarity(struct tcpc_device *tcpc_dev);
u8 tcpm_inquire_typec_attach_state(struct tcpc_device *tcpc_dev);
u8 tcpm_inquire_typec_role(struct tcpc_device *tcpc_dev);
u8 tcpm_inquire_typec_local_rp(struct tcpc_device *tcpc_dev);

int tcpm_typec_set_rp_level(struct tcpc_device *tcpc_dev, u8 level);

int tcpm_typec_change_role(struct tcpc_device *tcpc_dev, u8 typec_role);

#ifdef CONFIG_USB_POWER_DELIVERY

bool tcpm_inquire_pd_connected(struct tcpc_device *tcpc_dev);

bool tcpm_inquire_pd_prev_connected(struct tcpc_device *tcpc_dev);

u8 tcpm_inquire_pd_data_role(struct tcpc_device *tcpc_dev);

u8 tcpm_inquire_pd_power_role(struct tcpc_device *tcpc_dev);

u8 tcpm_inquire_pd_vconn_role(struct tcpc_device *tcpc_dev);

#endif	/* CONFIG_USB_POWER_DELIVERY */

/* Request TCPM to send PD Request */

int tcpm_power_role_swap(struct tcpc_device *tcpc_dev);
int tcpm_data_role_swap(struct tcpc_device *tcpc_dev);
int tcpm_vconn_swap(struct tcpc_device *tcpc_dev);
int tcpm_goto_min(struct tcpc_device *tcpc_dev);
int tcpm_soft_reset(struct tcpc_device *tcpc_dev);
int tcpm_hard_reset(struct tcpc_device *tcpc_dev);
int tcpm_get_source_cap(
		struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap);
int tcpm_get_sink_cap(struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap);
int tcpm_bist_cm2(struct tcpc_device *tcpc_dev);
int tcpm_request(struct tcpc_device *tcpc_dev, int mv, int ma);
int tcpm_error_recovery(struct tcpc_device *tcpc_dev);

/* Request TCPM to send VDM */

int tcpm_discover_cable(struct tcpc_device *tcpc_dev, u32 *vdos);

int tcpm_vdm_request_id(struct tcpc_device *tcpc_dev, u8 *cnt, u8 *payload);

/* Notify TCPM */

int tcpm_notify_vbus_stable(struct tcpc_device *tcpc_dev);
#endif /* TCPM_H_ */
