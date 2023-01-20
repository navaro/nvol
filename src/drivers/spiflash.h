/*
    Copyright (C) 2015-2021, Navaro, All Rights Reserved
    SPDX-License-Identifier: MIT

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */




/**
 * @file    spiflash.h
 * @brief   spiflash.
 * @details spiflash driver.
 *
 * @addtogroup drivers
 * @details spiflash driver.
 * @{
 */

#ifndef __SPIFLASH_H__
#define __SPIFLASH_H__

#include <stdint.h>

/*===========================================================================*/
/* Constants.                                                                */
/*===========================================================================*/

#define DBG_MESSAGE_SPIFLASH(severity, fmt_str, ...)        DBG_MESSAGE_T_LOG (SVC_LOGGER_TYPE(severity,0), 0, fmt_str, ##__VA_ARGS__)
#define DBG_ASSERT_SPIFLASH                                 DBG_ASSERT_T

#define SPIFLASH_ADDRESS_BYTES                  3
#define S25FlXXXS_USE_LOCK                      0


#define SPIFLASH_WRITE_REGISTERS                0x01        /**< @brief Status Register write.   */
#define SPIFLASH_READ_STATUS_REGISTER           0x05        /**< @brief Status Register Read.   */
#define SPIFLASH_READ_STATUS2_REGISTER          0x07        /**< @brief Status Register Read.   */
#define SPIFLASH_READ_PRODUCT_ID                0xAB        /**< @brief JEDEC ID Read.   */
#define SPIFLASH_READ_JEDEC_ID                  0x9F        /**< @brief JEDEC ID Read.   */
#define SPIFLASH_CMD_WRITE_ENABLE               0x06        /**< @brief Write Enable.   */
#define SPIFLASH_CMD_WRITE_ENABLE_VOLATILE      0x50        /**< @brief Write Enable.   */
#define SPIFLASH_CMD_WRITE_DISABLE              0x04        /**< @brief Write Disable.   */
#define SPIFLASH_CMD_CHIP_ERASE                 0xC7        /**< @brief Chip Erase.   */
#if SPIFLASH_ADDRESS_BYTES == 3
#define SPIFLASH_CMD_SECTOR_4K_ERASE            0x20        /**< @brief Sector Erase.   */
#define SPIFLASH_CMD_SECTOR_64K_ERASE           0xD8        /**< @brief Sector Erase.   */
#else
#define SPIFLASH_CMD_SECTOR_4K_ERASE            0x21        /**< @brief Sector Erase.   */
#define SPIFLASH_CMD_SECTOR_64K_ERASE           0xDC        /**< @brief Sector Erase.   */
#endif
#define SPIFLASH_CMD_ERASE_SUSPEND              0x75        /**< @brief Sector Erase.   */
#define SPIFLASH_CMD_ERASE_RESUME               0x7A        /**< @brief Sector Erase.   */
#if SPIFLASH_ADDRESS_BYTES == 3
#define SPIFLASH_CMD_PAGE_PROGRAM               0x02        /**< @brief Page Program.   */
#define SPIFLASH_CMD_READ                       0x03        /**< @brief Read.   */
#else
#define SPIFLASH_CMD_PAGE_PROGRAM               0x12        /**< @brief Page Program.   */
#define SPIFLASH_CMD_READ                       0x13        /**< @brief Read.   */
#endif
#define SPIFLASH_CMD_READ_SFDP                  0x5A        /**< @brief Read.   */
#define SPIFLASH_CMD_CLSR                       0x30        /**< @brief Clear Status Register.   */

#define SPIFLASH_RESET_ENABLE                   0x66        /**< @brief   */
#define SPIFLASH_RESET                          0x99        /**< @brief   */

#define SPIFLASH_STATUS_WIP                     (0x01 << 0) /**< @brief Write in progress   */
#define SPIFLASH_STATUS_WEL                     (0x01 << 1) /**< @brief Write enable   */
#define SPIFLASH_STATUS_E_ERR                   (0x01 << 5) /**< @brief    */
#define SPIFLASH_STATUS_P_ERR                   (0x01 << 6) /**< @brief    */
#define SPIFLASH_STATUS2_ES                     (0x01 << 1)

#define SPIFLASH_SECTOR_SIZE_4K                 0x1000
#define SPIFLASH_SECTOR_SIZE_64K                0x10000

#define SPIFLASH_WRITE_PAGE_RETRY               500
#define SPIFLASH_WRITE_ENABLE_RETRY             200
#define SPIFLASH_WAIT_WIP_RETRY                 400
#define SPIFLASH_WAIT_ERASE_RETRY               20
#define SPIFLASH_WAIT_CHIP_ERASE_RETRY          500

#define SPIFLASH_LOCAL_BUFFER_SIZE              0x400

/*===========================================================================*/
/* Data structures and types.                                                */
/*===========================================================================*/
struct SPIDriver ;
struct SPIConfig ;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

    extern int32_t      spiflash_init (struct SPIDriver * spiddrv, const struct SPIConfig * config) ;
    extern int32_t      spiflash_write_disable (void) ;
    extern int32_t      spiflash_chip_erase (void) ;

    extern uint32_t     spiflash_sector_size (uint32_t addr) ;
    extern int32_t      spiflash_sector_erase_start (uint32_t addr) ;
    extern uint32_t     spiflash_erase_busy (void) ;
    extern int32_t      spiflash_sector_erase (uint32_t addr_start, uint32_t addr_end) ;
    extern uint32_t     spiflash_is_dirty (uint32_t addr_start, uint32_t addr_end) ;

    extern int32_t      spiflash_write (uint32_t address, uint32_t len, const uint8_t* data) ;
    extern int32_t      spiflash_read (uint32_t address, uint32_t len, uint8_t* data) ;
    extern int32_t      spiflash_read_sfdp (uint32_t address, uint32_t len, uint8_t* data) ;

    extern void         spiflash_test (void) ;

#ifdef __cplusplus
}
#endif


#endif /* __SPIFLASH_H__ */
