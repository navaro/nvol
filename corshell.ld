

SECTIONS
{
  .corshell BLOCK(4K)   :
  {
		. = ALIGN((32));
		
        __corshell_cmds_base__ = .;
        KEEP(*(SORT(.corshell.cmds.*)))
        . = ALIGN(4);
         __corshell_cmds_end__ = .;

  
  }
} 
INSERT AFTER .text;

