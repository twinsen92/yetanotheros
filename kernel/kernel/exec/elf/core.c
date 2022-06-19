/* kernel/exec/elf/core.c - ELF executable loader core implementation */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/debug.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/proc.h>
#include <kernel/scheduler.h>
#include <kernel/vfs.h>
#include <kernel/utils.h>
#include <kernel/exec/elf/core.h>

#include <user/yaos2/kernel/defs.h>

static void handle_section_load(struct proc *proc, struct file *f, struct elf_32_section_header *sh)
{
	uint8_t *contents;

	/* Read the segment contents and write them to the reserved process memory. */
	contents = kalloc(HEAP_NORMAL, 1, sh->file_size);
	f->seek_beg(f, sh->file_offset);
	f->read(f, contents, sh->file_size);
	proc_vmwrite(proc, (uvaddr_t)sh->mem_offset, contents, sh->file_size);
	kfree(contents);
}

static void handle_section(struct proc *proc, struct file *f, struct elf_32_section_header *sh)
{
	if (sh->type == ELF_SHT_PROGBITS && sh->file_size > 0 && sh->mem_offset > 0)
	{
		handle_section_load(proc, f, sh);
	}
}

/* TODO: This can be done better... */
void exec_user_elf_program(const char *path, const char *stdin, const char *stdout, const char *stderr)
{
	struct file *f = NULL;
	struct elf_header header;
	struct elf_32_header header_32;
	struct elf_32_program_header program_32;
	struct elf_32_section_header section_32;
	int i;
	struct proc *proc;
	uvaddr_t vbreak = UVNULL, v, vto;
	uvaddr_t stack;
	size_t stack_size;
	struct thread *thread;

	/* Open the file and make sure it is open. */
	f = vfs_open(path);
	kassert(f != NULL);

	/* Read the header of the file. */
	f->read(f, &header, sizeof(header));

	/* TODO: Check if the file is an ELF file and react accordingly. */
	kassert(kmemcmp(header.magic, ELF_MAGIC, sizeof(header.magic)));
	/* TODO: Assume based on some information, and react without a panic. */
	kassert(header.bits == ELF_BITS_32);
	kassert(header.endianness == ELF_LITTLE_ENDIAN);
	kassert(header.iset == ELF_ISET_X86);
	/* TODO: May want to support more than just plain executables... */
	kassert(header.type == ELF_TYPE_EXEC);

	/* Start creating a virtual memory space for the program. */
	/* TODO: Use an actual name. */
	proc = proc_alloc("elf executable");

	/* Read the bits dependent header of the file. */
	f->read(f, &header_32, sizeof(header_32));

	/* Go to ith entry in the program header table and load it into memory. */
	for (i = 0; i < header_32.pht_len; i++)
	{
		f->seek_beg(f, header_32.pht_pos + (i * header_32.phe_size));
		f->read(f, &(program_32), sizeof(program_32));

		/* TODO: More flexibility...? */
		kassert(program_32.type == ELF_SEG_LOAD);

		/* Reserve memory for this segment in process' virtual space. */
		v = (uvaddr_t)mask_to_page(program_32.mem_offset);
		vto = (uvaddr_t)mask_to_page(program_32.mem_offset + program_32.mem_size);

		while (v <= vto)
		{
			proc_vmreserve(proc, v, VM_USER | VM_WRITE | VM_EXEC);
			v += PAGE_SIZE;
		}

		/* Move the program break address. */
		if (vto > vbreak)
			vbreak = vto;

		/* TODO: Actually use the flags field. */
		/* TODO: What about the seg_align field? What is it used for? */
	}

	/* Go to ith entry in the program header table and load it into memory. */
	for (i = 0; i < header_32.sht_len; i++)
	{
		f->seek_beg(f, header_32.sht_pos + (i * header_32.she_size));
		f->read(f, &(section_32), sizeof(section_32));

		handle_section(proc, f, &section_32);
	}

	vfs_close(f);

	/* Allocate a stack for the program right after its executable. */
	proc_vmreserve(proc, vbreak, VM_USER | VM_WRITE);
	stack = vbreak;
	stack_size = PAGE_SIZE;
	vbreak += PAGE_SIZE;

	/* Set the break pointer now that we have everything covered. */
	proc_set_break(proc, vbreak);

	/* Create the main thread. */
	thread = uthread_create(header_32.pe_pos, stack, stack_size, "elf thread", proc);

	/*
	 * Open stdin, stdout and stderr. We don't have to compete with any other thread so we'll
	 * just straight up bind them.
	 */
	proc_lock(proc);

	f = vfs_open(stdin);

	if (f == NULL)
		kpanic("exec_user_elf_program(): could not open stdin");

	proc_bind(proc, STDIN_FILENO, f);

	f = vfs_open(stdout);

	if (f == NULL)
		kpanic("exec_user_elf_program(): could not open stdout");

	proc_bind(proc, STDOUT_FILENO, f);

	f = vfs_open(stderr);

	if (f == NULL)
		kpanic("exec_user_elf_program(): could not open stderr");

	proc_bind(proc, STDERR_FILENO, f);

	proc_unlock(proc);

	/* Schedule the main thread. */
	schedule_proc(proc, thread);
}
