/*
    Copyright (C) 2015-2023, Navaro, All Rights Reserved
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
 * @file    spiflash.c
 * @brief   spiflash.
 * @details SPI Memory.
 *
 * @addtogroup drivers
 * @details spiflash driver.
 * @{
 */

#include <string.h>
#include "hal.h"
#include "spiflash.h"
#include "../errordef.h"
#include "../os.h"
#include "../platform.h"
#include "../debug.h"

#define  SPIFLASH_LOCK()
#define  SPIFLASH_UNLOCK()

static SPIDriver*                       _spiflash_spidrv = 0 ;
static const SPIConfig *                _spiflash_config = 0 ;
static uint32_t                         _spiflash_erasing = 0 ;

#define ALLOW_EXTERNAL_MEMORY_READ      0
#define ALLOW_EXTERNAL_MEMORY_WRITE     0
#define ALLOW_EXTERNAL_MEMORY           (ALLOW_EXTERNAL_MEMORY_READ&ALLOW_EXTERNAL_MEMORY_WRITE)


static char                             _spiflash_buffer[SPIFLASH_LOCAL_BUFFER_SIZE]  PLATFORM_SECTION_DMA ;


#if !ALLOW_EXTERNAL_MEMORY
static inline int is_external_mem(uint32_t address)
{
    return !platform_is_dma_mem ((void*)address) ;
}

#endif

static void spiflash_cmd (uint8_t opcode, uint8_t *result, uint32_t len) ;
static void spiflash_cmd_write (uint8_t opcode, uint8_t *cmd, uint32_t len) ;
static void spiflash_cmd_addr (uint8_t opcode, uint32_t address, uint8_t *result, uint32_t len) ;
static void spiflash_cmd_addr_write (uint8_t opcode, uint32_t address, const uint8_t *data, uint32_t len) ;
static void read_buffer (uint8_t *result, uint32_t len) ;
static int32_t spiflash_wait_wip (void) ;
static int32_t spiflash_write_enable (void) ;

#define  spi_select(drv)        spiSelect(drv) ;
#define  spi_unselect(drv)      spiUnselect(drv) ;

void spiflash_test (void)
{
    uint32_t status = 0;
    uint32_t id = 0 ;
    uint32_t id2 = 0 ;

    uint32_t i = 0 ;
    uint32_t id_last = 0x18609d ; // 0xbf4226 ;
    (void)id_last ;
    while (1) {

        SPIFLASH_LOCK();
        spiAcquireBus (_spiflash_spidrv,_spiflash_config);
        id = 0 ;

        spiflash_cmd (SPIFLASH_READ_JEDEC_ID, (uint8_t*)&id, 3) ;
#if 1

        uint8_t * pid = (uint8_t*)&id2 ;
        spi_select (_spiflash_spidrv) ;
        spi_lld_polled_exchange (_spiflash_spidrv,  SPIFLASH_READ_JEDEC_ID) ;
        pid[0] = spi_lld_polled_exchange (_spiflash_spidrv, 0) ;
        pid[1] = spi_lld_polled_exchange (_spiflash_spidrv, 0) ;
        pid[2] = spi_lld_polled_exchange (_spiflash_spidrv, 0) ;
        spi_unselect (_spiflash_spidrv) ;
#else
      id2 = id_last ;

#endif
        spiReleaseBus (_spiflash_spidrv);
        SPIFLASH_UNLOCK();

        if (id != id2) {
            status++ ;
            id_last = id ;
        }
        os_thread_sleep(100);
        if (!(i%10)) {
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_REPORT,
                    "FLASH : : flash rom id 0x%x read %d error %d ",
                    id, i, status) ;

        }
        i++ ;
    }




}

/**
 * @brief   Initialises the driver.
 * @note    Configuration (such as speed) can change during spiAcquireBus() as
 *          defined in the configuration.
 * @param[in] spiddrv   Driver handle
 * @param[in] config    Driver config
 * @return              Error.
 */
int32_t
spiflash_init (struct SPIDriver * spiddrv, const struct SPIConfig * config)
{
    uint8_t status = EOK;
    uint32_t id = 0 ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG, "-->> spiflash_init") ;

    _spiflash_spidrv = (SPIDriver *)spiddrv ;
    _spiflash_config = (SPIConfig *)config ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);

    spiflash_cmd (SPIFLASH_RESET_ENABLE, 0, 0) ;
    spiflash_cmd (SPIFLASH_RESET, 0, 0) ;

    os_thread_sleep (1) ;

    spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;
    spiflash_cmd (SPIFLASH_READ_JEDEC_ID, (uint8_t*)&id, 3) ;

    /*
     * Chip specific initialisation
     */
    if (
            (id != 0x4226bf) &&
            (id != 0x5318b9) &&
            (id != 0x16609d) &&
            (id != 0x18609d) &&
            (id != 0x176001)
    ) { // 0x4226bf00
             spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;
            id = 0 ;
            spiflash_cmd (SPIFLASH_READ_JEDEC_ID, (uint8_t*)&id, 3) ;
            //DBG_ASSERT_T(id == 0x9d16609d, "FLASH init failed 0x%x!!!", id) ;
    }
    if (id == 0x4226bf) {
        // SST26VF032B - write enable
        spiflash_write_enable () ;
        spiflash_cmd (0x98, 0, 0) ;
        spiflash_wait_wip () ;

    }
    if (id == 0x176001) {
        id = 0 ;
        spiflash_cmd (SPIFLASH_CMD_WRITE_ENABLE_VOLATILE, 0, 0) ;
        spiflash_cmd_write (SPIFLASH_WRITE_REGISTERS, (uint8_t*)&id, 3) ;
        spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;

    }

    spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;
    if (status & (SPIFLASH_STATUS_E_ERR|SPIFLASH_STATUS_P_ERR)) {
        spiflash_cmd (SPIFLASH_CMD_CLSR, 0, 0) ;

    }
    spiflash_cmd (SPIFLASH_READ_STATUS2_REGISTER, (uint8_t*)&status, 1) ;

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : status 0x%x", (uint32_t)status);

    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : flash rom id 0x%x", id) ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_init") ;

    return status ;
}

/**
 * @brief   Disable writes / write protect FLASH.
 */
int32_t
spiflash_write_disable (void)
{
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_write_disable") ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);
    spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;
    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_write_disable");

    return EOK ;
}


/**
 * @brief   Enable writes
 */
static int32_t
spiflash_write_enable (void)
{
    static uint32_t status  ;
    uint32_t count = SPIFLASH_WRITE_ENABLE_RETRY ;
    int i = 0 ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_write_enable") ;

    spiflash_cmd (SPIFLASH_CMD_WRITE_ENABLE, 0, 0) ;
    spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : write enabled status 0x%x", (uint32_t)status) ;
    while (!(status & SPIFLASH_STATUS_WEL) && --count) {
        if (++i < SPIFLASH_WRITE_ENABLE_RETRY/2) os_thread_sleep(0) ;
        else os_thread_sleep(10) ;
        spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;
    }
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : write enable complete status 0x%x", (uint32_t)status) ;

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_write_enable") ;

    return count ? EOK : E_TIMEOUT ;
}

/**
 * @brief   wait for write in progress
 */
static int32_t
spiflash_wait_wip (void)
{
    uint8_t status  ;
    uint32_t count = SPIFLASH_WAIT_WIP_RETRY ;
    int i = 0 ;

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_wait_wip") ;

    spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : chip wip status 0x%x", (uint32_t)status) ;
    while ((status & SPIFLASH_STATUS_WIP) && --count) {
        if (++i < SPIFLASH_WAIT_WIP_RETRY/2) os_thread_sleep(0) ;
        else os_thread_sleep(4) ;
        spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;
    }

    if (status & (SPIFLASH_STATUS_E_ERR|SPIFLASH_STATUS_P_ERR)) {
        spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;
        spiflash_cmd (SPIFLASH_CMD_CLSR, 0, 0) ;
        return EFAIL ;
    }

    return count ? EOK : E_TIMEOUT ;
}

static uint32_t
spiflash_aquire ()
{
    uint32_t status  ;
    /*
    if (_spiflash_erasing) {
        status = spiflash_wait_wip () ;
        spiflash_cmd (SPIFLASH_CMD_ERASE_SUSPEND, 0, 0) ;
    }
    */
    status = spiflash_wait_wip () ;

    return status ;

}

static uint32_t
spiflash_release ()
{
    uint32_t status  = EOK ;

    return status ;
    /*
    static uint32_t sr1  ;

    if (_spiflash_erasing) {
        spiflash_wait_wip () ;
        spiflash_cmd (SPIFLASH_READ_STATUS2_REGISTER, (uint8_t*)&sr1, 1) ;
        if (sr1 & (SPIFLASH_STATUS_E_ERR|SPIFLASH_STATUS_P_ERR)) {
            spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;
            spiflash_cmd (SPIFLASH_CMD_CLSR, 0, 0) ;
            status = EFAIL ;
        }
        if (sr1 & SPIFLASH_STATUS2_ES) {
            spiflash_cmd (SPIFLASH_CMD_ERASE_RESUME, 0, 0) ;
            status = spiflash_wait_wip () ;
        } else {
            _spiflash_erasing = 0 ;
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_INFO,
                    "FLASH : : spiflash_release erase complete!!!") ;
        }
    } else {
            //DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
             *      "FLASH : : spiflash_write_in_progress 0x%x", status) ;
//          spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&sr1, 1) ;
//          if (sr1 & (SPIFLASH_STATUS_E_ERR|SPIFLASH_STATUS_P_ERR)) {
//              spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;
//              spiflash_cmd (SPIFLASH_CMD_CLSR, 0, 0) ;
//              status = EFAIL ;
//          } else {
//              status = status & SPIFLASH_STATUS_WIP ? E_BUSY : EOK ;
//              if (status & SPIFLASH_STATUS_WIP) {
//                  status = spiflash_wait_wip () ;
//              } else {
//                  status = EOK ;
//              }
//              status = EOK ;
//          }

    }

    return status ;
    */
}

/**
 * @brief   spiflash_sector_size.
 * @return  sector size in bytes.
 */
uint32_t
spiflash_sector_size (uint32_t addr)
{
    return SPIFLASH_SECTOR_SIZE_4K ;
#if 0
    if (addr < 0x10000) {
        return SPIFLASH_SECTOR_SIZE_4K ;
    }

    return SPIFLASH_SECTOR_SIZE_64K ;
#endif
}


/**
 * @brief   spiflash_erase_busy.
 * @return              bool.

uint32_t
spiflash_erase_busy (void)
{
    uint32_t erasing ;
    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);

    if (_spiflash_erasing) {
            spiflash_aquire () ;
            spiflash_release () ;
    }

    erasing = _spiflash_erasing ;

    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    return erasing ;
}
 */

/**
 * @brief   start erasing sector of address
 * @return              Error.
 */
int32_t
spiflash_sector_erase_start (uint32_t address)
{
    uint32_t status = EOK ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_sector_erase") ;

    DBG_ASSERT_SPIFLASH (!_spiflash_erasing,
                    "FLASH : : spiflash_sector_erase unexpected!") ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);

    if ((status = spiflash_wait_wip()) != EOK) {
        DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_ERROR,
                    "FLASH : : spiflash_sector_erase wip failed.") ;
    }
    else if ((status = spiflash_write_enable()) != EOK) {
        DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_ERROR,
                    "FLASH : : spiflash_sector_erase enable failed.") ;
        //status = E_TIMEOUT ;
    } else {

        if (spiflash_sector_size (address) == SPIFLASH_SECTOR_SIZE_4K) {
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : starting 4k sector erase...") ;
            spiflash_cmd_addr (SPIFLASH_CMD_SECTOR_4K_ERASE, address, 0, 0) ;
        } else {
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : starting 64k sector erase...") ;
            spiflash_cmd_addr (SPIFLASH_CMD_SECTOR_64K_ERASE, address, 0, 0) ;
        }

        spiflash_cmd_addr (SPIFLASH_CMD_SECTOR_4K_ERASE, address, 0, 0) ;


        for (int j = 0; (j < 10) && ((spiflash_wait_wip()) != EOK); j++) {
            os_thread_sleep(1);
        }

        /*
        spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;
        if (status & (SPIFLASH_STATUS_E_ERR|SPIFLASH_STATUS_P_ERR)) {
            spiflash_cmd (SPIFLASH_CMD_WRITE_DISABLE, 0, 0) ;
            spiflash_cmd (SPIFLASH_CMD_CLSR, 0, 0) ;
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_ERROR,
                    "spiflash_sector_erase start failed 0x%x.", status) ;

            status = EFAIL ;
        } else {
            status = EOK ;
        }
        if (!(status & SPIFLASH_STATUS_WIP)) {
            _spiflash_erasing = 0 ;
        }
        */


    }

    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : sector erase complete status 0x%x",
                    (uint32_t)status) ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_sector_erase") ;

    return status ;
}

/**
 * @brief   check is a page is dirty
 * @return              1 or 0 (TRUE / FALSE).
 */
uint32_t
spiflash_is_dirty (uint32_t addr_start, uint32_t addr_end)
{
    uint32_t res ;
    uint32_t dirty = 0 ;
#define READ_BUFFER_SIZE    (sizeof(_spiflash_buffer) - 4)
    uint8_t * pbuffer = (uint8_t*)&_spiflash_buffer[4] ;

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_is_dirty") ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);

    if ((res = spiflash_aquire ()) == EOK) {
        while (addr_end > addr_start) {
            uint32_t i ;
            uint32_t len = addr_end - addr_start ;
            if (len >= READ_BUFFER_SIZE) len = READ_BUFFER_SIZE ;
            else {
                memset (pbuffer, 0xFF, READ_BUFFER_SIZE) ;
            }
            spiflash_cmd_addr (SPIFLASH_CMD_READ, addr_start, pbuffer, len) ;
            for (i=0; i<(len/sizeof(uint32_t));i++) {
                if (((uint32_t *) pbuffer)[i] != 0xFFFFFFFF) {
                    dirty = 1 ;
                    break ;
                }
            }

            if (dirty) break ;
            addr_start += len ;

        }
        spiflash_release () ;

     } else {
         dirty = 1 ;

     }


    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_is_dirty") ;

    return dirty ;
}


/**
 * @brief   spiflash_sector_erase.
 *
 *
 * @return              Error.
 *
 * @init
 */
int32_t
spiflash_sector_erase (uint32_t addr_start, uint32_t addr_end)
{
    unsigned int erased = 0 ;
    unsigned int clean = 0 ;
    uint32_t starttime = os_sys_timestamp () ;
    uint32_t sectorsize ;

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_INFO,
                "FLASH : : erasing address 0x%x to 0x%x",
                addr_start, addr_end);

    while (addr_start < addr_end) {

        sectorsize = spiflash_sector_size(addr_start) ;
        if (spiflash_is_dirty(addr_start, addr_start+sectorsize)) {
            if (spiflash_sector_erase_start(addr_start) != EOK) {
                DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_ERROR,
                        "FLASH : : erased error at address 0x%u of 0x%u",
                        addr_start, addr_end) ;
                break ;
            }
            erased++ ;

        } else {
            clean++ ;
        }
        addr_start += sectorsize ;

    }

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_INFO,
                "FLASH : : sector erase done in %ums. %u erased, %u clean.",
                os_sys_timestamp() - starttime, erased, clean) ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);
    if (spiflash_wait_wip()) {
        DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_ERROR,
                "FLASH : : erase timeout.") ;
    }
    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                "FLASH : : erased 0x%x - 0x%x %d sectors 0x%x",
                addr_start, addr_end, erased) ;

    return addr_start >= addr_end ? EOK : EFAIL ;
}



/**
 * @brief   spiflash_chip_erase.
 * @return              Error.
 */
int32_t spiflash_chip_erase (void)
{
    uint32_t count = SPIFLASH_WAIT_CHIP_ERASE_RETRY ;
    uint32_t status = EOK ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_chip_erase") ;

    DBG_ASSERT_SPIFLASH (!_spiflash_erasing,
                    "FLASH : : spiflash_sector_erase unexpected!") ;

    SPIFLASH_LOCK();
    if (!_spiflash_erasing) {


        spiAcquireBus (_spiflash_spidrv,_spiflash_config);

        if ((status = spiflash_wait_wip()) != EOK) {
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_ERROR,
                    "FLASH : : spiflash_chip_erase wip failed.") ;
        }
        else if ((status = spiflash_write_enable()) != EOK) {
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_ERROR,
                    "FLASH : : spiflash_chip_erase enable failed.") ;
            //status = E_TIMEOUT ;
        } else {
            DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : starting chip erase...") ;
            spiflash_cmd (SPIFLASH_CMD_CHIP_ERASE, 0, 0) ;
            //status =  spiflash_wait_wip () ;
            spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;
            while ((status & SPIFLASH_STATUS_WIP) && --count) {

                if (_spiflash_config) {
                    spiReleaseBus (_spiflash_spidrv);
                    os_thread_sleep(100) ;
                    spiAcquireBus (_spiflash_spidrv,_spiflash_config);
                } else {
                    os_thread_sleep(100) ;
                }
                spiflash_cmd (SPIFLASH_READ_STATUS_REGISTER, (uint8_t*)&status, 1) ;

            }

            spiReleaseBus (_spiflash_spidrv);

        }
    } else {
        status = EFAIL ;
    }
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "FLASH : : chip erase complete status 0x%x", (uint32_t)status) ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_chip_erase") ;

    return status ;
}

/**
 * @brief   write bytes to FLASH.
 * @param[in] address   start address
 * @param[in] len       number of bytes
 * @param[in] data      bytes to write
 * @return              Error.
 */
int32_t spiflash_write (uint32_t address, uint32_t len, const uint8_t* data)
{
    uint32_t bytes ;
    uint32_t res = EOK ;

    bytes = 0x100 - (address & 0xFF) ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);

    if ((res = spiflash_aquire ()) == EOK) {


        //res = spiflash_wait_wip () ;
        while (len && (res == EOK)) {

            if (len < bytes) bytes = len ;

            if ((res = spiflash_write_enable()) == EOK) {
                spiflash_cmd_addr_write (SPIFLASH_CMD_PAGE_PROGRAM, address,
                                    data, bytes) ;
            }

            res = spiflash_wait_wip () ;

            data += bytes ;
            len -= bytes ;
            address += bytes ;
            bytes = 0x100 ;
        }

    } else {
        DBG_ASSERT_T(0,
                    "FLASH : : spiflash_write failed to aquire bus %d!!!",
                    res) ;
    }

    spiflash_release () ;

    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    return res ;
}

/**
 * @brief   read bytes from FLASH.
 * @param[in] address   start address
 * @param[in] len       number of bytes
 * @param[in] data      buffer to read to
 * @return              Error.
 */
int32_t
spiflash_read (uint32_t address, uint32_t len, uint8_t* data)
{
    uint32_t res = EOK ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_read") ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);

    spiflash_cmd_addr (SPIFLASH_CMD_READ, address, data, len) ;

    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_read") ;
    return res ;
}


int32_t
spiflash_read_sfdp (uint32_t address, uint32_t len, uint8_t* data)
{
    uint32_t res ;
    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "-->> spiflash_read_sfdp") ;

    SPIFLASH_LOCK();
    spiAcquireBus (_spiflash_spidrv,_spiflash_config);

    if ((res = spiflash_aquire ()) == EOK) {
        _spiflash_buffer[0] = SPIFLASH_CMD_READ_SFDP ;
        _spiflash_buffer[1] = ((uint8_t*)&address)[2] ;
        _spiflash_buffer[2] = ((uint8_t*)&address)[1] ;
        _spiflash_buffer[3] = ((uint8_t*)&address)[0] ;
        _spiflash_buffer[4] = 0 ;


        spi_select (_spiflash_spidrv) ;
        spiSend (_spiflash_spidrv, 5, _spiflash_buffer) ;
        if (len && data) {
            read_buffer (data, len) ;
        }

        spi_unselect (_spiflash_spidrv) ;
     }

    spiflash_release () ;

    spiReleaseBus (_spiflash_spidrv);
    SPIFLASH_UNLOCK();

    DBG_MESSAGE_SPIFLASH (DBG_MESSAGE_SEVERITY_DEBUG,
                    "<<-- spiflash_read_sfdp") ;
    return res ;
}


static void
read_buffer (uint8_t *result, uint32_t len)
{
#if  !ALLOW_EXTERNAL_MEMORY_WRITE
    if (is_external_mem((uint32_t)result)) {
        while (len > SPIFLASH_LOCAL_BUFFER_SIZE) {
            spiReceive (_spiflash_spidrv, SPIFLASH_LOCAL_BUFFER_SIZE, _spiflash_buffer) ;
            memcpy (result, _spiflash_buffer, SPIFLASH_LOCAL_BUFFER_SIZE) ;
            result += SPIFLASH_LOCAL_BUFFER_SIZE ;
            len -= SPIFLASH_LOCAL_BUFFER_SIZE ;
        }
        spiReceive (_spiflash_spidrv, len, _spiflash_buffer) ;
        memcpy (result, _spiflash_buffer, len) ;

    } else
#endif
    {
        spiReceive (_spiflash_spidrv, len, result) ;
    }

}

static void
write_buffer (const uint8_t *data, uint32_t len)
{
#if  !ALLOW_EXTERNAL_MEMORY_READ
    if (is_external_mem((uint32_t)data)) {
        while (len > SPIFLASH_LOCAL_BUFFER_SIZE) {
            memcpy (_spiflash_buffer, data, SPIFLASH_LOCAL_BUFFER_SIZE) ;
            spiSend (_spiflash_spidrv, SPIFLASH_LOCAL_BUFFER_SIZE,
                    _spiflash_buffer) ;
            data += SPIFLASH_LOCAL_BUFFER_SIZE ;
            len -= SPIFLASH_LOCAL_BUFFER_SIZE ;
        }

        memcpy (_spiflash_buffer, data, len) ;
        spiSend (_spiflash_spidrv, len, _spiflash_buffer) ;

    } else
#endif
    {
        spiSend (_spiflash_spidrv, len, data) ;
    }

}

/**
 * @brief   SPI FLASH read command
 * @param[in] opcode    command
 * @param[in] result    result buffer
 * @param[in] len    result buffer size
 */
void
spiflash_cmd (uint8_t opcode, uint8_t *result, uint32_t len)
{
    _spiflash_buffer[0] = opcode ;

    spi_select (_spiflash_spidrv) ;
    spiSend (_spiflash_spidrv, 1, _spiflash_buffer) ;
    if (len && result) {
        read_buffer (result, len) ;
    }

    spi_unselect (_spiflash_spidrv) ;

    return  ;
}

/**
 * @brief   SPI FLASH write command
 * @param[in] opcode    command
 * @param[in] result    command data buffer
 * @param[in] len    command data size
 */
void
spiflash_cmd_write (uint8_t opcode, uint8_t *cmd, uint32_t len)
{
    _spiflash_buffer[0] = opcode ;
    memcpy (&_spiflash_buffer[1], cmd, len) ;

    spi_select (_spiflash_spidrv) ;
    spiSend (_spiflash_spidrv, len, _spiflash_buffer) ;
    spi_unselect (_spiflash_spidrv) ;

    return  ;
}


/**
 * @brief   SPI FLASH address read command
 * @param[in] opcode    command
 * @param[in] address   FLASH address
 * @param[in] result    result buffer
 * @param[in] len    result buffer size
 */
void
spiflash_cmd_addr (uint8_t opcode, uint32_t address, uint8_t *result,
                uint32_t len)
{
    _spiflash_buffer[0] = opcode ;
#if SPIFLASH_ADDRESS_BYTES == 3
    _spiflash_buffer[1] = ((uint8_t*)&address)[2] ;
    _spiflash_buffer[2] = ((uint8_t*)&address)[1] ;
    _spiflash_buffer[3] = ((uint8_t*)&address)[0] ;
#else
    _spiflash_buffer[1] = ((uint8_t*)&address)[3] ;
    _spiflash_buffer[2] = ((uint8_t*)&address)[2] ;
    _spiflash_buffer[3] = ((uint8_t*)&address)[1] ;
    _spiflash_buffer[4] = ((uint8_t*)&address)[0] ;
#endif

    spi_select (_spiflash_spidrv) ;
    spiSend (_spiflash_spidrv, SPIFLASH_ADDRESS_BYTES + 1, _spiflash_buffer) ;
    if (len && result) {
        read_buffer (result, len) ;
    }

    spi_unselect (_spiflash_spidrv) ;

    return  ;
}

/**
 * @brief   SPI FLASH address write command
 * @param[in] opcode    command
 * @param[in] address   FLASH address
 * @param[in] result    command data buffer
 * @param[in] len    command data size
 */
static void
spiflash_cmd_addr_write (uint8_t opcode, uint32_t address, const uint8_t *data,
                uint32_t len)
{
    _spiflash_buffer[0] = opcode ;
#if SPIFLASH_ADDRESS_BYTES == 3
    _spiflash_buffer[1] = ((uint8_t*)&address)[2] ;
    _spiflash_buffer[2] = ((uint8_t*)&address)[1] ;
    _spiflash_buffer[3] = ((uint8_t*)&address)[0] ;
#else
    _spiflash_buffer[1] = ((uint8_t*)&address)[3] ;
    _spiflash_buffer[2] = ((uint8_t*)&address)[2] ;
    _spiflash_buffer[3] = ((uint8_t*)&address)[1] ;
    _spiflash_buffer[4] = ((uint8_t*)&address)[0] ;
#endif

    spi_select (_spiflash_spidrv) ;
    spiSend (_spiflash_spidrv, SPIFLASH_ADDRESS_BYTES + 1, _spiflash_buffer) ;
    if (len && data) {
        write_buffer (data, len) ;
    }

    spi_unselect (_spiflash_spidrv) ;

    return  ;
}

