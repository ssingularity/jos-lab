// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

extern void _pgfault_upcall(void);
extern volatile pte_t uvpt[];     // VA of "virtual page table"
extern volatile pde_t uvpd[];     // VA of current page directory

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);
		if((FEC_WR & err) && (uvpt[(uintptr_t)addr / PGSIZE] & PTE_COW)){
		sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W);
		memmove(PFTEMP, addr, PGSIZE);
		sys_page_map(0, PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W);
		sys_page_unmap(0, PFTEMP);
	}else{
		panic("page fault at %x !", (uintptr_t)addr);
	}

	panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	void * addr = (void *) (pn * PGSIZE);
	//writable or copy-on-write
	if (uvpt[pn] & (PTE_W | PTE_COW)){
		sys_page_map(0, addr, envid, addr, PTE_U|PTE_P|PTE_COW);
		sys_page_map(0, addr, 0, addr, PTE_U|PTE_P|PTE_COW);
	}
	else {
		sys_page_map(0, addr, envid, addr, PTE_U|PTE_P);
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();
	if (envid == 0){
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	else {
		for (uintptr_t addr =  UTEXT; addr < USTACKTOP; addr += PGSIZE){
			duppage(envid, PGNUM(addr));
		}
		sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_U|PTE_W|PTE_P);
		sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
		sys_env_set_status(envid, ENV_RUNNABLE);
		return envid;
	}
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
