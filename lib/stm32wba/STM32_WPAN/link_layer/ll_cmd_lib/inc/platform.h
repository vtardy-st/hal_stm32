/*$Id: //dwh/bluetooth/DWC_ble154combo/firmware/rel/2.00a-lca06/inc/platform.h#2 $*/
/**
 ********************************************************************************
 * @file    platform.h
 * @brief   this file include all helper functions in radio.c which is not described in Openthread
 *
 *
 ******************************************************************************
 * @copy
 * Synopsys MIT License:
 * Copyright (c) 2020-Present Synopsys, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of the software and
 * associated documentation files (the “Software”), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING, BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE
 * ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * */


#ifndef INCLUDE_PLATFORM_H_
#define INCLUDE_PLATFORM_H_

#include "radio.h"
#include "common_types.h"


/******************************************************************************************//**
 *  								 Enums
 *********************************************************************************************/


/******************************************************************************************//**
 *  								 Structures
 *********************************************************************************************/
/**  @ingroup systm_layer
*  @{
*/
/**
 * @brief Structure defines all callback functions used to notify MAC layer after specific event compeletion
 *
 */
struct mac_cbk_dispatch_tbl{
	/**
	 * @brief  Energy detection scan done callback
	 *
	 * @param[in] aInstance			: Radio instance
	 * @param[in] aEnergyScanMaxRssi: The result for ED operation which is RSSI value
	 *
	 * @note  This function is called after handling SM_DONE event in ral through a sequence of calls
	 * 		  ral_sm_done() -> ral_ed_scan_done() -> callback
	 */
	void (*mac_ed_scan_done)(otInstance *aInstance, int8_t aEnergyScanMaxRssi);
	/**
	 * @brief  Transmission done callback
	 *
	 * @param[in] aInstance	: Radio instance
	 * @param[in] aFrame	: Pointer to the transmitted frame
	 * @param[in] aAckFrame	: Pointer to the received ACK frame (in case of AR bit is detected in aFrame)
	 * @param[in] aError	: Error code if happens in transmission
	 *
	 * @note  This function is called after handling SM_DONE event in ral through a sequence of calls
	 * 		  ral_sm_done() -> ral_tx_done() -> callback
	 */
	void (*mac_tx_done)(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);
	/**
	 * @brief  Reception done callback
	 *
	 * @param[in] aInstance	: Radio instance
	 * @param[in] aFrame	: Pointer to the received frame
	 * @param[in] aError	: Error code if happens in transmission
	 *
	 * @note  This function is called after handling SM_DONE event in ral through a sequence of calls
	 * 		  ral_sm_done() -> ral_rx_done() -> callback
	 */
	void (*mac_rx_done)(otInstance *aInstance, otRadioFrame *aFrame, otError aError);
	/**
	 * @brief  The start of successful transmission callback
	 *
	 * @param[in] aInstance	: Radio instance
	 * @param[in] aFrame	: Pointer to the transmitted frame
	 * @param[in] aError	: Error code if happens in transmission
	 *
	 * @note  This function is called after calling proc_radio_tx which is called with calling of otplatRadioTransmit
	 *        in case of no error returned. These errors may be (CCA channel access failure, ral busy due to ongoing transmission ...etc )
	 */
	void (*mac_tx_strtd)(otInstance *aInstance, otRadioFrame *aFrame);
	/**
	 * @brief The frame update callback
	 *
	 * @param[in] aInstance	: Radio instance
	 * @param[in] aFrame	: Pointer to the frame
	 */
	void (*mac_frm_updtd)(otInstance *aInstance, otRadioFrame *aFrame);

};
/**
 * @}
 *
 */
/******************************************************************************************//**
 *  								 APIs
 *********************************************************************************************/
/**  @ingroup systm_layer_cmn_proc
*  @{
*/
/**
 * @brief  Initialize radio layer including registration for RAL callbacks, some filter configurations,
 * 		   and some automatic configurations like auto_sleep and auto_ack...etc
 *
 */
void radio_init(void);
/**
 * @brief  Register upper layer callback functions.
 * 		   This function called from upper layer init function (mac_init in case of MAC integration e.g. Zigbee stack).
 *
 * @param[in] ptr_cbk_dispatch_tbl : Pointer to callback functions to be registered
 */
void radio_call_back_funcs_init(struct mac_cbk_dispatch_tbl * ptr_cbk_dispatch_tbl);
/**
 * @}
 *
 */
/**
 * @brief Initialize openthread dispatch table
 *
 * @param sInstance	: Radio instance
 */
void otDispatch_tbl_init(otInstance *sInstance);


/**
 * @}
 *
 */
/**  @ingroup systm_layer_cmn_config
*  @{
*/
/**
 * @brief  Configure automatic ACK response by RAL layer including AIFS, ACK frame timeout.. etc
 *
 * @param[in] auto_ack_state : Auto ACK state (TRUE to enable, FALSE to disable)
 * @retval Status of configuration
 */
otError radio_set_auto_Ack_state(uint8_t auto_ack_state);
/**
 * @}
 *
 */
/**  @ingroup systm_layer_cmn_proc
*  @{
*/
/**
 * @brief Reset radio layer operation which in turns stop all running operations
 *
 * @return Status
 */
uint32_t radio_reset(void);


/**
 * @}
 *
 */
#if RADIO_CSMA
/**  @ingroup systm_layer_cmn_config
*  @{
*/
/**
 * @brief Set minimum CSMA backoff exponent
 *
 * @param[in] value : Minimum csma backoff exponent
 */
void set_min_csma_be(uint8_t value);
/**
 * @brief Set maximum CSMA backoff exponent
 *
 * @param[in] value : Maximum CSMA backoff exponent
 */
void set_max_csma_be(uint8_t value);
/**
 * @brief Set maximum CSMA backoff counter
 *
 * @param[in] value : Maximum CSMA backoff counter
 */
void set_max_csma_backoff(uint8_t value);

#endif /*end of RADIO_CSMA*/


/**
 * @}
 *
 */
/**  @ingroup systm_layer_cmn_proc
*  @{
*/
/**
 * @brief Generate random number
 *
 * @param[out] ptr_rnd		: Pointer to the output random bytes
 * @param[in] len	 		: Number of required random bytes
 * @param[in] check_cont_rx : Flag to check continuous reception
 * @return Status
 */
uint32_t mac_gen_rnd_num(uint8_t *ptr_rnd, uint16_t len, uint8_t check_cont_rx);


/**
 * @brief Run periodic calibration if temperature has changed
 *
 */
void radio_run_phy_clbr_on_temp_change(void);
#endif /*!SUPPORT_COEXISTENCE*/

#if DEFAULT_PHY_CALIBRATION_PERIOD
/**
 * @brief Get the calibration state of MAC channel.
 *
 * @param[in] channel		: MAC channel (11:26).
 * @retval Calibration state
 */
uint8_t radio_get_mac_ch_clbr_state(uint8_t channel);
#if SUPPORT_COEXISTENCE
/**
 * @brief Get the duration of single MAC channel calibration in sleep timer steps
 *
 * @return Duration of single MAC channel calibration in sleep timer steps
 */
uint32_t radio_get_mac_ch_clbr_durn(void);
/**
 * @brief Check any pending PHY calibration for MAC channels and run it before releasing the scheduler grant.
 */
void radio_check_mac_ch_phy_clbr_pending(void);
#endif /* SUPPORT_COEXISTENCE */
#endif /* DEFAULT_PHY_CALIBRATION_PERIOD */

#if (!SUPPORT_COEXISTENCE)
/**
 * @brief   Set link layer deep sleep mode
 *
 * @param[in] dp_slp_mode : Deep sleep mode (enable or disable)
 * @retval Status .
 */
otError radio_set_dp_slp_mode(dpslp_state_e dp_slp_mode);
#endif /*end of (!SUPPORT_COEXISTENCE)*/
#if SUPPORT_MAC_CONT_TESTING_CMDS_PHY_SUPPORT
/**
 * @brief Set the PHY continuous modulation and continuous wave modes upon enable
 *
 * @param[in] type			: The type of the modulation (0: modulation, 1: wave)
 * @param[in] enable_mode	: TRUE to enable the selected mode, FALSE to disable it
 * @param[in] chnl_num		: Channel number to be used in modulation (range: 0 to 15)
 * @param[in] pwr         	: The used power in dBm.
 * @retval Status
 */
otError platform_zigbee_set_phy_cont_modul_mode(uint8_t type, uint8_t enable_mode, uint8_t chnl_num, int8_t pwr);
#endif /*end of SUPPORT_MAC_CONT_TESTING_CMDS_PHY_SUPPORT */
#if SUPPORT_ANT_DIV
/**
 * @brief Set antenna diversity feature parameters
 *
 * @param[in] aInstance     	: Radio instance
 * @param[in] ptr_ant_div_params: Pointer to antenna diversity feature parameters
 * @retval Status
 */
otError radio_set_ant_div_params(otInstance *aInstance, antenna_diversity_st* ptr_ant_div_params);
/**
 * @brief Get antenna diversity feature parameters
 *
 * @param[in] aInstance 			: Radio instance
 * @param[out] ptr_ant_div_params	: Pointer to antenna diversity feature parameters
 */
void radio_get_ant_div_params(otInstance *aInstance, antenna_diversity_st* ptr_ant_div_params);
/**
 * @brief Set antenna diversity feature state
 *
 * @param[in] aInstance	: Radio instance
 * @param[in] enable	: Antenna diversity feature state (TRUE to enable, FALSE to disable)
 * @retval Status
 */
otError radio_set_ant_div_enable(otInstance *aInstance, uint8_t enable);
/**
 * @brief Set the default antenna ID of the antenna diversity feature to be used for transmission and reception
 *
 * @param[in] aInstance	    	: Radio instance
 * @param[in] default_ant_id	: Default antenna ID
 * @retval Status
 */
otError radio_set_default_ant_id(otInstance *aInstance, uint8_t default_ant_id);
/**
 * @brief Set the RSSI threshold for antenna diversity feature
 *
 * @param[in] aInstance	    : Radio instance
 * @param[in] rssi_threshold: RSSI threshold to compare with during antenna diversity measurements
 * @retval Status
 */
otError radio_set_ant_div_rssi_threshold(otInstance *aInstance, int8_t rssi_threshold);
#endif /* SUPPORT_ANT_DIV */
/**
 * @brief Set bus latency between thread core and radio platform
 *
 * @param[in] aInstance	    : Radio instance
 * @param[in] bus_latency	: Time in microseconds for latency between thread core and radio platform
 */
void otPlatRadioSetBusLatency(otInstance *aInstance, uint32_t bus_latency);
#if SUPPORT_OPENTHREAD_1_2
/**
 * @brief   Set clock accuracy
 *
 * @param[in] clk_acc: Clock accuracy in PPM
 */
void radio_set_clk_accuracy(uint8_t clk_acc);
/**
 * @brief   Set clock uncertainty
 *
 * @param[in] clk_uncer: Clock uncertainty in units of 10 us.
 */
void radio_set_clk_uncertainty(uint8_t clk_uncer);
#endif /*SUPPORT_OPENTHREAD_1_2*/


#if SUPPORT_CONFIG_LIB
/**
 * @brief Set configurable library feature parameters
 *
 * @param[in] aInstance 			: Radio instance
 * @param[in] ptr_config_lib_params : pointer to configurable library feature parameters
 * @retval Status
 */
otError radio_set_config_lib_params(otInstance *aInstance, config_lib_st* ptr_config_lib_params);
/**
 * @brief Get current configurable library parameters
 *
 * @param[in] aInstance	            : Radio instance
 * @param[out] ptr_config_lib_params: Pointer to configurable library feature parameters
 */
void radio_get_config_lib_params(otInstance *aInstance, config_lib_st* ptr_config_lib_params);

/**
 * @brief Set RTL polling time
 *
 * @param[in] aInstance       : Radio instance
 * @param[in] rtl_polling_time: RTL polling time value
 */
void radio_set_rtl_polling_time(otInstance *aInstance, uint8_t rtl_polling_time);

/**
 * @brief Get current RTL polling time
 *
 * @param[in] aInstance : Radio instance
 * @retval Current RTL polling time
 */
uint8_t radio_get_rtl_polling_time(otInstance *aInstance);


#endif /* SUPPORT_CONFIG_LIB */



#endif /* INCLUDE_PLATFORM_H_ */
