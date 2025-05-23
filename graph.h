#pragma once

#include <assert.h>
#include "gluethread/glthread.h"
#include "net.h"

#define NODE_NAME_SIZE 		16
#define IF_NAME_SIZE 		16
#define MAX_INTF_PER_NODE 	10

typedef struct node_ node_t;
typedef struct link_ link_t;

typedef struct interface_ {

	char if_name[IF_NAME_SIZE];
	struct node_ *att_node; // owner node
	struct link_ *link;
	intf_nw_prop_t intf_nw_props;
} interface_t;

struct link_ {

	interface_t intf1;
	interface_t intf2;
	unsigned int cost;
};

struct node_ {

	char node_name[NODE_NAME_SIZE];
	interface_t *intf[MAX_INTF_PER_NODE];
	glthread_t graph_glue;
	unsigned int udp_port_number;
	int udp_sock_fd;
	node_nw_prop_t node_nw_props;
};

GLTHREAD_TO_STRUCT(graph_glue_to_node, node_t, graph_glue);

typedef struct graph_ {

	char topology_name[32];
	glthread_t node_list;
} graph_t;

static inline node_t*
get_nbr_node(interface_t *interface){

	assert(interface->att_node);
	assert(interface->link);

	link_t *link = interface->link;

	if(&link->intf1 == interface)
		return link->intf2.att_node;
	else
		return link->intf1.att_node;
}

static inline int
get_node_intf_available_slot(node_t *node){

	int index = 0;
	for(index = 0; index < MAX_INTF_PER_NODE; ++index)
	{
		if(node->intf[index])
			continue;

		return index;
	}

	return -1;
}

graph_t*
create_new_graph(char *topology_name);

node_t* 
create_graph_node(graph_t *graph, char *node_name);

void
insert_link_between_two_nodes(node_t *node1,
							  node_t *node2,
							  char *from_if_name,
							  char *to_if_name,
							  unsigned int cost);

static inline interface_t *
get_node_if_by_name(node_t *node, char *if_name){

	interface_t *interface = NULL;
	int index = 0;

	for(index; index < MAX_INTF_PER_NODE; ++index){
		
		interface = node->intf[index];

		if(interface == NULL) return NULL;

		if(strncmp(if_name, interface->if_name, IF_NAME_SIZE) == 0)
			return interface;
	}

	return NULL;

}

static inline node_t *
get_node_by_node_name(graph_t *topo, char *node_name){

	node_t *node = NULL;
	glthread_t *curr = NULL;

	ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr){

		node = graph_glue_to_node(curr);
		if(strncmp(node->node_name, node_name, strlen(node_name)) == 0)
			return node;

	} ITERATE_GLTHREAD_END(&topo->node_list, curr);

	return NULL;
}
