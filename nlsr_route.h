#ifndef _NLSR_ROUTE_H_
#define _NLSR_ROUTE_H_

#define EMPTY_PARENT -12345
#define INF_DISTANCE 2147483647

#define NO_NEXT_HOP -4321

struct map_entry
{
	char *router;
	int mapping;
};


struct routing_table_entry
{
	char *dest_router;
	int next_hop_face;
}
;

int route_calculate(struct ccn_schedule *sched, void *clienth, struct ccn_scheduled_event *ev, int flags);
void make_map(void);
void add_map_entry(char *router);
void add_adj_data_to_map(char *orig_router, char *body, int no_link);
void print_map(void);
void assign_mapping_number(void);
void make_adj_matrix(int **adj_matrix,int map_element);
void init_adj_matrix(int **adj_matrix,int map_element);
void print_adj_matrix(int **adj_matrix, int map_element);
int get_mapping_no(char *router);
void calculate_path(int **adj_matrix, long int *parent, long int V, long int S);
void sort_queue_by_distance(long int *Q,long int *dist,long int start,long int element);
int is_not_explored(long int *Q, long int u,long int start, long int element);
void print_path(long int *parent, long int dest);
void print_all_path_from_source(long int *parent,long int source);

/* Routing Table Relates function */

int get_next_hop(char *dest_router);
void add_next_hop_router(char *dest_router);
void add_next_hop_from_lsa_adj_body(char *body, int no_link);
void print_routing_table(void);
void do_old_routing_table_updates();

#endif
