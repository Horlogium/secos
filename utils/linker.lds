/* GPLv2 (c) Airbus */
OUTPUT_FORMAT("elf32-i386","elf32-i386","elf32-i386");
OUTPUT_ARCH("i386")

ENTRY(entry)

PHDRS
{
   phboot  PT_LOAD FLAGS (7);
   phstack PT_LOAD FLAGS (6);
   phsetup PT_LOAD FLAGS (7);
}

SECTIONS
{
   . = 0x300000;
   .mbh      : { KEEP(*(.mbh)) . = ALIGN(4);     } : phboot
   .stack    : { KEEP(*(.stack))                 } : phstack

   __kernel_start__ = .;

   .idt_jmp  : { KEEP(*(.idt_jmp))               } : phsetup
   .text     : { *(.text)                        } : phsetup
   .rodata   : { *(.rodata)                      } : phsetup
   .data     : { *(.data)                        } : phsetup
   .bss      : { *(.bss COMMON)                  } : phsetup
   /DISCARD/ : { *(.note* .indent .comment)      } : phsetup

    __kernel_end__ = .;

   . = 0x400000;
   .user1    : { KEEP(*(.user1)) . = ALIGN(4);   } : phsetup

   . = 0x401000;
   .usr1_cpt : { KEEP(*(.usr1_cpt)) . = ALIGN(4);   } : phsetup

   . = 0x405000;
   .user2    : { KEEP(*(.user2)) . = ALIGN(4);   } : phsetup

   . = 0x405100;
   .usr2_sys : { KEEP(*(.usr2_sys)) . = ALIGN(4);   } : phsetup

   . = 0x406000;
   .usr2_cpt : { KEEP(*(.usr2_cpt)) . = ALIGN(4);   } : phsetup
}
