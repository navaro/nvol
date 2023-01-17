# NVOL
## Overview
NVOL is a persistent FLASH registry, often referred to as EEPROM emulation. It offers a basic API for reading and writing indexed key/value pairs to and from a FLASH memory, similar to a registry. Some of the features of NVOL include:


- Internal data management and wear leveling using 2 dedicated sectors.
- Configurable sector sizes.
- Reliability with robustness against power failures and asynchronous resets.
- Ensure data integrity with automatic recovery of corrupt sectors or entries during startup.
- Indexed lookup with a built-in hash table.
- Configurable key sizes and types.
- Configurable value sizes.
- Configurable value cache in RAM.
- Efficient and lightweight implementaiton.
- Support transactions.


## Background
The disadvantage of a FLASH memory is that it cannot be erased or written in single bytes. FLASH memory can only be erased and written in large blocks. A typical erase
block size may be 4K or 8K bytes. For NVOL, the FLASH implementations should support partial writes, where an erased block may be written multiple times as long as bits only changed from “1” (erased state) to “0” (programmed state).

For the FLASH, the page is considered the smallest block size that can be erased. Each sector can be made up of one or more of these pages.

The first sector is initially erased. New registry entries are added sequentially to the FLASH. When an entry is updated, the old entry is marked as invalid and a new entry is written at the next available FLASH address. Once the first sector reaches capacity, all valid entries are copied to the second sector and the first sector is then erased. This process repeats itself.

NVOL efficiently handles and keeps track of valid entries and their locations on FLASH. The sectors are managed dynamically

## Implementation

Macros are available to create static instances of NVOL, which can then be used with the API. An example of a declaration would be:
```
#define NVOL3_REGISTRY_START                0
#define NVOL3_REGISTRY_SECTOR_SIZE          STORAGE_64K
#define NVOL3_REGISTRY_SECTOR_COUNT         2

#define REGISTRY_KEY_LENGTH                 24
#define REGISTRY_VALUE_LENGT_MAX            224

NVOL3_INSTANCE_DECL(_regdef_nvol3_entry,
        NVOL3_REGISTRY_START,
        NVOL3_REGISTRY_START + NVOL3_REGISTRY_SECTOR_SIZE,
        NVOL3_REGISTRY_SECTOR_SIZE,
        REGISTRY_KEY_LENGTH,            /*key_size*/
        DICTIONARY_KEYSPEC_BINARY(6),   /*dictionary key_type (24 char string)*/
        53,                             /*hashsize*/
        REGISTRY_VALUE_LENGT_MAX,       /*data_size*/
        0,                              /*local_size (value cache in RAM)*/
        0,                              /*tallie*/
        NVOL3_SECTOR_VERSION            /*version*/
        ) ;
```


This creates a NVOL with two 64K sectors at the start of the FLASH. The total size of the key, including the entry header, is 256 bytes. This is not a requirement, but alignment should be taken into account. The lookup dictionary has a hash size of 53. Furthermore, this instance will not store any values in RAM (```local_size = 0```) so will always read them from FLASH when needed.

Now *_regdef_nvol3_entry* can be used with the NVOL API. The NVOL API is slightly invoved so a simple registry example is provided.

# NVOL String Registry Example

The test example provided can be compiled and run in a GitHub codespace. Simply open a codespace for this repostory and type ```make``` in the terminal that opens. Now you can start the NVOL example by typing ```./build/nvol``` in the terminal. All the commands can be executed in the terminal of the codespace.

When the example program is launched, a command shell will open and it will display the following:
```
@navaro ➜ /workspaces/nvol (main) $ ./build/nvol 
REG   : : resetting _regdef_nvol3_entry
NVOL3 : : '_regdef_nvol3_entry' 0 / 255 records loaded
Navaro corshell Demo v 'Jan 16 2023'

use 'help' or '?' for help.
# >
```

First, we will run a simple script to populate our registry with values.At the prompt type 
```
source ./test/reg.sh
```

This should fill the registry with values. To list all the commands for testing the registry type ``` ? reg```, it should display the following list:

```
# >? reg
reg [key] [value]
regadd <key> <value>
regdel <key>
regerase 
regstats 
# >
```

We can now use the reg command to view and update the registry:

1. list the entire contents of the regitry:
```
# >reg
user.address:           123 Main St.
player.name:            cool_cat
test:                   123
user.occupation:        contributor
player.age:             Ancient
player.points:          9000
user.email:             johnsmith@example.com
user.phone_number:      555-555-5555
user.age:               30
player.status:          awakened
player.species:         Dragon
user.name:              John Smith
player.level:           legendary
user.favorite_color:    blue
player.location:        Mount Olympus
user.gender:            male
player.power:           invisibility
user.marital_status:    single
player.weapon:          lightning bolt
user.children:          0
player.gender:          Otherworldly
```

2. Change the "favorite_color" of the user:
```
# >reg user.favorite_color yellow
OK
```

3. View the updated entry:
```
# >reg user.favorite_color
user.favorite_color:    yellow
```

4. Look at the status of the registry:
```
# >regstats
NVOL3 : : '_regdef_nvol3_entry' 21 / 255 records loaded
record  : 256 recordsize
        : 0x000000 1st sector version 0x0155 flags 0xaaaaffff
        : 0x010000 2nd sector version 0x0000 flags 0xffffffff
        : 0x010000 sector size
        : 21 loaded
        : 21 inuse
        : 1 invalid
        : 0 error
        : 672 lookup table bytes
        : 101 dict hash size (21)
        : dict hash - max 2, empty 82, used 19
```

These commands are all implemented in ```src/registry/registrycmd.c```. The implementation is intuitive and self-explanatory and should requiring no further explanation.

The shell is a project in its own right but it is included in this example for demonstration purposes. It is easy to extend. Use ```?``` to see the complete list of implemented commands for this example.

# Porting

The example provides an emulation of the FLASH in RAM. To port the implementation to use a real FLASH chip the following exported functions need to be implemented for your platform:

```
int32_t spiflash_read (uint32_t address, uint32_t len, uint8_t* data) ;
int32_t spiflash_write (uint32_t address, uint32_t len, const uint8_t* data) ;
int32_t spiflash_sector_erase (uint32_t addr_start, uint32_t addr_end) ;
```


For the demo these functions were implemented in ```src/nvram/nvram.c``` making use of RAM.
