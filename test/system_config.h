

#ifndef CFG_PORT_POSIX
#define CFG_PORT_POSIX						1
#endif

#ifndef CFG_STRSUB_USE
#define CFG_STRSUB_USE						1
#endif

#ifndef CFG_REGISTRY_USE
#define CFG_REGISTRY_USE					1
#endif

#ifndef CFG_STRTAB_USE
#define CFG_STRTAB_USE                                          1
#endif

#ifndef CFG_PLATFORM_SVC_SERVICES
#define CFG_PLATFORM_SVC_SERVICES			0
#endif


#define STORAGE_128K						(128*1024)
#define STORAGE_96K							(96*1024)
#define STORAGE_64K							(64*1024)
#define STORAGE_32K							(32*1024)
#define STORAGE_16K							(16*1024)
#define STORAGE_8K							(8*1024)
#define STORAGE_4K							(4*1024)



#define NVOL3_REGISTRY_START				0
#define NVOL3_REGISTRY_SECTOR_SIZE			STORAGE_32K
#define NVOL3_REGISTRY_SECTOR_COUNT			2

#define NVOL3_STRTAB_START                              (NVOL3_REGISTRY_START + NVOL3_REGISTRY_SECTOR_SIZE*NVOL3_REGISTRY_SECTOR_COUNT)
#define NVOL3_STRTAB_SECTOR_SIZE                        STORAGE_32K
#define NVOL3_STRTAB_SECTOR_COUNT                       2

#define PLATFORM_SECTION_NOINIT						__attribute__ ((section (".noinit")))
