#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

static struct E1000 *base;
struct tx_desc *tx_descs;
#define N_TXDESC (PGSIZE / sizeof(struct tx_desc))

int
e1000_tx_init()
{
	// Allocate one page for descriptors
	struct PageInfo* txPP = page_alloc(ALLOC_ZERO);
	tx_descs = (struct tx_desc *) page2kva(txPP);
	// Initialize all descriptors
	for (int i=0; i<MAX_TX_DESC_NUM; i++){
		tx_descs[i].addr = PADDR(tx_pkt_buffer[i].content);
		tx_descs[i].status |= E1000_TX_STATUS_DD;
	}
	// Set hardward registers
	// Look kern/e1000.h to find useful definations
	e1000->TDBAL = PADDR(tx_descs);
	e1000->TDBAH = 0;
	e1000->TDLEN = PGSIZE;
	e1000->TDH = 0;
	e1000->TDT = 0;
	e1000->TCTL |= E1000_TCTL_EN;
    e1000->TCTL |= E1000_TCTL_PSP;
    e1000->TCTL |= E1000_TCTL_CT_ETHER;
    e1000->TCTL |= E1000_TCTL_COLD_FULL_DUPLEX;
    e1000->TIPG = E1000_TIPG_DEFAULT;
	return 0;
}

struct rx_desc *rx_descs;
#define N_RXDESC (PGSIZE / sizeof(struct rx_desc))

int
e1000_rx_init()
{
	// Allocate one page for descriptors
	struct PageInfo* rxPP = page_alloc(ALLOC_ZERO);
	rx_descs = (struct rx_desc *) page2kva(rxPP);
	// Initialize all descriptors
	// You should allocate some pages as receive buffer
	for (int i=0; i<MAX_RX_DESC_NUM; i++){
		rx_descs[i].addr = PADDR(rx_pkt_buffer[i].content);
	}
	// Set hardward registers
	// Look kern/e1000.h to find useful definations
	e1000->RAL = QEMU_MAC_LOW;
	e1000->RAH = QEMU_MAC_HIGH;
	e1000->RDBAL = PADDR(rx_descs);
	e1000->RDBAH = 0;
	e1000->RDLEN = PGSIZE;
	e1000->RDH = 1;
	e1000->RDT = 0;
	e1000->RCTL |= E1000_RCTL_EN;
    e1000->RCTL |= E1000_RCTL_SECRC;
    e1000->RCTL |= E1000_RCTL_BSIZE_2048;

	return 0;
}

int
pci_e1000_attach(struct pci_func *pcif)
{
	// Enable PCI function
	// Map MMIO region and save the address in 'base;
	pci_func_enable(pcif);
	e1000 = (struct E1000*)mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("E1000 STATUS: %08x\n", e1000->STATUS);
	e1000_tx_init();
	e1000_rx_init();
	return 0;
}

int
e1000_tx(const void *buf, uint32_t len)
{
	// Send 'len' bytes in 'buf' to ethernet
	// Hint: buf is a kernel virtual address
	if (len <= 0 || len > MAX_TX_PKT_LEN) {
		cprintf("length error\n");
		return -1;
	}
	int tail = e1000->TDT;
	if ((tx_descs[tail].status & E1000_TX_STATUS_DD) == 0) {
		cprintf("status error\n");
		return -1;
	}
	memmove(tx_pkt_buffer[tail].content, buf, len);
    tx_descs[tail].length = len;
    tx_descs[tail].status &= ~E1000_TX_STATUS_DD;
    tx_descs[tail].cmd |= E1000_TX_CMD_RS;
    tx_descs[tail].cmd |= E1000_TX_CMD_EOP;
    e1000->TDT = (tail + 1) % MAX_TX_DESC_NUM;
	return 0;
}

int
e1000_rx(void *buf, uint32_t len)
{
	// Copy one received buffer to buf
	// You could return -E_AGAIN if there is no packet
	// Check whether the buf is large enough to hold
	// the packet
	// Do not forget to reset the decscriptor and
	// give it back to hardware by modifying RDT
	int tail = (e1000->RDT + 1) % MAX_RX_DESC_NUM;
	if ((rx_descs[tail].status & E1000_RX_STATUS_DD) == 0) {
		cprintf("status error\n");
		return -E_AGAIN;
	}
	memmove(buf, rx_pkt_buffer[tail].content, len);
    rx_descs[tail].length = len;
    rx_descs[tail].status &= ~E1000_RX_STATUS_DD;
    e1000->RDT = tail;
	return len;
}
