#include "../graph.h"
#include "../net.h"
#include "layer2.h"

/*----------------
 *   Mac Table   *
 ----------------*/

typedef struct mac_table_entry_{

	mac_add_t mac;
	char oif_name [IF_NAME_SIZE];
	glthread_t mac_entry_glue;
} mac_table_entry_t;
GLTHREAD_TO_STRUCT(mac_glue_to_mac_entry, mac_table_entry_t, mac_entry_glue);

typedef struct mac_table_{

	glthread_t mac_entries;
} mac_table_t;

void
init_mac_table(mac_table_t **mac_table){

	*mac_table = (mac_table_t *)(calloc(1,sizeof(mac_table_t)));
	init_glthread(&((*mac_table)->mac_entries));
}

mac_table_entry_t *
mac_table_lookup(mac_table_t *mac_table, char *mac){
	
	glthread_t *curr;
	mac_table_entry_t *mac_entry;

	ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries, curr){

		mac_entry = mac_glue_to_mac_entry(curr);
		if(mac_entry && strncmp(mac_entry->mac.mac, mac, sizeof(mac_add_t)) == 0)
			return mac_entry;
	} ITERATE_GLTHREAD_END(&mac_table->mac_entries, curr);

	return NULL;
}

void
clear_mac_table(mac_table_t *mac_table){

	glthread_t *curr;
	mac_table_entry_t *mac_entry;

	ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries, curr) {

		mac_entry = mac_glue_to_mac_entry(curr);
		remove_glthread(curr);
		free(mac_entry);

	} ITERATE_GLTHREAD_END(&mac_table->mac_entries, curr);
}

void
delete_mac_table_entry(mac_table_t *mac_table, char *mac){
	
	mac_table_entry_t *mac_table_entry;
	mac_table_entry = mac_table_lookup(mac_table, mac);
	if(!mac_table_entry)
		return;
	remove_glthread(&mac_table_entry->mac_entry_glue);
	free(mac_table_entry);

}

#define IS_MAC_TABLE_ENTRY_EQUAL(mac_entry_1, mac_entry_2)	\
	(strncmp(mac_entry_1->mac.mac, mac_entry_2->mac.mac, sizeof(mac_add_t)) == 0 &&	\
	strncmp(mac_entry_1->oif_name, mac_entry_2->oif_name, IF_NAME_SIZE) == 0)

bool_t
mac_table_entry_add(mac_table_t *mac_table,
		mac_table_entry_t *mac_table_entry){
	
	mac_table_entry_t *old_mac_table_entry = mac_table_lookup(mac_table, 
			mac_table_entry->mac.mac);

	if(old_mac_table_entry &&
			IS_MAC_TABLE_ENTRY_EQUAL(old_mac_table_entry, mac_table_entry))
		return FALSE;

	if(old_mac_table_entry){
		delete_mac_table_entry(mac_table, old_mac_table_entry->mac.mac);
	}

	init_glthread(&mac_table_entry->mac_entry_glue);
	glthread_add_next(&mac_table->mac_entries, &mac_table_entry->mac_entry_glue);
	return TRUE;
}

void
dump_mac_table(mac_table_t *mac_table){

	glthread_t *curr;
	mac_table_entry_t *mac_table_entry;

	ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries, curr){

		mac_table_entry = mac_glue_to_mac_entry(curr);
		printf("\tMAC : %u:%u:%u:%u:%u:%u | Interface : %s\n",
				mac_table_entry->mac.mac[0],
				mac_table_entry->mac.mac[1],
				mac_table_entry->mac.mac[2],
				mac_table_entry->mac.mac[3],
				mac_table_entry->mac.mac[4],
				mac_table_entry->mac.mac[5],
				mac_table_entry->oif_name);
	} ITERATE_GLTHREAD_END(&mac_table->mac_entries, curr);
}

/*-------------------------
 *   L2 Switch Function   *
 -------------------------*/

static void
l2_switch_perform_mac_learning(node_t *node, char *src_mac, char *if_name){
	
	mac_table_entry_t *mac_table_entry;

	mac_table_entry = calloc(1, sizeof(mac_table_entry_t));
	memcpy(mac_table_entry->mac.mac, src_mac, sizeof(mac_add_t));
	strncpy(mac_table_entry->oif_name, if_name, IF_NAME_SIZE);
	mac_table_entry->oif_name[IF_NAME_SIZE - 1] = '\0';

	if(FALSE == mac_table_entry_add(NODE_MAC_TABLE(node), mac_table_entry))
		free(mac_table_entry);
}

static void
l2_switch_forward_frame(node_t *node, interface_t *recv_intf,
		char *pkt, unsigned int pkt_size){

	/*이더넷헤더의 목적지 주소가 Broadcast인경우*/
	ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
	if(IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac)){
		send_pkt_flood_l2_intf_only(node, recv_intf, pkt, pkt_size);
		return;
	}

	mac_table_entry_t *mac_table_entry = 
		mac_table_lookup(NODE_MAC_TABLE(node), ethernet_hdr->dst_mac.mac);

	if(!mac_table_entry) {

		send_pkt_flood_l2_intf_only(node, recv_intf, pkt, pkt_size);
		return;
	}

	char *oif_name = mac_table_entry->oif_name;
	interface_t *oif = get_node_if_by_name(node, oif_name);
	if(!oif){
		return;
	}
	send_pkt_out(pkt, pkt_size, oif);
}

void
l2_switch_recv_frame(interface_t *interface,
		char *pkt, unsigned int pkt_size) {

	node_t *node = interface->att_node;

	ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;

	char *dst_mac = (char *)ethernet_hdr->dst_mac.mac;
	char *src_mac = (char *)ethernet_hdr->src_mac.mac;

	l2_switch_perform_mac_learning(node, src_mac, interface->if_name);
	l2_switch_forward_frame(node, interface, pkt, pkt_size);
}
