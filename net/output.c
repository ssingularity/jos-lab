#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet request (using ipc_recv)
	//	- send the packet to the device driver (using sys_net_send)
	//	do the above things in a loop
	while (true)
	{
		int r = sys_ipc_recv(&nsipcbuf);
		if (r < 0) return;
		while (sys_net_send((void *)nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len));
	}
	
}
