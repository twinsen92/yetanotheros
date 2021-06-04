ENTRY(_start)

SECTIONS
{
	/* First marker, specifies where the kernel lies in virtual memory space. */
	__kernel_virtual_offset = 0xC0000000;
	. = __kernel_virtual_offset;
	__kernel_low_begin = . ;


	/* The kernel will at 1MB in physical memory. */
	__kernel_physical_start = 1M;
	. = __kernel_virtual_offset + __kernel_physical_start;
	__kernel_low_end = . ;
	__kernel_mem_ro_begin = . ;

	.text : AT(ADDR(.text) - __kernel_virtual_offset)
	{
		/* First put the multiboot header, as it is required to be put very early in the image or
		   the bootloader won't recognize the file format. */
		*(.multiboot)

		/* multiboot.S maps only the first 4 MiB of memory so we want the entire boot stage to be
		   there. */
		arch/i386/boot/*.o

		/* Keep track where the boot objects end so that we can verify that they're mapped during
		   boot. */
		__kernel_boot_end = . ;

		/* Test shift to make sure the boot part works. TODO: Remove this shift. */
		/* . = . + 4M; */

		/* Put the AP entry code and data at a known position in memory. We will move it to low
		   memory later, before starting other CPUs. */
		. = ALIGN(4K);

		__kernel_ap_entry_begin = . ;

		arch/i386/cpu/ap_entry.o

		. = ALIGN(4K);
		__kernel_ap_entry_end = . ;

		/* Now we can continue as usual. */
		*(.text)
		*(.eh_frame)
		*(.ctors)
		*(.dtors)
	}

	. = ALIGN(4K);

	/* Read-only data. */
	.rodata : AT(ADDR(.rodata) - __kernel_virtual_offset)
	{
		*(.got)
		*(.got.*)
		*(.rodata)
		*(.rodata.*)
	}

	__kernel_mem_ro_end = ALIGN(4K) ;

	. = ALIGN(4K);
	__kernel_mem_rw_begin = . ;

	/* Read-write data (initialized) */
	.data : AT(ADDR(.data) - __kernel_virtual_offset)
	{
		*(.data)
	}

	. = ALIGN(4K);

	/* Read-write data (uninitialized) and stack */
	__bss_begin = . ;
	.bss : AT(ADDR(.bss) - __kernel_virtual_offset)
	{
		*(COMMON)
		*(.bss)

		/* Reserve memory for kernel's paging structures. This wastes 4 KiB + 4 MiB of physical
		   memory. TODO: Do something smarter. */
		. = ALIGN(4K);
		__kernel_pd = . ;
		. = . + 4K;
		__kernel_page_tables = . ;
		. = . + 4M;
	}

	__kernel_mem_rw_end = ALIGN(4K) ;

	/* Break pointer. Dynamically allocated memory starts here. Virtual memory break is aligned to
	   4 MiBs, because it makes it easier to fit dynamic memory into one page table. */
	__kernel_mem_break = ALIGN(4M) ;
}
