/*
 * Copyright 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <string.h>


/**
 * @brief  This function register the vendor specific callback.
 * CONFIG_BT_HCI_VS_EVT_USER shall be defined to enable the callback registration.
 * The callback function aci_hal_end_of_radio_activity_event() and
 * aci_hal_sync_event() will be called when the corresponding event is received
 * from the controller.
 *
 * @return Value indicating success or error code.
 */
int aci_hal_init(void);

/**
 * @brief  This function is notified when an ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT
 *         event is received from the controller.
 *
 * @param Last_State Completed radio event.
 * @param Next_State Incoming radio event.
 * @param Next_State_SysTime Remaining time before next event expressed in ms.
 * @param Last_State_Slot Not used.
 * @param Next_State_Slot Not used.
 * @return Value indicating success or error code.
 */
void aci_hal_end_of_radio_activity_event(uint8_t Last_State,
					 uint8_t Next_State,
					 uint32_t Next_State_SysTime,
					 uint8_t Last_State_Slot,
					 uint8_t Next_State_Slot);

/**
 * @brief  This function is notified when an ACI_HAL_SYNC_EVENT event
 *         is received from the controller.
 *
 * @param Group_Id Identifier of the CIG or BIG.
 * @param Next_Anchor_Point Timestamp of the next ISO anchor point.
 * @param Time_Stamp Timestamp of the calibration signal.
 * @param Next_Sdu_Delivery_Timeout Timestamp before which the SDU must
 *        be provided to the LL for being handled in the next ISO event.
 * @return Value indicating success or error code.
 */
void aci_hal_sync_event(uint8_t Group_Id,
			uint32_t Next_Anchor_Point,
			uint32_t Time_Stamp,
			uint32_t Next_Sdu_Delivery_Timeout);

/**
 * @brief  This function sets the public Bluetooth address.
 *
 * @param uid_addr Pointer to a bt_addr_t structure to store the public address.
 * @return Value indicating success or error code.
 */
int aci_hal_set_pub_addr(bt_addr_t *uid_addr);

/**
 * @brief  This function gets the configured public Bluetooth address.
 *
 * @param[out] uid_addr Pointer to a bt_addr_t structure to store the
 *	       public address.
 * @return Value indicating success or error code.
 */
int aci_hal_get_pub_addr(bt_addr_t *uid_addr);

/**
 * @brief  This function configures the RSSI golden range.
 *
 * @param lower_limit Lower limit of the golden range.
 * @param upper_limit Upper limit of the golden range.
 * @return Value indicating success or error code.
 */
int aci_hal_set_rssi_golden_range(int lower_limit , int upper_limit);

/**
 * @brief  This function configures the Link Layer LL RX ACL control.
 *
 * @param rx_pckt_count Number of rx packets expected to be received
 *        with a payload size of rx_pckt_len octets.
 * @param rx_pckt_len Length of rx packets expected to be received
 *        on the first rx_pckt_count slots.
 * @return Value indicating success or error code.
 */
int aci_hal_set_rx_acl_ctrl(uint8_t rx_pckt_count, uint8_t rx_pckt_len);

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
int aci_hal_set_tx_power_level(uint8_t en_high_power, uint8_t pa_level);

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
int aci_hal_le_tx_test_packet_number(uint32_t *number_of_packets);

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
int aci_hal_tone_start(uint8_t rf_channel, uint8_t freq_offset);

/**
 * @brief  This function is used to stop the previously started
 * aci_hal_tone_start().
 *
 * @return Value indicating success or error code.
 */
int aci_hal_tone_stop(void);

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
int aci_hal_set_radio_activity_mask(uint16_t radio_activity_mask);

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
int aci_hal_set_event_mask(uint32_t event_mask);

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
			       uint16_t* link_connection_handle);

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
				  uint8_t trigger_source);

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
int aci_hal_read_rssi( uint8_t* rssi);

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
				uint8_t pattern);

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
				uint8_t* out_data );