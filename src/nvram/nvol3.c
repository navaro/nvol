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


#include <string.h>
#include <common/debug.h>
#include <common/errordef.h>
#include "nvol3.h"
#include "nvram.h"

#define FLASH_READ(address, len, data)          nvram_read (address, len, data)
#define FLASH_WRITE(address, len, data)         nvram_write (address, len, data)
#define FLASH_ERASE(start, end)                 nvram_erase (start, end)


#pragma pack(1)
typedef struct NVOL3_SECTOR_RECORD_S {
        uint32_t    flags;                                      // flags indicate sector status
        uint32_t    reserved1 [2];                                      //
        uint32_t    version ;

//      char        reserved2 [NVOL3_PAGE_SIZE - sizeof(uint32_t)*4] ;


} NVOL3_SECTOR_RECORD_T;
#pragma pack()

// sector flags
#define NVOL3_SECTOR_EMPTY        0xFFFFFFFF
#define NVOL3_SECTOR_INITIALIZING 0xAAFFFFFF
#define NVOL3_SECTOR_VALID        0xAAAAFFFF
#define NVOL3_SECTOR_INVALID      0xAAAAAAAA

#define NVOL3_INVALID_VAR_IDX           ((uint16_t)-1)

static inline uint32_t
max_records (NVOL3_INSTANCE_T * instance)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    return (int32_t)((config->sector_size - NVOL3_PAGE_SIZE) / config->record_size) ;
}


static uint16_t
get_sector_version (uint32_t sector_addr, uint32_t * flags)
{
    NVOL3_SECTOR_RECORD_T sector ;

    if (flags) *flags = 0 ;

    /* read sector record */
    if (FLASH_READ (sector_addr, sizeof (NVOL3_SECTOR_RECORD_T), (uint8_t*)&sector) != EOK) {
        return NVOL3_SECTOR_VERSION_0 ;
    }

    if (flags) *flags = sector.flags;

    return (uint16_t) ~(sector.version) ;
}

static int32_t
set_sector_flags (uint32_t sector_addr, uint32_t flags, const NVOL3_CONFIG_T    * config)
{
    NVOL3_SECTOR_RECORD_T sector ;

    /* configure sector record */
    memset (&sector, 0x55, sizeof (NVOL3_SECTOR_RECORD_T)) ;
    sector.flags = flags;
    sector.version =  ~((uint32_t)config->version)  ;

    int32_t res = FLASH_WRITE((uint32_t)sector_addr, sizeof(NVOL3_SECTOR_RECORD_T), (uint8_t*)&sector)  ;

    uint32_t sector_flags;
    uint16_t sector_version = get_sector_version (sector_addr, &sector_flags) ;

    if (
          (sector_version != config->version) ||
          (sector_flags != flags)
    )
    {
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ASSERT,
            "NVOL3 :A: '%s' failed setting sector flags!!", config->name) ;

    }


    return res ;
}

static int32_t
erase_sector (uint32_t sector_addr, uint32_t sector_size)
{
    return FLASH_ERASE (sector_addr, sector_addr + sector_size) ;
}

static int32_t
read_variable_record_head (NVOL3_INSTANCE_T * instance, NVOL3_RECORD_HEAD_T *head, uint16_t idx)
{
    int32_t status  ;
    uint32_t offset ;
    const NVOL3_CONFIG_T    *   config = instance->config ;

    DBG_CHECK_NVOL3 (idx < max_records(instance), EFAIL,
            "read_variable_record_head idx %d", idx) ;

    offset = NVOL3_PAGE_SIZE + config->record_size * idx ;
    status = FLASH_READ (instance->sector + offset, sizeof (NVOL3_RECORD_HEAD_T), (uint8_t*)head) ;
    if (status != EOK) {
          DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                  "NVOL3 :E: read_variable_record_head read %d!", status) ;
          return status;
    }
    if (head->flags == NVOL3_RECORD_FLAGS_EMPTY) {
        return E_EMPTY ;
    }
    if (head->flags != NVOL3_RECORD_FLAGS_VALID) {
        return head->flags == NVOL3_RECORD_FLAGS_INVALID ? E_INVALID : E_UNKNOWN ;
    }
    if ((head->length > (config->record_size - sizeof (NVOL3_RECORD_HEAD_T))) ) {
        return E_UNKNOWN ;
    }

    return EOK ;
}

static int32_t
read_variable_record (NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T *rec, uint16_t idx, uint32_t bytes)
{
    int32_t status = EFAIL ;
    uint32_t offset ;
    const NVOL3_CONFIG_T    *   config = instance->config ;

    DBG_CHECK_NVOL3 (idx < max_records(instance), EFAIL,
            "read_variable_record idx %d", idx) ;

    offset = NVOL3_PAGE_SIZE + config->record_size * idx ;
    status = FLASH_READ (instance->sector + offset, sizeof (NVOL3_RECORD_HEAD_T), (uint8_t*)rec) ;
    if (status != EOK) {
          DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                  "NVOL3 :E: read_variable_record read %d!", status) ;
          return status;
    }
    if (rec->head.flags == NVOL3_RECORD_FLAGS_EMPTY) {
        return E_EMPTY ;
    }
    if (rec->head.flags != NVOL3_RECORD_FLAGS_VALID) {
        return rec->head.flags == NVOL3_RECORD_FLAGS_INVALID ? E_INVALID : E_UNKNOWN ;
    }
    if ((rec->head.length > (config->record_size - sizeof (NVOL3_RECORD_HEAD_T))) ) {
        return E_UNKNOWN ;
    }
    if (rec->head.length) {
        offset += sizeof (NVOL3_RECORD_HEAD_T) ;
        if (bytes == 0) bytes = rec->head.length ;
        else if (bytes > rec->head.length) bytes = rec->head.length ;
        status = FLASH_READ (instance->sector + offset, bytes, (uint8_t*)rec->key_and_data) ;
    }

    return status ;
}


static int32_t
write_variable_record (NVOL3_INSTANCE_T * instance,  uint32_t sector_addr, NVOL3_RECORD_T *rec, uint16_t idx )
{
    int32_t status ;
    uint32_t offset ;
    const NVOL3_CONFIG_T    *   config = instance->config ;

    DBG_CHECK_NVOL3 (idx < max_records(instance), EFAIL, "write_variable_record idx") ;
    if (!rec->head.flags) {
        DBG_CHECK_NVOL3 (rec->head.flags && (rec->head.flags != NVOL3_RECORD_FLAGS_EMPTY), EFAIL,
                "write_variable_record idx invalid header") ;
    }

    offset = NVOL3_PAGE_SIZE + config->record_size * idx  ;
    status = FLASH_WRITE (sector_addr + offset, rec->head.length + sizeof(NVOL3_RECORD_HEAD_T), (uint8_t*)rec) ;

    DBG_ASSERT_T (status == EOK, "nvol_write_variable_record failed!!!") ;

    return status ;
}

static int32_t
set_variable_record_flags (NVOL3_INSTANCE_T * instance,  uint32_t sector_addr, uint16_t flags, uint16_t idx )
{
    int32_t status ;
    uint32_t offset ;
    const NVOL3_CONFIG_T    *   config = instance->config ;

    DBG_CHECK_NVOL3 (idx < max_records(instance), EFAIL,
            "set_variable_record_flags idx %d >= %d", idx, max_records(instance)) ;

    offset = NVOL3_PAGE_SIZE + config->record_size * idx  ;
    status = FLASH_WRITE (sector_addr + offset, (uint32_t)sizeof(uint16_t) , (uint8_t*)&flags) ;
    DBG_ASSERT_T (status == EOK, "set_variable_record_flags failed!!!") ;

    return status ;
}

static int32_t
variable_record_valid (NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T *rec)
{
  uint16_t checksum;
  uint16_t byte;

  checksum = 0 ;
  for (byte = 0; byte < rec->head.length; byte++) {
      checksum += rec->key_and_data[byte];
  }
  checksum = 0x10000 - checksum;
  if (rec->head.checksum != checksum) return E_INVALID;

  return EOK;
}

static int32_t
insert_lookup_table (NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T* rec, uint16_t idx)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    struct dlist * m ;
    unsigned int localsize = rec->head.length - config->key_size ;
    if (localsize > config->local_size) localsize = 0 ;


    dictionary_remove (instance->dict, (char*)&rec->key_and_data, config->key_size) ;
    m = dictionary_install_size(instance->dict, (char*)&rec->key_and_data, config->key_size,
            sizeof(NVOL3_ENTRY_T) + localsize) ;

    if (m) {
        NVOL3_ENTRY_T * entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, m) ;
        entry->idx = idx ;
        entry->length = rec->head.length - config->key_size;
        if (localsize) {
            memcpy (entry->local, &rec->key_and_data[config->key_size], localsize) ;
        }
    } else {
        return E_NOMEM ;
    }

    return EOK ;
}


static int32_t
construct_lookup_table ( NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T* scratch)
{
    uint16_t idx   = 0 ;
    int32_t status ;
    const NVOL3_CONFIG_T    *   config = instance->config ;

    instance->inuse = 0 ;
    instance->invalid = 0 ;
    instance->error = 0 ;

    while (idx < max_records(instance)) {
        if ((status = read_variable_record (instance, scratch,  idx, 0)) == E_EMPTY) {
          // last record
          status = EOK ;
          break ;
        }
        else if (status == E_TIMEOUT) {
          idx++ ;
          instance->invalid++ ;
          status = EOK ;
          continue ;
        }
        else if (status == E_INVALID) {
          idx++ ;
          instance->invalid++ ;
          status = EOK ;
          continue ;

        }
        else if (status != EOK) {
          idx++ ;
          instance->error++ ;
          status = EOK ;
          continue ;
        }


        // if variable record is valid then add to lookup table
        if (variable_record_valid(instance, scratch) == EOK) {
            status = insert_lookup_table (instance, scratch, idx);
            if (status != EOK) {
                DBG_ASSERT_NVOL3 (0, "nvol3_load : construct_lookup_table out of memory!!!") ;
                break ;
            }

            instance->inuse++ ;

        } else {
            DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                    "NVOL3 :E: construct_lookup_table invalid record flags 0x%.2x len %d!",
                    (uint32_t)scratch->head.flags, (uint32_t)scratch->head.length) ;
            set_variable_record_flags (instance,  instance->sector, NVOL3_RECORD_FLAGS_INVALID, idx ) ;
            instance->error++ ;
            instance->invalid++ ;

        }

        idx++ ;
    }


    instance->next_idx = idx ;
    instance->version = get_sector_version (instance->sector, 0) ;
    if (instance->version != config->version) {
        return E_VERSION ;
    }

    return status  ;

}

static inline int32_t
move_sector ( NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T* scratch, uint32_t dst_addr)
{
      uint16_t idx   = 0 ;
      uint16_t dst_idx   = 0 ;
      uint32_t sector_flags;
  int32_t status ;
  const NVOL3_CONFIG_T  *   config = instance->config ;

  DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,
          "NVOL3 : : '%s' move sectors dst 0x%x src 0x%x",
          config->name, dst_addr, instance->sector) ;

   get_sector_version (dst_addr, &sector_flags) ;

  if (sector_flags != NVOL3_SECTOR_EMPTY)
  {
    // erase it
     DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_WARNING,
             "NVOL3 :W: '%s' move sectors dst not empty!", config->name) ;
     if ((status = erase_sector(dst_addr, config->sector_size)) != EOK) {
         return status ;
     }
  }

  // mark destination sector as being initialized
  if ((status = set_sector_flags(dst_addr, NVOL3_SECTOR_INITIALIZING, config)) != EOK) {
      DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
              "NVOL3 :E: '%s' move error initializing dst sector!", config->name) ;
      return status;
  }

  while (idx < max_records(instance)) {
      if ((status = read_variable_record (instance, scratch,  idx, 0)) == E_EMPTY) {
          // last record
          status = EOK ;
          break ;
      }
      else if (status == E_TIMEOUT) {
          idx++ ;
          status = EOK ;
          continue ;
      }
      else if (status == E_INVALID) {
          idx++ ;
          status = EOK ;
          continue ;

      }
      else if (status != EOK) {
          idx++ ;
          status = EOK ;
          continue ;
      }


        // if variable record is valid then add to lookup table
        if (variable_record_valid(instance, scratch) == EOK) {
          if ((status = write_variable_record (instance,  dst_addr, scratch, dst_idx )) != EOK) {
              DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                      "NVOL3 :E: '%s' move error write dst sector!", config->name) ;
              //return status ;
          }
          dst_idx++ ;

        } else {
            DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                    "NVOL3 :E: move  sector invalid record flags 0x%.2x len %d!",
                    (uint32_t)scratch->head.flags, (uint32_t)scratch->head.length) ;

        }

        idx++ ;
  }

  if ((status = set_sector_flags( dst_addr, NVOL3_SECTOR_VALID, config)) != EOK) {
      DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
              "NVOL3 :E: '%s' swap error initializing dst sector!", config->name) ;
      return status;
  }



  return status  ;

}


static NVOL3_ENTRY_T*
retrieve_lookup_table (NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T* value)
{
    struct dlist * m ;
      const NVOL3_CONFIG_T  *   config = instance->config ;

        m = dictionary_get (instance->dict, (const char*)value->key_and_data, config->key_size) ;

    if (m) {
        //return (NVOL3_ENTRY_T*) m->value ;
        return (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, m) ;

    }
    return 0;

}


static int32_t
swap_sectors (NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T* scratch)
{
  uint16_t dst_idx;
  uint32_t sector_flags;
  uint32_t src_addr, dst_addr ;
  int32_t status ;
    const NVOL3_CONFIG_T    *   config = instance->config ;
    struct dlist * m ;
    struct dictionary_it  it ;


    DBG_ASSERT_NVOL3 ((instance->sector == config->sector1_addr) ||
            (instance->sector == config->sector2_addr) , "swap_sectors invalid") ;

    if (instance->sector == config->sector1_addr) {
        src_addr = config->sector1_addr ;
        dst_addr = config->sector2_addr ;
    } else {
        src_addr = config->sector2_addr ;
        dst_addr = config->sector1_addr ;
    }

  DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,
          "NVOL3 : : '%s' swap sectors dst 0x%x src 0x%x",
          config->name, dst_addr, src_addr) ;

   get_sector_version (dst_addr, &sector_flags) ;

  if (sector_flags != NVOL3_SECTOR_EMPTY)
  {
    // erase it
     DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_WARNING,
             "NVOL3 :W: '%s' swap sectors dst not empty!", config->name) ;
     if ((status = erase_sector(dst_addr, config->sector_size)) != EOK) {
         return status ;
     }
  }

  // mark destination sector as being initialized
  if ((status = set_sector_flags(dst_addr, NVOL3_SECTOR_INITIALIZING, config)) != EOK) {
      DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
              "NVOL3 :E: '%s' swap error initializing dst sector!", config->name) ;
      return status;
  }

  dst_idx = 0;

  for (m = dictionary_it_first (instance->dict, &it) ; m;  ) {
      NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, m) ;
      status = read_variable_record (instance, scratch, entry->idx, 0)  ;

      if (status == EOK) {
          if ((status = write_variable_record (instance,  dst_addr, scratch, dst_idx )) != EOK) {
              DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                      "NVOL3 :E: '%s' swap error write dst sector!", config->name) ;
              //return status ;
          }
          entry->idx = dst_idx ;

      } else {
          DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                  "NVOL3 :E: '%s' swap error read dst sector idx %d!",
                  config->name, entry->idx) ;
      }

      dst_idx++;
      m = dictionary_it_next (instance->dict, &it) ;
  }



  if ((status = set_sector_flags( dst_addr, NVOL3_SECTOR_VALID, config)) != EOK) {
      DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
              "NVOL3 :E: '%s' swap error initializing dst sector!", config->name) ;
      return status;
  }

  // now using destination sector
  instance->sector = dst_addr ;

  // regenerate lookup table
  construct_lookup_table(instance, scratch);

  if ((status = set_sector_flags(src_addr, NVOL3_SECTOR_INVALID, config)) != EOK) {
      DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
              "NVOL3 :E: '%s' swap error deinitialising src sector!", config->name) ;
      return status;
  }

  // erase source sector
  if ((status = erase_sector(src_addr, config->sector_size)) != EOK) {
     return status ;
  }
  DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_LOG,
          "NVOL3 : : swap sectors completed") ;

  return EOK;
}

static int32_t
init_sectors (NVOL3_INSTANCE_T * instance, NVOL3_RECORD_T* scratch)
{
  uint32_t sector1_flags, sector2_flags;
  const NVOL3_CONFIG_T  *   config = instance->config ;

  /*sector1_version =*/ get_sector_version (config->sector1_addr, &sector1_flags) ;
  /*sector2_version =*/ get_sector_version (config->sector2_addr, &sector2_flags) ;

  // if sector 1 has invalid flags then erase it
  if ((sector1_flags != NVOL3_SECTOR_EMPTY)        &&
      (sector1_flags != NVOL3_SECTOR_INITIALIZING) &&
      (sector1_flags != NVOL3_SECTOR_VALID)        &&
      (sector1_flags != NVOL3_SECTOR_INVALID))
  {
        erase_sector(config->sector1_addr, config->sector_size) ;
        sector1_flags = NVOL3_SECTOR_EMPTY;
  }

  // if sector 2 has invalid flags then erase it
  if ((sector2_flags != NVOL3_SECTOR_EMPTY)        &&
      (sector2_flags != NVOL3_SECTOR_INITIALIZING) &&
      (sector2_flags != NVOL3_SECTOR_VALID)        &&
      (sector2_flags != NVOL3_SECTOR_INVALID))
  {
        erase_sector(config->sector2_addr, config->sector_size);
        sector2_flags = NVOL3_SECTOR_EMPTY;
  }
    instance->next_idx = 0 ;

  // what happens next depends on status of both sectors
  switch (sector1_flags) {

    case NVOL3_SECTOR_EMPTY:
      switch (sector2_flags) {

        // sector 1 empty, sector 2 empty
        case NVOL3_SECTOR_EMPTY:
            erase_sector(config->sector1_addr, config->sector_size) ;

          // use sector 1
            instance->sector = config->sector1_addr ;
            if (set_sector_flags(instance->sector, NVOL3_SECTOR_VALID, config) != EOK) {
                return EFAIL ;
            }
            break ;


        // sector 1 empty, sector 2 initializing
        case NVOL3_SECTOR_INITIALIZING:
          // use sector 2
            instance->sector = config->sector2_addr ;
            if (set_sector_flags(instance->sector, NVOL3_SECTOR_VALID, config) != EOK) {
                return EFAIL ;
            }
            break ;

        // sector 1 empty, sector 2 valid
        case NVOL3_SECTOR_VALID:

            instance->sector = config->sector2_addr ;
            break ;

        // sector 1 empty, sector 2 invalid
        case NVOL3_SECTOR_INVALID:
          // swap sectors 2 -> 1

            instance->sector = config->sector2_addr ;
            construct_lookup_table (instance, scratch) ;
            if (swap_sectors (instance, scratch) != EOK) {
                return EFAIL ;
            }
            break ;
      }
      break;

    case NVOL3_SECTOR_INITIALIZING:
      switch (sector2_flags) {
        // sector 1 initializing, sector 2 empty
        case NVOL3_SECTOR_EMPTY:
          // use sector 1
            erase_sector(config->sector2_addr, config->sector_size) ;

            instance->sector = config->sector1_addr ;
            if (set_sector_flags(instance->sector, NVOL3_SECTOR_VALID, config) != EOK) {
                return EFAIL ;
            }
            break ;

        // sector 1 initializing, sector 2 initializing
        case NVOL3_SECTOR_INITIALIZING:
          // erase sector 2

            erase_sector(config->sector2_addr, config->sector_size) ;

            instance->sector = config->sector1_addr ;
            if (set_sector_flags(instance->sector, NVOL3_SECTOR_VALID, config) != EOK) {
                return EFAIL ;
            }
            break ;

        // sector 1 initializing, sector 2 valid
        case NVOL3_SECTOR_VALID:
          // erase sector 1
            erase_sector(config->sector1_addr, config->sector_size) ;

              // swap sectors 2 -> 1
            instance->sector = config->sector2_addr ;
            construct_lookup_table (instance, scratch) ;
            if (swap_sectors (instance, scratch) != EOK) {
                return EFAIL ;
            }
            break ;


        // sector 1 initializing, sector 2 invalid
        case NVOL3_SECTOR_INVALID:
          // erase sector 2
            erase_sector(config->sector2_addr, config->sector_size) ;

            // use sector 1
            instance->sector = config->sector1_addr ;
            if (set_sector_flags(instance->sector, NVOL3_SECTOR_VALID, config) != EOK) {
                return EFAIL ;
            }
            break ;

      }
      break;

    case NVOL3_SECTOR_VALID:
      switch (sector2_flags)  {
        // sector 1 valid, sector 2 empty
        case NVOL3_SECTOR_EMPTY:
          // sector 1 is active
            instance->sector = config->sector1_addr ;

            break ;

        // sector 1 valid, sector 2 initializing
        case NVOL3_SECTOR_INITIALIZING:
            erase_sector(config->sector2_addr, config->sector_size) ;


              // swap sectors 2 -> 1
            instance->sector = config->sector1_addr ;
            construct_lookup_table (instance, scratch) ;
            if (swap_sectors (instance, scratch) != EOK) {
                return EFAIL ;
            }
            break ;


        // sector 1 valid, sector 2 xxx
        case NVOL3_SECTOR_INVALID:
        case NVOL3_SECTOR_VALID:
          // erase sector 2 and use sector 1
            erase_sector(config->sector2_addr, config->sector_size) ;

            instance->sector = config->sector1_addr ;
            break ;

      }
      break;

    case NVOL3_SECTOR_INVALID:
      switch (sector2_flags)
      {
        // sector 1 invalid, sector 2 empty
        case NVOL3_SECTOR_EMPTY:
            erase_sector(config->sector2_addr, config->sector_size) ;

          // swap sectors 1 -> 2
            instance->sector = config->sector1_addr ;
            construct_lookup_table (instance, scratch) ;
            if (swap_sectors (instance, scratch) != EOK) {
                return EFAIL ;
            }
            break ;


        // sector 1 invalid, sector 2 initializing
        case NVOL3_SECTOR_INITIALIZING:
              // erase sector 1
            erase_sector(config->sector1_addr, config->sector_size) ;

              // use sector 2
            instance->sector = config->sector2_addr ;
            if (set_sector_flags(instance->sector, NVOL3_SECTOR_VALID, config) != EOK) {
                return EFAIL ;
            }
            break ;

        case NVOL3_SECTOR_VALID:
              // erase sector 1
            erase_sector(config->sector1_addr, config->sector_size) ;

              // use sector 2
            instance->sector = config->sector2_addr ;
            break ;


        // sector 1 invalid, sector 2 invalid
        case NVOL3_SECTOR_INVALID:
          // both sectors invalid so try to recover sector 1
            erase_sector(config->sector2_addr, config->sector_size) ;

          // swap sectors 1 -> 2
            instance->sector = config->sector1_addr ;
            construct_lookup_table (instance, scratch) ;
            if (swap_sectors (instance, scratch) != EOK) {
                return EFAIL ;
            }

            break ;

      }
      break;
  }

  return construct_lookup_table (instance, scratch) ;
}

/**
 * @brief   nvol3_load.
 * @details Loads the volume defined in the config of the instance parameter
 *
 * @param[in/out] instance
 *
 * @return
 * @retval EOK          success.
 * @retval EFAIL        read or write to FLASH failed.
 * @retval E_NOMEM       alloc failed.
 *
 * @nvol
 */
int32_t
nvol3_load (NVOL3_INSTANCE_T* instance)
{
    int32_t status  ;
    const NVOL3_CONFIG_T    *   config = instance->config ;
    NVOL3_RECORD_T* scratch = NVOL3_MALLOC (NVOL3_HEAP_SPACE, config->record_size) ;

    DBG_ASSERT_NVOL3 (config->record_size - sizeof (NVOL3_RECORD_HEAD_T) > 0, "nvol3_load param!") ;

    instance->sector = 0 ;
    instance->next_idx = 0 ;
    instance->inuse = 0 ;
    instance->invalid = 0 ;

    if (scratch) {

        if (instance->dict) {
            dictionary_destroy (instance->dict) ;
            instance->dict = 0 ;

        }
        instance->dict = dictionary_init(NVOL3_HEAP_SPACE, config->keyspec,
            config->hashsize) ;



        if (instance->dict) {
            status = init_sectors (instance, scratch) ;

        } else {
            status = E_NOMEM ;
        }

        NVOL3_FREE (NVOL3_HEAP_SPACE, scratch) ;

        if (status == EOK) {

            nvol3_entry_log_status (instance, 0) ;

        } else {
            DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ASSERT,
                    "NVOL3 :A: '%s' failed loading with %d!!", config->name, status) ;

        }
    } else {
        status = E_NOMEM ;
    }

    return status ;
}

/**
 * @brief   nvol3_validate.
 * @details Check if the volume on FLASH is a valid volume
 *
 * @param[in] instance
 *
 * @return
 * @retval EOK          success.
 * @retval EFAIL        Invalid volume.
 * @retval EVERSION     Incorrect sector version.
 *
 * @nvol
 */
int32_t
nvol3_validate (NVOL3_INSTANCE_T* instance)
{
      const NVOL3_CONFIG_T  *   config = instance->config ;
      uint32_t sector1_flags ;
      uint32_t sector2_flags ;
      uint16_t sector1_version = get_sector_version (config->sector1_addr, &sector1_flags) ;
      uint16_t sector2_version = get_sector_version (config->sector2_addr, &sector2_flags) ;


      if (  (sector1_flags == NVOL3_SECTOR_INITIALIZING) ||
              (sector1_flags == NVOL3_SECTOR_VALID) ||
              (sector1_flags == NVOL3_SECTOR_INVALID)) {
              if ((sector1_version == config->version)) {
                  return EOK ;
              } else {
                  return E_VERSION ;
              }
      }
      if (  (sector2_flags == NVOL3_SECTOR_INITIALIZING) ||
              (sector2_flags == NVOL3_SECTOR_VALID) ||
              (sector2_flags == NVOL3_SECTOR_INVALID)) {
              if ((sector2_version == config->version)) {
                  return EOK ;
              } else {
                  return E_VERSION ;
              }
      }


      return EFAIL ;
}

/**
 * @brief   nvol3_reset.
 * @details Erase the volume
 *
 * @param[in] instance
 *
 * @return
 * @retval EOK          success.
 * @retval EFAIL        read or write to FLASH failed.
 * @retval E_NOMEM       alloc failed.
 *
 * @nvol
 */
int32_t
nvol3_reset (NVOL3_INSTANCE_T* instance)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    if (instance->dict) dictionary_destroy (instance->dict) ;
    instance->dict = 0 ;

    erase_sector(config->sector1_addr, config->sector_size) ;
    erase_sector(config->sector2_addr, config->sector_size) ;

    uint32_t sector1_flags, sector2_flags;
    uint16_t sector1_version = get_sector_version (config->sector1_addr, &sector1_flags) ;
    uint16_t sector2_version = get_sector_version (config->sector2_addr, &sector2_flags) ;

    if (
          (sector1_version) ||
          (sector2_version) ||
          (sector1_flags != 0xFFFFFFFF) ||
          (sector2_flags != 0xFFFFFFFF)
    )
    {
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ASSERT,
            "NVOL3 :A: '%s' failed resetting!!", config->name) ;

    }

    return nvol3_load (instance) ;
}

/**
 * @brief   nvol3_delete.
 * @details Erase the volume
 *
 * @param[in] instance
 *
 * @return
 * @retval EOK          success.
 * @retval EFAIL        read or write to FLASH failed.
 * @retval E_NOMEM       alloc failed.
 *
 * @nvol
 */
int32_t
nvol3_delete (NVOL3_INSTANCE_T* instance)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    erase_sector(config->sector1_addr, config->sector_size) ;
    erase_sector(config->sector2_addr, config->sector_size) ;

    if (instance->dict) dictionary_destroy (instance->dict) ;
    instance->dict = 0 ;

    return EOK ;
}

/**
 * @brief   nvol3_unload.
 * @details Unload the volume and free all memory
 *
 * @param[in] instance
 *
 * @nvol
 */
void
nvol3_unload (NVOL3_INSTANCE_T* instance)
{

    if (instance->dict) dictionary_destroy (instance->dict) ;
    instance->dict = 0 ;

    return  ;
}


static int32_t
record_set (NVOL3_INSTANCE_T* instance, NVOL3_ENTRY_T* entry, NVOL3_RECORD_T *value, uint32_t key_and_data_length)
{
    uint16_t flags;
    uint16_t byte;
    // uint16_t size = value->head.length ;
    int32_t status ;
    uint16_t num_same_bytes = 0;
    uint16_t idx = NVOL3_INVALID_VAR_IDX;
    const NVOL3_CONFIG_T    *   config = instance->config ;
    NVOL3_RECORD_T* var = 0 ;

    DBG_CHECK_NVOL3(config->record_size - sizeof (NVOL3_RECORD_HEAD_T) >= key_and_data_length,
            EFAIL,
            "nvol3_set_variable_record param!") ;


      if (entry) {
        idx = entry->idx ;
          var = NVOL3_MALLOC (NVOL3_HEAP_SPACE, config->record_size) ;
          if (var == 0) return E_NOMEM ;
          flags = NVOL3_RECORD_FLAGS_NEW ;

          // get variable record
          if (read_variable_record (instance, var, idx, 0) == EOK) {
            if (key_and_data_length == var->head.length) {
                for (byte = 0; byte < key_and_data_length; byte++) {
                  if (value->key_and_data[byte] == var->key_and_data[byte]) num_same_bytes++;
                  else break ;
                }
            }

          }

      } else if (dictionary_count(instance->dict) >= (max_records(instance) - NVOL3_HEADROOM)) {
           DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_INFO,
                   " nvol3_set_variable_record volume full (%d records)",
                   dictionary_count(instance->dict)) ;

          return EFAIL;

      } else {
          flags = NVOL3_RECORD_FLAGS_PENDING ;

      }

      if (num_same_bytes == key_and_data_length){
            DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_INFO,
                    " NVOL3_SetVariable no update required") ;
            if (var) NVOL3_FREE (NVOL3_HEAP_SPACE, var) ;
            return EOK;
      }

      // if sector is full then swap sectors
      if (instance->next_idx >= max_records(instance))
      {
            if (var == 0) {
              var = NVOL3_MALLOC (NVOL3_HEAP_SPACE, config->record_size) ;
              if (var == 0) return E_NOMEM ;
            }
            if (swap_sectors (instance, var) != EOK) {
                NVOL3_FREE (NVOL3_HEAP_SPACE, var) ;
                return EFAIL ;
            }
            idx = NVOL3_INVALID_VAR_IDX ;
            // if no space in new sector then no room for more variables
            //if (instance->num_records >= (max_records(instance) - NVOL3_HEADROOM)) return EFAIL;
            NVOL3_FREE (NVOL3_HEAP_SPACE, var) ;

      } else if (var) {
          NVOL3_FREE (NVOL3_HEAP_SPACE, var) ;
      }


      value->head.length = key_and_data_length ;
      value->head.flags = flags ;
      value->head.checksum = 0 ;
      value->head.reserved = 0xFFFF ;
      for (byte = 0; byte < key_and_data_length; byte++) {
            value->head.checksum += value->key_and_data[byte];
      }
      value->head.checksum = 0x10000 - value->head.checksum;

      if (config->write_cb) {
          if ((status = config->write_cb (instance, value, config->ctx)) != EOK) {
              return status ;
          }
      }

      // store record in sector
      if ((status = write_variable_record(instance, instance->sector, value, instance->next_idx)) != EOK) {
          set_variable_record_flags (instance,  instance->sector, NVOL3_RECORD_FLAGS_INVALID, instance->next_idx ) ;
          instance->next_idx++ ;
          instance->invalid++ ;
          instance->error++ ;
          return status;
      }
      status = set_variable_record_flags (instance,  instance->sector, NVOL3_RECORD_FLAGS_VALID, instance->next_idx ) ;
      instance->next_idx++ ;
      instance->inuse++ ;

      if (status != EOK) {
          return status ;
      }

      // get offset of next free location
      if ((status = insert_lookup_table(instance, value, instance->next_idx-1 )) != EOK) {
          //set_variable_record_flags (instance,  instance->sector, 0, instance->next_idx ) ;
          DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_ERROR,
                  "nvol :E: '%s' failed insert %d", config->name, status) ;

          return status ;
      }



      if (idx != NVOL3_INVALID_VAR_IDX) {
          // mark previous record as invalid
          set_variable_record_flags (instance,  instance->sector, NVOL3_RECORD_FLAGS_INVALID, idx) ;
          instance->inuse-- ;
          instance->invalid++ ;

      }

      return EOK ;

}

/**
 * @brief   nvol3_set_record.
 * @details Update or create a record in the volume.
 * @notes   The header part of the record are used by the nvol2 and need
 *          not be initialized by the caller.
 *
 * @param[in] instance
 * @param[in] id
 * @param[in] value
 * @param[in] value_length
 *
 * @return
 * @retval EOK          success.
 * @retval EFAIL        read or write to FLASH failed.
 * @retval E_NOMEM       alloc failed.
 *
 * @nvol
 */
int32_t
nvol3_record_set (NVOL3_INSTANCE_T* instance, NVOL3_RECORD_T *value, uint32_t key_and_data_length)
{
    //const NVOL3_CONFIG_T  *   config = instance->config ;

    NVOL3_ENTRY_T* entry ;
    entry = retrieve_lookup_table(instance, value);

    return record_set (instance, entry, value, key_and_data_length) ;
}

static int32_t
_record_get (NVOL3_INSTANCE_T* instance, NVOL3_RECORD_T *record, struct dlist * m)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    uint16_t idx = NVOL3_INVALID_VAR_IDX;
      int32_t status  ;


    memset (&record->head, 0, sizeof (NVOL3_RECORD_HEAD_T)) ;
    if (m) {
        //NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*) m->value ;
        NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, m) ;
        if (entry->length <= config->local_size) {
            memcpy (record->key_and_data, dictionary_get_key (instance->dict, m), config->key_size) ;
            memcpy (&record->key_and_data[config->key_size], entry->local, entry->length) ;
            return entry->length + config->key_size;
        }
        idx = entry->idx ;

    }

    if (idx == NVOL3_INVALID_VAR_IDX) {
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_LOG,
                " NVOL get record for 0x%x not in lookup table!",
                *((uint32_t*)record->key_and_data)) ;
        return EFAIL;
    }

    // get variable record
    status = read_variable_record (instance, record, idx, 0) ;

    if (status < 0) return status ;
    return record->head.length ;
}

/**
 * @brief   nvol3_get_record.
 * @details Read a record in the volume.
 * @notes   The header part of the record are used by the nvol2 and need
 *          not be initialized by the caller. The caller should allocate
 *          enough memory for value to read the complete record.
 *
 * @param[in] instance
 * @param[in] id
 * @param[in] out
 *
 * @return
 * @retval EOK          success.
 * @retval EFAIL        read or write to FLASH failed.
 * @retval E_NOMEM       alloc failed.
 *
 * @nvol
 */
int32_t
nvol3_record_get (NVOL3_INSTANCE_T* instance, NVOL3_RECORD_T *record)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    //uint16_t idx = NVOL3_INVALID_VAR_IDX;

    struct dlist * m = dictionary_get (instance->dict, (const char*)record->key_and_data, config->key_size) ;
    return _record_get (instance, record, m) ;
}

/**
 * @brief   nvol3_record_delete.
 * @details Delete a record in the volume.
 * @notes   The header part of the record are used by the nvol2 and need
 *          not be initialized by the caller.
 *
 * @param[in] instance
 * @param[in] id
 * @param[in] value
 *
 * @return
 * @retval EOK          success.
 * @retval EFAIL        read or write to FLASH failed.
 * @retval E_NOMEM       alloc failed.
 *
 * @nvol
 */
int32_t
nvol3_record_delete (NVOL3_INSTANCE_T* instance, NVOL3_RECORD_T *record)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;


    memset (&record->head, 0, sizeof (NVOL3_RECORD_HEAD_T)) ;

    struct dlist * m = dictionary_get (instance->dict, (const char*)record->key_and_data, config->key_size) ;
    if (m) {
        //NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*) m->value ;
        NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, m) ;
        uint16_t idx = entry->idx ;
        set_variable_record_flags (instance,  instance->sector, NVOL3_RECORD_FLAGS_INVALID, idx ) ;
        instance->inuse-- ;
        instance->invalid++ ;

        dictionary_remove(instance->dict, (const char*)record->key_and_data, config->key_size) ;

        return EOK ;
    }

    return E_NOTFOUND ;
}

/**
 * @brief   nvol3_record_data_length.
 * @details Return the length of a record.
 *
 * @param[in] instance
 * @param[in] id
 *
 * @return              Length of the record
 * @retval EFAIL        Not a record.
 *
 * @nvol
 */
int32_t
nvol3_record_key_and_data_length (NVOL3_INSTANCE_T* instance, const char * key)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    NVOL3_RECORD_HEAD_T head ;

    struct dlist * m = dictionary_get (instance->dict, key, config->key_size) ;
      if (m) {
          //NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*) m->value ;
          NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, m) ;

          if (read_variable_record_head (instance, &head, entry->idx) == EOK) {
              return head.length ;

          }

      }

      return EFAIL ;
}

/**
 * @brief   nvol3_is_record.
 * @details
 *
 * @param[in] instance
 * @param[in] id
 *
 * @return
 * @retval EOK          Record exist.
 * @retval EFAIL        Not a record.
 *
 * @nvol
 */
int32_t
nvol3_record_status (NVOL3_INSTANCE_T* instance, const char * key)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    struct dlist * m = dictionary_get (instance->dict, key, config->key_size) ;

    return m ? EOK : E_NOTFOUND ;
}

/**
 * @brief   nvol3_iterator_first.
 * @details Initialize the iterator and return the first record in the volume.
 *
 * @param[in] instance
 * @param[out] value
 * @param[in/out] it
 *
 * @return
 * @retval EOK          Record exist.
 * @retval EFAIL        FLASH read write or emty volume.
 *
 * @nvol
 */
int32_t
nvol3_record_first (NVOL3_INSTANCE_T* instance, NVOL3_RECORD_T *value, NVOL3_ITERATOR_T * it)
{
    //const NVOL3_CONFIG_T  *   config = instance->config ;
    struct dlist * m = dictionary_it_first (instance->dict, &it->it) ;
    if (m) {
        return _record_get (instance, value, m) ;
    }


    return E_EOF ;
}

/**
 * @brief   nvol3_record_next.
 * @details Increments the iterator and return the value.
 *
 * @param[in] instance
 * @param[out] value
 * @param[in/out] it
 *
 * @return
 * @retval EOK          Record exist.
 * @retval EFAIL        FLASH read write or last record.
 *
 * @nvol
 */
int32_t
nvol3_record_next (NVOL3_INSTANCE_T* instance, NVOL3_RECORD_T *value, NVOL3_ITERATOR_T * it)
{
    //const NVOL3_CONFIG_T  *   config = instance->config ;
    struct dlist * m = dictionary_it_next (instance->dict, &it->it) ;
    if (m) {
        return _record_get (instance, value, m) ;
    }

    return E_EOF ;
}


/**
 * @brief   nvol3_entry_first.
 * @details Initialize the iterator and return the first record in the volume.
 *
 * @param[in] instance
 * @param[in/out] it
 *
 * @return
 * @retval EOK          Record exist.
 * @retval EFAIL        FLASH read write or emty volume.
 *
 * @nvol
 */
int32_t
nvol3_entry_first (NVOL3_INSTANCE_T* instance, NVOL3_ITERATOR_T * it)
{
    int32_t status = EFAIL ;
    struct dlist * m = dictionary_it_first (instance->dict, &it->it) ;
    if(m) {
        status = EOK ;
    }

    return status ;
}

/**
 * @brief   nvol3_entry_next.
 * @details Increments the iterator and return the value.
 *
 * @param[in] instance
 * @param[in/out] it
 *
 * @return
 * @retval EOK          Record exist.
 * @retval EFAIL        FLASH read write or last record.
 *
 * @nvol
 */
int32_t
nvol3_entry_next (NVOL3_INSTANCE_T* instance, NVOL3_ITERATOR_T * it)
{
    int32_t status = EFAIL ;
    struct dlist * m = dictionary_it_next (instance->dict, &it->it) ;
    if(m) {
        status = EOK ;

    }

    return status ;
}

/**
 * @brief   nvol3_entry_next.
 * @details Increments the iterator and return the value.
 *
 * @param[in] instance
 * @param[in/out] it
 *
 * @return
 * @retval EOK          Record exist.
 * @retval E_NOTFOUND    FLASH read write or last record.
 *
 * @nvol
 */
int32_t
nvol3_entry_at (NVOL3_INSTANCE_T* instance, const char * key, NVOL3_ITERATOR_T * it)
{
    int32_t status = E_NOTFOUND ;
    const NVOL3_CONFIG_T    *   config = instance->config ;

    struct dlist * m = dictionary_it_at (instance->dict, key, config->key_size, &it->it) ;
    if(m) {
        status = EOK ;

    }

    return status ;
}

int32_t
nvol3_entry_data (NVOL3_INSTANCE_T* instance, NVOL3_ITERATOR_T * it, char ** data)
{
    //NVOL3_ENTRY_T * entry = (NVOL3_ENTRY_T *)it->it.np->value ;
    NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, it->it.np) ;
    *data   =   (char *)entry->local ;

    return entry->length ;
}

const char *
nvol3_entry_key (NVOL3_INSTANCE_T* instance, NVOL3_ITERATOR_T * it)
{
    return dictionary_get_key (instance->dict, it->it.np) ;
}


int32_t
nvol3_entry_save (NVOL3_INSTANCE_T* instance, NVOL3_ITERATOR_T * it)
{
    int32_t status = E_NOMEM ;
    const NVOL3_CONFIG_T    *   config = instance->config ;
    //NVOL3_ENTRY_T * entry = (NVOL3_ENTRY_T *)it->it.np->value ;
    NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, it->it.np) ;

    //if (it) {
        NVOL3_RECORD_T* value = NVOL3_MALLOC (NVOL3_HEAP_SPACE, config->record_size) ;

        if (value) {
            memcpy (value->key_and_data, dictionary_get_key (instance->dict, it->it.np), config->key_size) ;
            memcpy (value->key_and_data + config->key_size, entry->local, entry->length) ;

            status = record_set (instance, entry, value, config->key_size + entry->length) ;

            NVOL3_FREE (NVOL3_HEAP_SPACE, value) ;
        }

    //}

    return status ;
}

int32_t
nvol3_entry_delete (NVOL3_INSTANCE_T* instance, NVOL3_ITERATOR_T * it)
{
    int32_t status ;
    const NVOL3_CONFIG_T    *   config = instance->config ;
    //NVOL3_ENTRY_T * entry = (NVOL3_ENTRY_T *)it->it.np->value ;
    NVOL3_ENTRY_T* entry = (NVOL3_ENTRY_T*)dictionary_get_value(instance->dict, it->it.np) ;

    uint16_t idx = entry->idx ;
    status = set_variable_record_flags (instance,  instance->sector, NVOL3_RECORD_FLAGS_INVALID, idx ) ;
    instance->inuse-- ;
    instance->invalid++ ;

    if (dictionary_remove(instance->dict, dictionary_get_key (instance->dict, it->it.np), config->key_size) == 0) {
        status = E_NOTFOUND ;
    }

    return status ;

}


int32_t
nvol3_callback_tallie (struct NVOL3_INSTANCE_S * inst, struct NVOL3_RECORD_S * record, uint32_t ctx)
{

    return EOK ;
}

void
nvol3_entry_log_status (NVOL3_INSTANCE_T* instance, uint32_t verbose)
{
    const NVOL3_CONFIG_T    *   config = instance->config ;
    DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT, "NVOL3 : : '%s' %d / %d records loaded",
            config->name, dictionary_count(instance->dict), max_records(instance)) ;
    if (verbose) {
        uint32_t sector1_flags, sector2_flags;
        uint16_t sector1_version = get_sector_version (config->sector1_addr, &sector1_flags) ;
        uint16_t sector2_version = get_sector_version (config->sector2_addr, &sector2_flags) ;

        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "record  : %d recordsize", config->record_size) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : 0x%.6x 1st sector version 0x%.4x flags 0x%.8x", config->sector1_addr, (uint32_t)sector1_version, sector1_flags) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : 0x%.6x 2nd sector version 0x%.4x flags 0x%.8x", config->sector2_addr, (uint32_t)sector2_version, sector2_flags) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : 0x%.6x sector size", config->sector_size) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : %d loaded", dictionary_count(instance->dict)) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : %d inuse", instance->inuse) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : %d invalid", instance->invalid) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : %d error", instance->error) ;
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : %d lookup table bytes",
                dictionary_count(instance->dict) * (sizeof(struct dlist *) + config->key_size + config->local_size)) ;

        unsigned int s = dictionary_hashtab_size (instance->dict) ;
        unsigned int i ;
        unsigned int empty = 0, max = 0, used = 0 ;

        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : %d dict hash size (%d)",
                s, dictionary_count(instance->dict)) ;

        for (i=0; i< s; i++) {
            unsigned int cnt = dictionary_hashtab_cnt (instance->dict, i) ;
            if (cnt > max) max = cnt ;
            if (!cnt) empty++ ;
            if (cnt) used++ ;

        }
        DBG_MESSAGE_NVOL3 (DBG_MESSAGE_SEVERITY_REPORT,    "        : dict hash - max %d, empty %d, used %d",
                max, empty, used) ;

    }
}


