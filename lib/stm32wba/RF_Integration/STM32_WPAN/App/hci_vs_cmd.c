/*
 * Copyright 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hci_vs_cmd);

#include "common/bt_str.h"

#include "host/conn_internal.h"
#include "host/classic/sco_internal.h"
#include "ble_types.h"
#include "ble_defs.h"

#define ACI_HAL_WRITE_CONFIG_DATA			BT_OP(BT_OGF_VS, 0xFC0C)
#define ACI_HAL_READ_CONFIG_DATA			BT_OP(BT_OGF_VS, 0xFC0D)
#define ACI_HAL_SET_TX_POWER_LEVEL			BT_OP(BT_OGF_VS, 0xFC0F)
#define ACI_HAL_LE_TX_TEST_PACKET_NUMBER		BT_OP(BT_OGF_VS, 0xFC14)
#define ACI_HAL_TONE_START				BT_OP(BT_OGF_VS, 0xFC15)
#define ACI_HAL_TONE_STOP				BT_OP(BT_OGF_VS, 0xFC16)
#define ACI_HAL_SET_RADIO_ACTIVITY_MASK			BT_OP(BT_OGF_VS, 0xFC18)
#define ACI_HAL_SET_EVENT_MASK				BT_OP(BT_OGF_VS, 0xFC1A)
#define ACI_HAL_GET_LINK_STATUS_V2			BT_OP(BT_OGF_VS, 0xFC1B)
#define ACI_HAL_SET_SYNC_EVENT_CONFIG			BT_OP(BT_OGF_VS, 0xFC21)
#define ACI_HAL_READ_RSSI				BT_OP(BT_OGF_VS, 0xFC22)
#define ACI_HAL_CONTINUOUS_TX_START			BT_OP(BT_OGF_VS, 0xFC2E)
#define ACI_HAL_EAD_ENCRYPT_DECRYPT			BT_OP(BT_OGF_VS, 0xFC2F)

#define ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT 		0x1804
#define ACI_HAL_SYNC_EVENT				0x1808


typedef struct _aci_hal_cmd {
	uint16_t opcode;
	uint16_t data_len;
	uint8_t *data;
}aci_hal_cmd;

struct hci_vs_evt {
	uint16_t  evtcode;
	uint8_t  data[0];
} __packed;


static int stm32wbax_send_vs_cmd(const aci_hal_cmd *cmd, struct net_buf **rsp)
{
	int err;
	struct net_buf *buf;

	if ((cmd == NULL) || (rsp == NULL)) {
		return -EINVAL;
	}

	if ((cmd->data_len > 0U) && (cmd->data == NULL)) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	__ASSERT(net_buf_tailroom(buf) >= cmd->data_len, "No space in buffer");

	if (cmd->data_len > 0U) {
		net_buf_add_mem(buf, cmd->data, cmd->data_len);
	}
	err = bt_hci_cmd_send_sync(cmd->opcode, buf, rsp);
	if (err != 0) {
		LOG_ERR("Failed to send VS cmd");
	}

	return err;
}

__weak void aci_hal_end_of_radio_activity_event(uint8_t Last_State,
						uint8_t Next_State,
						uint32_t Next_State_SysTime,
						uint8_t Last_State_Slot,
						uint8_t Next_State_Slot)
{
	LOG_INF("ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT: \n    Last_State=0x%02x\n    Next_State=0x%02x\n    Next_State_SysTime=%u\n    Last_State_Slot=0x%02x\n    Next_State_Slot=0x%02x\n",
		Last_State,
		Next_State,
		Next_State_SysTime,
		Last_State_Slot,
		Next_State_Slot);

}

__weak void aci_hal_sync_event(uint8_t Group_Id,
			       uint32_t Next_Anchor_Point,
			       uint32_t Time_Stamp,
			       uint32_t Next_Sdu_Delivery_Timeout)
{
	LOG_INF("ACI_HAL_SYNC_EVENT: \n    Group_Id=0x%02x\n    Next_Anchor_Point=%u\n    Time_Stamp=%u\n    Next_Sdu_Delivery_Timeout=%u\n",
		Group_Id,
		Next_Anchor_Point,
		Time_Stamp,
		Next_Sdu_Delivery_Timeout);

}

static bool aci_hal_cb(struct net_buf_simple *buf)
{
	struct hci_vs_evt *evt;

	evt = net_buf_simple_pull_mem(buf, sizeof(*evt));

	switch(evt->evtcode) {
		case ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT:
			{
			aci_hal_end_of_radio_activity_event_rp0 *rp;
			rp = (aci_hal_end_of_radio_activity_event_rp0 *)evt->data;

			aci_hal_end_of_radio_activity_event(rp->Last_State,
							    rp->Next_State,
							    rp->Next_State_SysTime,
							    rp->Last_State_Slot,
							    rp->Next_State_Slot);
			}
			break;
		case ACI_HAL_SYNC_EVENT:
			{
			aci_hal_sync_event_rp0 *rp;
			rp = (aci_hal_sync_event_rp0 *)evt->data;

			aci_hal_sync_event(rp->Group_Id,
					   rp->Next_Anchor_Point,
					   rp->Time_Stamp,
					   rp->Next_Sdu_Delivery_Timeout);
			}
			break;
	}

	return true;
}

/**
 * @brief  This function register the vendor specific callback.
 * CONFIG_BT_HCI_VS_EVT_USER shall be defined to enable the callback registration.
 * The callback function aci_hal_end_of_radio_activity_event() and
 * aci_hal_sync_event() will be called when the corresponding event is received
 * from the controller.
 *
 * @return Value indicating success or error code.
 */
int aci_hal_init(void)
{
#if defined(CONFIG_BT_HCI_VS_EVT_USER)
	/* Register the prop callback */
	return bt_hci_register_vnd_evt_cb(aci_hal_cb);
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_HCI_VS_EVT_USER */
}

/**
 * @brief  This function sets the public Bluetooth address.
 *
 * @param uid_addr Pointer to a bt_addr_t structure to store the public address.
 * @return Value indicating success or error code.
 */
int aci_hal_set_pub_addr(bt_addr_t *uid_addr)
{
	aci_hal_write_config_data_cp0 parm;
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	aci_hal_write_config_data_rp0 *rp;
	uint8_t err;

	if(!uid_addr) {
		return -EINVAL;
	}
	/* Set Public Address parameter*/
	parm.Offset = CONFIG_DATA_PUBLIC_ADDRESS_OFFSET;
	parm.Length = CONFIG_DATA_PUBLIC_ADDRESS_LEN;
	bt_addr_copy((bt_addr_t *)parm.Value, uid_addr);

	hal_cmd.opcode = ACI_HAL_WRITE_CONFIG_DATA;
	hal_cmd.data_len = CONFIG_DATA_PUBLIC_ADDRESS_LEN+2;
	hal_cmd.data = (uint8_t *)&parm;
	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_write_config_data_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_write_config_data_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set public address status: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function gets the configured public Bluetooth address.
 *
 * @param uid_addr Pointer to a bt_addr_t structure to store the public address.
 * @return Value indicating success or error code.
 */
int aci_hal_get_pub_addr(bt_addr_t *uid_addr)
{
	aci_hal_read_config_data_cp0 parm;
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	int err = 0;
	aci_hal_read_config_data_rp0 *rp;

	/* Set Public Address parameter*/
	parm.Offset = CONFIG_DATA_PUBLIC_ADDRESS_OFFSET;

	hal_cmd.opcode = ACI_HAL_READ_CONFIG_DATA;
	hal_cmd.data_len = 1;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd,&rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len > sizeof(aci_hal_read_config_data_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_read_config_data_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to read public address, status: 0x%02X", rp->Status);
			err = -EIO;
		} else if (rp->Data_Length != CONFIG_DATA_PUBLIC_ADDRESS_LEN) {
			LOG_ERR("Invalid public address length: %d", rp->Data_Length);
			err = -EIO;
		} else {
			memcpy(uid_addr->val, &rp->Data[0], sizeof(uid_addr->val));
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function configures the RSSI golden range.
 *
 * @param lower_limit Lower limit of the golden range.
 * @param upper_limit Upper limit of the golden range.
 * @return Value indicating success or error code.
 */
int aci_hal_set_rssi_golden_range(int lower_limit , int upper_limit)
{
	aci_hal_write_config_data_cp0 parm;
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	aci_hal_write_config_data_rp0 *rp;
	uint8_t err;

	/* Set parameter*/
	parm.Offset = CONFIG_DATA_LL_RSSI_GOLDEN_RANGE_OFFSET;
	parm.Length = CONFIG_DATA_LL_RSSI_GOLDEN_RANGE_LEN;
	parm.Value[0] = lower_limit;
	parm.Value[1] = upper_limit;


	hal_cmd.opcode = ACI_HAL_WRITE_CONFIG_DATA;
	hal_cmd.data_len = parm.Length + 2;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_write_config_data_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_write_config_data_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set rssi golden range: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}


/**
 * @brief  This function configures the Link Layer LL RX ACL control.
 *
 * @param rx_pckt_count Number of rx packets expected to be received
 *        with a payload size of rx_pckt_len octets.
 * @param rx_pckt_len Length of rx packets expected to be received
 *        on the first rx_pckt_count slots.
 * @return Value indicating success or error code.
 */
int aci_hal_set_rx_acl_ctrl(uint8_t rx_pckt_count, uint8_t rx_pckt_len)
{
	aci_hal_write_config_data_cp0 parm;
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	aci_hal_write_config_data_rp0 *rp;
	uint8_t err;

	/* Set parameter*/
	parm.Offset = CONFIG_DATA_LL_RX_ACL_CTRL_OFFSET;
	parm.Length = CONFIG_DATA_LL_RX_ACL_CTRL_LEN;
	parm.Value[0] = rx_pckt_count;
	parm.Value[1] = rx_pckt_len;

	hal_cmd.opcode = ACI_HAL_WRITE_CONFIG_DATA;
	hal_cmd.data_len = parm.Length + 2;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_write_config_data_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_write_config_data_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set rx acl ctrl: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function sets the TX power level of the device.
 * By controlling the PA level, that determines the output power level(dBm)
 * at the IC pin.
 * When the system starts up or reboots, the default TX power level is used,
 * which is the maximum value. Once this command is given, the output power
 * changes instantly, regardless if there is BLE communication going on or not.
 * For example, for debugging purpose, the device can be set to advertise all
 * the time. By using this command, one can then observe the evolution of the
 * TX signal strength.
 * The system keeps the last received TX power level from the command, i.e. the
 * 2nd command overwrites the previous TX power level. The new TX power level
 * remains until another aci_hal_set_tx_power_level() call, or the system
 * reboots.
 *
 * @param en_high_power Enable High Power mode - Deprecated and ignored
 *        Values:
 *        - 0x00: Standard Power
 *        - 0x01: High Power
 * @param pa_level Power amplifier output level.
 *        Values:
 *        - 0x00 ... 0x23
 * @return Value indicating success or error code.
 */
int aci_hal_set_tx_power_level(uint8_t en_high_power, uint8_t pa_level)
{
	uint8_t parm[2];
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	aci_hal_set_tx_power_level_rp0 *rp;
	uint8_t err;

	/* Set parameter*/
	parm[0] = en_high_power;
	parm[1] = pa_level;

	hal_cmd.opcode = ACI_HAL_SET_TX_POWER_LEVEL;
	hal_cmd.data_len = 2u;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_set_tx_power_level_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_set_tx_power_level_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set tx power level: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}


/**
 * @brief  This function returns the number of packets sent in Direct Test Mode.
 * When the Direct TX test is started, a 16-bit counter is used to count how
 * many packets have been transmitted.
 * This command can be used to check how many packets have been sent during the
 * Direct TX test.
 * The counter starts from 0 and counts upwards. The counter can wrap and start
 * from 0 again. The counter is not cleared until the next Direct TX test
 * starts.
 *
 * @param[out] number_of_packets Number of packets sent during the last Direct
 *        TX test.
 * @return Value indicating success or error code.
 */
int aci_hal_le_tx_test_packet_number(uint32_t *number_of_packets)
{
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	int err = 0;
	aci_hal_le_tx_test_packet_number_rp0 *rp;

	*number_of_packets = 0;

	hal_cmd.opcode = ACI_HAL_LE_TX_TEST_PACKET_NUMBER;
	hal_cmd.data_len = 0;

	err = stm32wbax_send_vs_cmd(&hal_cmd,&rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len < sizeof(aci_hal_le_tx_test_packet_number_rp0)) {
			LOG_ERR("Invalid response length");
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_le_tx_test_packet_number_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to get tx test packet number, status: 0x%02X", rp->Status);
			err = -EIO;
		} else {
			*number_of_packets = rp->Number_Of_Packets;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function starts a carrier frequency, i.e. a tone, on a specific channel.
 * The frequency sine wave at the specific channel may be used for debugging
 * purpose only. The channel ID is a parameter from 0x00 to 0x27 for the 40 BLE
 * channels, e.g. 0x00 for 2.402 GHz, 0x01 for 2.404 GHz etc.
 * This command should not be used when normal BLE activities are ongoing.
 * The tone should be stopped by ACI_HAL_TONE_STOP command.
 *
 * @param rf_channel BLE Channel ID, from 0x00 to 0x27 meaning (2.402 +
 *        0.002*0xXX) GHz
 *        Device will continuously emit 0s, that means that the tone will be at
 *        the channel center frequency minus the maximum frequency deviation
 *        (250 kHz).
 *        Values:
 *        - 0x00 ... 0x27
 * @param freq_offset Frequency Offset for tone channel
 *        Values:
 *        - 0x00 ... 0xFF
 * @return Value indicating success or error code.
 */
int aci_hal_tone_start(uint8_t rf_channel, uint8_t freq_offset)
{
	uint8_t parm[2];
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	aci_hal_tone_start_rp0 *rp;
	uint8_t err;

	/* Set parameter*/
	parm[0] = rf_channel;
	parm[1] = freq_offset;

	hal_cmd.opcode = ACI_HAL_TONE_START;
	hal_cmd.data_len = 2u;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_tone_start_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_tone_start_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to start tone: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}


/**
 * @brief  This function is used to stop the previously started
 * aci_hal_tone_start().
 *
 * @return Value indicating success or error code.
 */
int aci_hal_tone_stop(void)
{
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	aci_hal_tone_stop_rp0 *rp;
	uint8_t err;

	hal_cmd.opcode = ACI_HAL_TONE_STOP;
	hal_cmd.data_len = 0u;


	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_tone_stop_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_tone_stop_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to stop tone: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function sets the bitmask associated to
 * ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT.
 * Only the radio activities enabled in the mask will be reported to
 * application by ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT
 *
 * @param radio_activity_mask Bitmask of radio events
 *        Flags:
 *        - 0x0001: Idle
 *        - 0x0002: Advertising
 *        - 0x0004: Peripheral connection
 *        - 0x0008: Scanning
 *        - 0x0020: Central connection
 *        - 0x0040: TX test mode
 *        - 0x0080: RX test mode
 *        - 0x0200: Periodic advertising
 *        - 0x0400: Periodic sync
 *        - 0x0800: Iso broadcast
 *        - 0x1000: Iso sync
 *        - 0x2000: Iso peripheral connection
 *        - 0x4000: Iso central connection
 * @return Value indicating success or error code.
 */
int aci_hal_set_radio_activity_mask(uint16_t radio_activity_mask)
{
	uint8_t parm[2];
	aci_hal_cmd hal_cmd;
	aci_hal_set_radio_activity_mask_rp0 *rp;
	struct net_buf *rsp;
	int err;

	/* Set parameter*/
	parm[0] = (uint8_t) radio_activity_mask;
	parm[1] = (uint8_t) (radio_activity_mask >> 8);

	hal_cmd.opcode = ACI_HAL_SET_RADIO_ACTIVITY_MASK;
	hal_cmd.data_len = 2u;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_set_radio_activity_mask_rp0)) {
			LOG_ERR("Invalid response length %d !", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_set_radio_activity_mask_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set radio activity mask, status: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function is used to enable/disable the generation of HAL events.
 * If the bit in the Event_Mask is set to a one, then the event associated with
 * that bit will be enabled.
 *
 * @param event_mask ACI HAL event mask. Default: 0x00000000.
 *        Flags:
 *        - 0x00000000: No events specified (Default)
 *        - 0x00000002: ACI_HAL_SYNC_EVENT
 * @return Value indicating success or error code.
 */
int aci_hal_set_event_mask(uint32_t event_mask)
{
	uint8_t parm[4];
	aci_hal_cmd hal_cmd;
	aci_hal_set_event_mask_rp0 *rp;
	struct net_buf *rsp;
	int err;

	/* Set parameter*/
	parm[0] = (uint8_t) event_mask;
	parm[1] = (uint8_t) (event_mask >> 8);
	parm[2] = (uint8_t) (event_mask >> 16);
	parm[3] = (uint8_t) (event_mask >> 24);

	hal_cmd.opcode = ACI_HAL_SET_EVENT_MASK;
	hal_cmd.data_len = 4u;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_set_event_mask_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_set_event_mask_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set event mask, status: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function returns the status of the BLE links managed by
 * the device.
 *
 * @param[out] link_status Array of link status (up to 20 links plus 2 ISO
 *        streams). Each link status is 1 byte.
 *        Values:
 *        - 0x00: Idle
 *        - 0x01: Advertising
 *        - 0x02: Peripheral connection
 *        - 0x03: Scanning
 *        - 0x05: Central connection
 *        - 0x06: TX test mode
 *        - 0x07: RX test mode
 *        - 0x09: Periodic advertising
 *        - 0x0A: Periodic sync
 *        - 0x0B: Iso broadcast
 *        - 0x0C: Iso sync
 *        - 0x0D: Iso peripheral connection
 *        - 0x0E: Iso central connection
 * @param[out] link_connection_handle Array of connection handles (2 bytes).
 *        Valid only if the corresponding link status is "connected"
 * @return Value indicating success or error code.
 */
int aci_hal_get_link_status_v2(uint8_t* link_status,
			       uint16_t* link_connection_handle)
{
	aci_hal_cmd hal_cmd;
	aci_hal_get_link_status_v2_rp0 *rp;
	struct net_buf *rsp;
	int err;

	hal_cmd.opcode = ACI_HAL_GET_LINK_STATUS_V2;
	hal_cmd.data_len = 0;
	hal_cmd.data = NULL;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len > sizeof(aci_hal_get_link_status_v2_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_get_link_status_v2_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to get link status, status: 0x%02X", rp->Status);
			err = -EIO;
		} else {
			memcpy(link_status, rp->Link_Status, 22);
			memcpy(((uint8_t *)link_connection_handle),((uint8_t *)rp->Link_Connection_Handle), 44);
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function is used to configure ACI_HAL_SYNC_EVENT.
 *
 * @param group_id Identifier of the CIG or BIG.
 *        Values:
 *        - 0x00 ... 0xEF
 * @param enable_sync Enable or disable generation of sync event.
 *        Values:
 *        - 0x00 ... 0x01
 * @param enable_cb_trigger Enable or disable generation of calibration signal.
 *        Values:
 *        - 0x00 ... 0x01
 * @param trigger_source Identify trigger source (CIG or BIG).
 *        Values:
 *        - 0x00: CIG
 *        - 0x01: BIG
 * @return Value indicating success or error code.
 */
int aci_hal_set_sync_event_config(uint8_t group_id,
				  uint8_t enable_sync,
				  uint8_t enable_cb_trigger,
				  uint8_t trigger_source)
{
	uint8_t parm[4];
	aci_hal_cmd hal_cmd;
	aci_hal_set_sync_event_config_rp0 *rp;
	struct net_buf *rsp;
	int err;

	/* Set parameter*/
	parm[0] = group_id;
	parm[1] = enable_sync;
	parm[2] = enable_cb_trigger;
	parm[3] = trigger_source;

	hal_cmd.opcode = ACI_HAL_SET_SYNC_EVENT_CONFIG;
	hal_cmd.data_len = 4u;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_set_sync_event_config_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_set_sync_event_config_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set event mask, status: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function returns the value of the RSSI.
 * Note: in Direct Test Mode, the RSSI can only be read when the test has ended
 * (after LE Test End command).
 *
 * @param[out] rssi RSSI (signed integer).
 *        Units: dBm.
 *        Values:
 *        - 127: RSSI not available
 *        - -127 ... 20
 * @return Value indicating success or error code.
 */
int aci_hal_read_rssi( uint8_t* rssi)
{
	aci_hal_cmd hal_cmd;
	aci_hal_read_rssi_rp0 *rp;
	struct net_buf *rsp;
	int err;

	hal_cmd.opcode = ACI_HAL_READ_RSSI;
	hal_cmd.data_len = 0;
	hal_cmd.data = NULL;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_read_rssi_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_read_rssi_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set event mask, status: 0x%02X", rp->Status);
			err = -EIO;
		} else {
			*rssi = rp->RSSI;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function starts the continuous transmit test mode.
 *
 * @param rf_channel Index of the BLE RF channel used for test mode.
 *        Values:
 *        - 0x00 ... 0x27
 * @param PHY PHY used for test mode.
 *        Values:
 *        - 0x01: LE 1M PHY
 *        - 0x02: LE 2M PHY
 * @param pattern Data pattern used for test mode.
 *        Values:
 *        - 0x00: Pseudo-Random bit sequence 9
 *        - 0x01: Pattern of alternating bits '11110000'
 *        - 0x02: Pattern of alternating bits '10101010'
 *        - 0x03: Pseudo-Random bit sequence 15
 *        - 0x04: Pattern of All '1' bits
 *        - 0x05: Pattern of All '0' bits
 *        - 0x06: Pattern of alternating bits '00001111'
 *        - 0x07: Pattern of alternating bits '0101'
 * @return Value indicating success or error code.
 */
int aci_hal_continuous_tx_start(uint8_t rf_channel,
				uint8_t PHY,
				uint8_t pattern)
{
	uint8_t parm[3];
	aci_hal_cmd hal_cmd;
	aci_hal_continuous_tx_start_rp0 *rp;
	struct net_buf *rsp;
	int err;

	/* Set parameter*/
	parm[0] = rf_channel;
	parm[1] = PHY;
	parm[2] = pattern;

	hal_cmd.opcode = ACI_HAL_CONTINUOUS_TX_START;
	hal_cmd.data_len = 3u;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		if (rsp->len != sizeof(aci_hal_continuous_tx_start_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}
		rp = (aci_hal_continuous_tx_start_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set event mask, status: 0x%02X", rp->Status);
			err = -EIO;
		}
	}
	net_buf_unref(rsp);

	return err;
}

/**
 * @brief  This function encrypts or decrypts data following the Encrypted
 * Advertising Data scheme.
 * When encryption mode is selected, In_Data shall only contain the Payload
 * field to encrypt. The command adds the Randomizer and MIC fields in the
 * result. The result data length (Out_Data_Length) is equal to the input
 * length plus 9.
 * When decryption mode is selected, In_Data shall contain the full Encrypted
 * Data (Randomizer + Payload + MIC). The result data length (Out_Data_Length)
 * is equal to the input length minus 9.
 * If the decryption fails, the returned status is a failure status, otherwise
 * it is a success status.
 *
 * @param mode EAD operation mode: encryption or decryption.
 *        Values:
 *        - 0x00: Encryption
 *        - 0x01: Decryption
 * @param key Session key used for EAD operation (in Little Endian format).
 * @param IV Initialization vector used for EAD operation (in Little Endian
 *        format).
 * @param in_data_length Length of input data
 * @param in_data Input data
 * @param[out] out_data_length Length of result data
 * @param[out] out_data Result data
 * @return Value indicating success or error code.
 */
int aci_hal_ead_encrypt_decrypt(uint8_t mode,
				const uint8_t* key,
				const uint8_t* IV,
				uint16_t in_data_length,
				const uint8_t* in_data,
				uint16_t* out_data_length,
				uint8_t* out_data )
{

	aci_hal_ead_encrypt_decrypt_cp0 parm;
	aci_hal_cmd hal_cmd;
	struct net_buf *rsp;
	aci_hal_ead_encrypt_decrypt_rp0 *rp;
	size_t rsp_header_len;
	uint16_t out_cap;
	uint8_t err;

	if ((mode != 0x00U) && (mode != 0x01U)) {
		return -EINVAL;
	}

	if ((key == NULL) || (IV == NULL) || (out_data_length == NULL) || (out_data == NULL)) {
		return -EINVAL;
	}

	if ((in_data_length > 0U) && (in_data == NULL)) {
		return -EINVAL;
	}

	if (in_data_length > sizeof(parm.In_Data)) {
		return -EMSGSIZE;
	}

	out_cap = *out_data_length;
	if (out_cap == 0U) {
		return -EINVAL;
	}

	/* Set parameter*/
	parm.Mode = mode;
	memcpy(parm.Key,key,16);
	memcpy(parm.IV,IV,8);
	parm.In_Data_Length = in_data_length;
	if (in_data_length > 0U) {
		memcpy(parm.In_Data,in_data,in_data_length);
	}

	hal_cmd.opcode = ACI_HAL_EAD_ENCRYPT_DECRYPT;
	hal_cmd.data_len = in_data_length + 16 + 8 + 2 + 1;
	hal_cmd.data = (uint8_t *)&parm;

	err = stm32wbax_send_vs_cmd(&hal_cmd, &rsp);
	if (err) {
		LOG_ERR("Failed to send VS cmd");
		return err;
	} else {
		rsp_header_len = offsetof(aci_hal_ead_encrypt_decrypt_rp0, Out_Data);
		if (rsp->len < rsp_header_len) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}

		if (rsp->len > sizeof(aci_hal_ead_encrypt_decrypt_rp0)) {
			LOG_ERR("Invalid response length %d", rsp->len);
			net_buf_unref(rsp);
			return -EIO;
		}

		rp = (aci_hal_ead_encrypt_decrypt_rp0 *)rsp->data;
		if (rp->Status != 0) {
			LOG_ERR("Failed to set rx acl ctrl: 0x%02X", rp->Status);
			err = -EIO;
		} else {
			if (rp->Out_Data_Length > sizeof(rp->Out_Data)) {
				LOG_ERR("Invalid response out_data length %u", rp->Out_Data_Length);
				net_buf_unref(rsp);
				return -EIO;
			}

			if ((rsp_header_len + rp->Out_Data_Length) > rsp->len) {
				LOG_ERR("Truncated response length %d for out_data %u",
					rsp->len, rp->Out_Data_Length);
				net_buf_unref(rsp);
				return -EIO;
			}

			if (rp->Out_Data_Length > out_cap) {
				LOG_ERR("Output buffer too small: need %u have %u",
					rp->Out_Data_Length, out_cap);
				net_buf_unref(rsp);
				return -EMSGSIZE;
			}

			*out_data_length = rp->Out_Data_Length;
			memcpy(out_data, rp->Out_Data, rp->Out_Data_Length);
		}
	}
	net_buf_unref(rsp);

	return err;
}
