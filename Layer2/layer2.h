#pragma once
#include "../gluethread/glthread.h"
#include "../comm.h"
#include "../utils.h"
#include "../tcpconst.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack (push, 1)
/*--------------------
 *    ARP Header     *
 --------------------*/
typedef struct arp_hdr_ {

	short hw_type;			/* 이더넷 케이블*/
	short proto_type;		/* 0x800 : IPV4 */
	char hw_addr_len;		/* 6byte MAC */
	char proto_addr_len;	/* 4byte IPV4 */
	short op_code;			/* 1:req 2:reply*/
	mac_add_t src_mac;		/* MAC address */
	unsigned int src_ip;
	mac_add_t dst_mac;
	unsigned int dst_ip;
} arp_hdr_t;

/*-------------------
 *  Ethernet Header *
 -------------------*/
typedef struct ethernet_hdr_ {

	mac_add_t dst_mac;
	mac_add_t src_mac;
	unsigned short type;
	char payload[248];  /*가변길이라고 가정(최대 1500byte)*/
	unsigned int FCS;
} ethernet_hdr_t;
#pragma pack (pop)

#define ETH_HDR_SIZE_EXCL_PAYLOAD 	\
	(sizeof(ethernet_hdr_t) - sizeof(((ethernet_hdr_t*)0)->payload))

#define ETH_FCS(eth_hdr_ptr, payload_size)	\
	(*(unsigned int *)(((char *)(((ethernet_hdr_t *)eth_hdr_ptr)->payload) + payload_size)))	

#define GET_ETHERNET_HDR_PAYLOAD(eth_hdr_ptr)	\
	(char *)((eth_hdr_ptr)->payload)

static inline ethernet_hdr_t *
ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){

	char* temp;
	temp = (char *)calloc(1, pkt_size);
	memcpy(temp, pkt, pkt_size);

	ethernet_hdr_t *eth_hdr = (ethernet_hdr_t *)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
	memset((char *)eth_hdr, 0, ETH_HDR_SIZE_EXCL_PAYLOAD);
	memcpy(eth_hdr->payload, temp, pkt_size + ETH_HDR_SIZE_EXCL_PAYLOAD);

	free(temp);

	return eth_hdr;
}

static inline bool_t
l2_frame_recv_qualify_on_interface(interface_t *interface,
		ethernet_hdr_t *ethernet_hdr){

	if(interface->intf_nw_props.is_ipadd_config == FALSE)
		return TRUE;

	if(memcmp(IF_MAC(interface),
				ethernet_hdr->dst_mac.mac,
				sizeof(mac_add_t)) == 0){
	
		return TRUE;
	}

	if(IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac))
		return TRUE;

}

/*-------------------
 *  ARP Table APIs  *
 -------------------*/
typedef struct arp_table_{

	glthread_t arp_entries;
} arp_table_t;

typedef struct arp_entry_{

	ip_add_t ip_addr;
	mac_add_t mac_addr;
	char oif_name[IF_NAME_SIZE];
	glthread_t arp_glue;
} arp_entry_t;
GLTHREAD_TO_STRUCT(arp_glue_to_arp_entry, arp_entry_t, arp_glue);


void
init_arp_table(arp_table_t **arp_table);

bool_t
arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry);

arp_entry_t *
arp_table_lookup(arp_table_t *arp_table, char *ip_addr);

void
arp_table_update_from_arp_reply(arp_table_t *arp_table, arp_hdr_t *arp_hdr, interface_t *iif);

void
delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr);

void
dump_arp_table(arp_table_t *arp_table);

/*------------------
 *   Routine ARP   *
 ------------------*/
void
send_arp_broadcast_request(node_t *node,
		interface_t *oif,
		char *ip_addr);

void
node_set_intf_l2_mode(node_t *node, char *intf_name,
		intf_l2_mode_t intf_l2_mode);
