#ifndef _LIB_EASY3332_H
#define _LIB_EASY3332_H
/****************************************************************************
       Copyright (c) 2005, Infineon Technologies.  All rights reserved.

                               No Warranty
   Because the program is licensed free of charge, there is no warranty for
   the program, to the extent permitted by applicable law.  Except when
   otherwise stated in writing the copyright holders and/or other parties
   provide the program "as is" without warranty of any kind, either
   expressed or implied, including, but not limited to, the implied
   warranties of merchantability and fitness for a particular purpose. The
   entire risk as to the quality and performance of the program is with
   you.  should the program prove defective, you assume the cost of all
   necessary servicing, repair or correction.

   In no event unless required by applicable law or agreed to in writing
   will any copyright holder, or any other party who may modify and/or
   redistribute the program as permitted above, be liable to you for
   damages, including any general, special, incidental or consequential
   damages arising out of the use or inability to use the program
   (including but not limited to loss of data or data being rendered
   inaccurate or losses sustained by you or third parties or a failure of
   the program to operate with any other programs), even if such holder or
   other party has been advised of the possibility of such damages.
 ****************************************************************************
   Module      : lib_easy3332.h
   Date        : 2005-06-07
   Description : This header file contains global interface functions for
                 the board driver.
*******************************************************************************/

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ====================================== */
/* Board Driver/Device Control Functions  */
/* ====================================== */

/* traces */
IFX_void_t  easy3332_set_tracelevel   (IFX_uint32_t fdBoard, IFX_uint32_t level);

/* versions */
IFX_int32_t easy3332_get_drvversion   (IFX_uint32_t fdBoard,
                                       IFX_char_t   *vers_string);
IFX_int32_t easy3332_get_boardversion (IFX_uint32_t fdBoard,
                                       IFX_int32_t  *board_vers);


/* board configuration */
IFX_int32_t easy3332_reset_config     (IFX_uint32_t fdBoard);
IFX_int32_t easy3332_set_config       (IFX_uint32_t fdBoard,
                                       IFX_uint32_t access_mode,
                                       IFX_uint32_t clock_rate);
IFX_int32_t easy3332_get_config       (IFX_uint32_t fdBoard,
                                       IFX_uint32_t *access_mode,
                                       IFX_uint32_t *clock_rate);
/* reset operations */
IFX_int32_t easy3332_reset_chip       (IFX_uint32_t fdBoard,
                                       IFX_uint32_t chip_num);
IFX_int32_t easy3332_chip_setreset    (IFX_uint32_t fdBoard,
                                       IFX_uint32_t chip_num);
IFX_int32_t easy3332_chip_clearreset  (IFX_uint32_t fdBoard,
                                       IFX_uint32_t chip_num);

/* board initialization */
IFX_int32_t easy3332_init_board       (IFX_uint32_t fdBoard,
                                      IFX_uint32_t *chips_num);
IFX_int32_t easy3332_get_chipparam    (IFX_uint32_t fdBoard,
                                       IFX_uint32_t chip_num,
                                       IFX_uint32_t *access_mode,
                                       IFX_uint32_t *clock_rate,
                                       IFX_uint32_t *base_address,
                                       IFX_int32_t  *irq_num);

/* conversion functions accross drivers */
IFX_int32_t easy3332_b2v_ifce_accessmode (IFX_uint32_t access_mode);
IFX_int32_t easy3332_v2b_ifce_accessmode (IFX_uint32_t vin_access_mode);

/* common user operations */
IFX_int32_t easy3332_set_led          (IFX_uint32_t  fdBoard,
                                       IFX_uint32_t  led_num,
                                       IFX_boolean_t led_state);
IFX_int32_t easy3332_set_accessmode   (IFX_uint32_t fdBoard,
                                       IFX_uint32_t access_mode);
IFX_int32_t easy3332_set_clockrate    (IFX_uint32_t fdBoard,
                                       IFX_uint32_t clock_rate);

/* register operations */
IFX_int32_t easy3332_write_reg        (IFX_uint32_t fdBoard,
                                       IFX_uint8_t reg_offset,
                                       IFX_uint8_t reg_value);
IFX_int32_t easy3332_read_reg         (IFX_uint32_t fdBoard,
                                       IFX_uint8_t reg_offset,
                                       IFX_uint8_t *reg_value);
IFX_int32_t easy3332_modify_reg       (IFX_uint32_t fdBoard,
                                       IFX_uint8_t reg_offset,
                                       IFX_uint8_t reg_mask,
                                       IFX_uint8_t reg_value);

/* ======================================= */
/* Vinetic Driver/Device Control Function  */
/* ======================================= */

IFX_int32_t vinetic_init_device       (IFX_uint32_t fdVinCtrlDev,
                                       VIN_ACCESS   vin_access_mode,
                                       IFX_uint32_t base_address,
                                       IFX_int32_t  irq_num);
IFX_int32_t vinetic_reset_device      (IFX_uint32_t fdVinCtrlDev);
#endif /* _LIB_EASY3332_H */
