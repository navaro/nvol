

SECTIONS
{
  .corshell BLOCK(4K)   :
  {
		. = ALIGN((32));
		
        __rodata_cmds_base__ = .;
         KEEP(*(.rodata.cmds.*))
        . = ALIGN(4);
         __rodata_cmds_end__ = .;

  
  }
} 
INSERT AFTER .text;

