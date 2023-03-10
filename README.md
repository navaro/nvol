# NVOL
## Overview
NVOL is a persistent FLASH registry, often referred to as EEPROM emulation. It offers a basic API for reading and writing indexed key/value pairs to and from a FLASH memory, similar to a registry. It's important to mention that both the key and value are stored in FLASH, allowing for modification of the registry during runtime. Some of the features of NVOL include:


- Internal data management and wear leveling using 2 sectors.
- Configurable sector sizes.
- Reliability with robustness against power failures and asynchronous resets.
- Ensure data integrity with automatic recovery of corrupt sectors or entries during startup.
- Indexed lookup with a built-in hash table.
- Configurable key sizes and types.
- Configurable value sizes.
- Configurable value cache in RAM.
- Efficient and lightweight implementaiton.
- Support transactions.

The library provides two options to manage data storage: Option 1: Immediately save entries to FLASH memory whenever changed. Option 2: Store entries in RAM and save to FLASH memory on demand.

## Background
The disadvantage of a FLASH memory is that it cannot be erased or written in single bytes. FLASH memory can only be erased and written in large blocks. A typical erase
block size may be 4K or 8K bytes. For NVOL, the FLASH implementations should support partial writes, where an erased block may be written multiple times as long as bits only changed from “1” (erased state) to “0” (programmed state).

For the FLASH, the page is considered the smallest block size that can be erased. Each sector can be made up of one or more of these pages.

The first sector is initially erased. New registry entries are added sequentially to the FLASH. When an entry is updated, the old entry is marked as invalid and a new entry is written at the next available FLASH address. Once the first sector reaches capacity, all valid entries are copied to the second sector and the first sector is then erased. This process repeats itself.

NVOL efficiently handles and keeps track of valid entries and their locations on FLASH. The sectors are managed dynamically.

## Implementation

Macros are available to create static instances of NVOL, which can then be used with the API. An example of a declaration would be:
```
#define NVOL3_REGISTRY_START                0
#define NVOL3_REGISTRY_SECTOR_SIZE          STORAGE_32K
#define NVOL3_REGISTRY_SECTOR_COUNT         2

#define REGISTRY_KEY_LENGTH                 24
#define REGISTRY_VALUE_LENGT_MAX            224

NVOL3_INSTANCE_DECL(_regdef_nvol3_entry,
        ramdrv_read, ramdrv_write, ramdrv_erase,
        NVOL3_REGISTRY_START,
        NVOL3_REGISTRY_START + NVOL3_REGISTRY_SECTOR_SIZE,
        NVOL3_REGISTRY_SECTOR_SIZE,
        REGISTRY_KEY_LENGTH,            /*key_size*/
        DICTIONARY_KEYSPEC_BINARY(6),   /*dictionary key_type (24 char string)*/
        53,                             /*hashsize*/
        REGISTRY_VALUE_LENGT_MAX,       /*data_size*/
        0,                              /*local_size (no cache in RAM)*/
        0,                              /*tallie*/
        NVOL3_SECTOR_VERSION            /*version*/
        ) ;

```


This creates a NVOL with two 32K sectors at the start of the FLASH. The total size of the key, including the entry header, is 256 bytes. This is not a requirement, but alignment should be taken into account. The lookup dictionary has a hash size of 53. Furthermore, this instance will not store any values in RAM (```local_size = 0```) so will always read them from FLASH when needed.

In the demo the nvramdrv driver is used that emulation a FLASH memory in RAM, the access functions is ramdrv_read, ramdrv_write and ramdrv_erase configured for this instance.

Now *_regdef_nvol3_entry* can be used with the NVOL API. The NVOL API is slightly invoved so a simple registry example is provided.

# NVOL String Registry Example

The test example provided can be compiled and run in a GitHub codespace. Simply open a codespace for this repostory and type ```make``` in the terminal that opens (if the terminal is not open use ``` ctrl + ` ``` to open the terminal). Now you can start the NVOL example by typing ```./build/nvol``` in the terminal. All the commands can be executed in the terminal of the codespace.

When the example program is launched, a command shell will open and it will display the following:
```
@navaro ➜ /workspaces/nvol (main) $ ./build/nvol 
REG   : : resetting _regdef_nvol3_entry
NVOL3 : : '_regdef_nvol3_entry' 0 / 255 records loaded
Navaro corshell Demo v 'Jan 16 2023'

use 'help' or '?' for help.
# >
```

First, we will run a simple script to populate our registry with values. At the prompt type:
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
regtest [repeat]
# >
```

We can now use the reg command to view and update the registry:

1. list the entire contents of the regitry:
```
# >reg
player.age:             Ancient
player.gender:          Otherworldly
player.level:           legendary
player.location:        Mount Olympus
player.name:            cool_cat
player.points:          9000
player.power:           invisibility
player.species:         Dragon
player.status:          awakened
player.weapon:          lightning bolt
test:                   123
user.address:           123 Main St.
user.age:               30
user.children:          0
user.email:             johnsmith@example.com
user.favorite_color:    blue
user.gender:            male
user.marital_status:    single
user.name:              John Smith
user.occupation:        contributor
user.phone_number:      555-555-5555

    21 entries found.
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

4. Delete an entry:
```
# >regdel user.gender
OK
```

5. Filter only "player" entries:
```
# >reg player
player.age:             Ancient
player.gender:          Otherworldly
player.level:           legendary
player.location:        Mount Olympus
player.name:            cool_cat
player.points:          9000
player.power:           invisibility
player.species:         Dragon
player.status:          awakened
player.weapon:          lightning bolt

    10 entries found.
```

6. Look at the status of the registry:
```
# >regstats
NVOL3 : : '_regdef_nvol3_entry' 20 / 255 records loaded
record  : 256 recordsize
        : 0x000000 1st sector version 0x0155 flags 0xaaaaffff
        : 0x010000 2nd sector version 0x0000 flags 0xffffffff
        : 0x010000 sector size
        : 20 loaded
        : 20 inuse
        : 2 invalid
        : 0 error
        : 640 lookup table bytes
        : 53 dict hash size
        : dict hash - max 2, empty 34, used 19
```

These commands are all implemented in ```src/registry/registrycmd.c```. The implementation is intuitive and self-explanatory and should requiring no further explanation.

The shell is a project in and of itself, but is only included in this example for demonstration purposes. It is easy to extend. Use ```?``` to see the complete list of commands implemented for this example.

# NVOL String Table Example

Where the registry makes use of strings to index the entries, the string table uses integer values. This approach requires less RAM resources.

For this example there is also a script to populate the string table with values. At the prompt type:

```
source ./test/strtab.sh
```

Again, we can now list the string table with a simple command:

```
# >strtab
0000   ENGLISH
0001   The end of the world is our playground.
0002   The company is family. The family is the company.
0003   Escape the mundane. Embrace the extraordinary.
0004   Work hard, live easy.
0005   The future is now. Join us.
0006   There is no I in team. But there is in severance.
0007   We're not just a company. We're a movement.
0008   Success is a journey, not a destination.
0009   Innovation starts with you.
0010   Welcome to the beginning of something great.
0100   DUTCH
0101   Het einde van de wereld is onze speeltuin.
0102   Het bedrijf is familie. De familie is het bedrijf.
0103   Ontsnap aan het alledaagse. Omarm het buitengewone.
0104   Hard werken, makkelijk leven.
0105   De toekomst is nu. Doe met ons mee.
0106   Er is geen ik in team. Maar wel in ontslagvergoeding.
0107   We zijn niet alleen een bedrijf. We zijn een beweging.
0108   Succes is een reis, geen bestemming.
0109   Innovatie begint bij jou.
0110   Welkom bij het begin van iets groots.
0999   test

    23 entries found.
# >
```

Like the registry, the string table example registers itself with the string substitution library to replace index values delimited with < > with the string in the string table. For example:

```
# >echo "<1>"
The end of the world is our playground.
# >
```


# Porting

The demo uses an emulation driver for FLASH in RAM. This is implemented in "crc/drivers/ramdrv.h/c". 

In the same directory a sample driver for a real FLASH chip is proviced in the files "spiflash.h/c". This was implemeted using the ChibiOS/HAL SPI driver and should be a good starting point for porting NVOL to your own platform.
