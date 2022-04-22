/**
 * @file pro.cpp
 * @author Jiri Kristof <xkrist22@stud.fit.vutbr.cz>
 * @brief File contains parallel implementation of algorithm "preorder tree"
 */

#include "pro.h"

using namespace std;

/// edge id counter initialization
int Edge::id_counter = 1;

Edge::Edge(char start_node, char end_node) {
	this->start_node = start_node;
	this->end_node = end_node;
	this->id = Edge::id_counter;
	Edge::id_counter++;
}

int Edge::get_id() {
	return this->id;
}

char Edge::get_start_node() {
	return this->start_node;
}

char Edge::get_end_node() {
	return this->end_node;
}

Adj_elem::Adj_elem(char node_id, int forward_id, int reverse_id, bool inserted_as_forward) {
	this->forward_id = forward_id;
	this->reverse_id = reverse_id;
	this->node_id = node_id;
	this->inserted_as_forward = inserted_as_forward;
}

int Adj_elem::get_forward_id() {
	return this->forward_id;
}

int Adj_elem::get_reverse_id() {
	return this->reverse_id;
}

char Adj_elem::get_node_id() {
	return this->node_id;
}

bool Adj_elem::is_forward_elem() {
	return this->inserted_as_forward;
}

int Nodes::get_preorder_position() {
	return this->preorder_position;
}

char Nodes::get_node_name() {
	return this->node_name;
}

Nodes::Nodes(int preorder_position, char name) {
	this->preorder_position = preorder_position;
	this->node_name = name;
}

int main(int argc, char** argv) {
	// first at all, check, if there is any argument
	if (argc < 2) {
		return 0;
	}
	
	// input tree in form of array
	string node_list = argv[1];
	// variables for storing edge id and
	// weight of each edge
	int edge_id, weight;
	// flag specifying if edge in process
	// is forward or not
	bool forward;
	// variables for storing rank of processes
	// and total number of processes
	int rank, size;

	// edge list stores every created edge
	// used only by main
	edge_list_t edge_list;
	// stores lists of adj elems 
	// (see more in constructor of Adj_elem)
	// create by main, broadcast to every
	// process
	adj_t adj;
	// list will contain name of nodes with its
	// preorder position (used only by main)
	node_list_t preorder_node_list;
	
	// MPI initialilzation
	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
	
	/**** CREATE ADJ LIST AND BROADCAST ****/
	if (rank == PROC_MAIN) {
		// firstly, check, if it is necessary to try to
		// compute preorder sequence
		if (node_list.length() == 1) {
			printf("%c\n", node_list[0]);
			MPI_Finalize();
			return 0;
		}
		// create edges and send them to correct processes
		// NOTE: tree is indexed from 1
		for (int i = 0; i < node_list.length(); i++) {
			// create indexes specifying node childs and
			// number of process, which will recieve edge
			int left_index = (2 * (i + 1)) - 1;
			int right_index = (2 * (i + 1));
			vector<Adj_elem> adj_forward_node;
			vector<Adj_elem> adj_reverse_node_left;
			vector<Adj_elem> adj_reverse_node_right;
			
			if (left_index < node_list.length()) {
				// create 2 edges
				Edge forward = Edge(node_list[i], node_list[left_index]);
				Edge reverse = Edge(node_list[left_index], node_list[i]);
				// add edges to list of edges
				edge_list.push_back(forward);
				edge_list.push_back(reverse);
				// create adj elem
				Adj_elem forward_elem = Adj_elem(node_list[i], forward.get_id(), reverse.get_id(), true);
				Adj_elem reverse_elem = Adj_elem(node_list[left_index], reverse.get_id(), forward.get_id(), false);
				// push adj elem to adj for node
				adj_forward_node.push_back(forward_elem);
				adj_reverse_node_left.push_back(reverse_elem);
			}

			if (right_index < node_list.length()) {
				// create 2 edges
				Edge forward = Edge(node_list[i], node_list[right_index]);
				Edge reverse = Edge(node_list[right_index], node_list[i]);
				// add edges to list of edges
				edge_list.push_back(forward);
				edge_list.push_back(reverse);
				// create adj elem
				Adj_elem forward_elem = Adj_elem(node_list[i], forward.get_id(), reverse.get_id(), true);
				Adj_elem reverse_elem = Adj_elem(node_list[right_index], reverse.get_id(), forward.get_id(), false);
				// push adj elem to adj for node
				adj_forward_node.push_back(forward_elem);
				adj_reverse_node_right.push_back(reverse_elem);
			}

			if (!adj_forward_node.empty()) {
				adj = utility::push_edge_vector(adj_forward_node, adj);
			}

			if (!adj_reverse_node_left.empty()) {
				adj = utility::push_edge_vector(adj_reverse_node_left, adj);
			}

			if (!adj_reverse_node_right.empty()) {
				adj = utility::push_edge_vector(adj_reverse_node_right, adj);
			}
		}

		// send id of edges to processes (not to process 0, which is main)
		for (int i = 0; i < edge_list.size(); i++) {
			int edge_id = edge_list[i].get_id();
			MPI_Send(&edge_id,1,MPI_INT,edge_id,EDGE_ID,MPI_COMM_WORLD);
		}

		// broadcast number of lists for each node
		int node_list_num = adj.size();
		for (int i = 1; i < size; i++) {
			MPI_Send(&node_list_num,1,MPI_INT,i,NODE_LIST_NUM,MPI_COMM_WORLD);
		}

		// iterate over lists for each node
		//adj_t adj;
		for (int i = 0; i < node_list_num; i++) {
			// broadcast number of adj elems
			int adj_elem_num = adj[i].size();
			for (int j = 1; j < size; j++) {
				MPI_Send(&adj_elem_num,1,MPI_INT,j,NODE_ADJ_NUM,MPI_COMM_WORLD);
			}
			// iterate over adj elems
			for (int j = 0; j < adj_elem_num; j++) {
				// broadcast adj elems (aka forward and reverse)
				for (int k = 1; k < size; k++) {
					char node_id = adj[i][j].get_node_id();
					int forward_id = adj[i][j].get_forward_id();
					int reverse_id = adj[i][j].get_reverse_id();
					int inserted_as_forward = adj[i][j].is_forward_elem();
					MPI_Send(&node_id,1,MPI_CHAR,k,ADJ_NODE,MPI_COMM_WORLD);
					MPI_Send(&forward_id,1,MPI_INT,k,ADJ_FORWARD,MPI_COMM_WORLD);
					MPI_Send(&reverse_id,1,MPI_INT,k,ADJ_REVERSE,MPI_COMM_WORLD);
					MPI_Send(&inserted_as_forward,1,MPI_C_BOOL,k,ADJ_BOOL,MPI_COMM_WORLD);
				}
			}
		}
	} else {
		// recieve id of edge
		MPI_Recv(&edge_id,1,MPI_INT,PROC_MAIN,EDGE_ID,MPI_COMM_WORLD,&status);
		// recieve number of lists in adj
		int node_list_num;
		MPI_Recv(&node_list_num,1,MPI_INT,PROC_MAIN,NODE_LIST_NUM,MPI_COMM_WORLD,&status);

		// iterate over recieving lists for each node
		for (int i = 0; i < node_list_num; i++) {
			vector<Adj_elem> adj_for_node;

			// recieve number of adj elems
			int adj_elem_num;
			MPI_Recv(&adj_elem_num,1,MPI_INT,PROC_MAIN,NODE_ADJ_NUM,MPI_COMM_WORLD,&status);

			// iterate over adj elems
			for (int j = 0; j < adj_elem_num; j++) {
				// recieve adj elems (aka forward and reverse)
				char node_id;
				int forward_id;
				int reverse_id;
				bool inserted_as_forward;
				MPI_Recv(&node_id,1,MPI_CHAR,PROC_MAIN,ADJ_NODE,MPI_COMM_WORLD,&status);
				MPI_Recv(&forward_id,1,MPI_INT,PROC_MAIN,ADJ_FORWARD,MPI_COMM_WORLD,&status);
				MPI_Recv(&reverse_id,1,MPI_INT,PROC_MAIN,ADJ_REVERSE,MPI_COMM_WORLD,&status);
				MPI_Recv(&inserted_as_forward,1,MPI_C_BOOL,PROC_MAIN,ADJ_BOOL,MPI_COMM_WORLD,&status);
				Adj_elem adj_elem = Adj_elem(node_id, forward_id, reverse_id, inserted_as_forward);
				adj_for_node.push_back(adj_elem);
			}
			adj = utility::push_edge_vector(adj_for_node, adj);
		}
	}

	/**** EULER TOUR ****/
	// count euler tour only by non-main edges
	int euler_next;
	vector<int> euler_tour;
	if (rank != PROC_MAIN) {
		// count id of next edge in euler tour
		euler_next = utility::euler_tour(edge_id, adj);
		// send next edge id to main process
		MPI_Send(&euler_next,1,MPI_INT,PROC_MAIN,EULER_TOUR,MPI_COMM_WORLD);
	} else {
		// recieve euler next edge from every process and create euler tour list
		for (int i = 1; i < size; i++) {
			MPI_Recv(&euler_next,1,MPI_INT,i,EULER_TOUR,MPI_COMM_WORLD,&status);
			euler_tour.push_back(euler_next);
		}
		// fix euler tour (last edge to root pointing to itself)
		euler_tour = utility::fix_euler_tour(euler_tour, edge_list, node_list[0]);
	}
	if (rank == PROC_MAIN) {
		// send edges to processes in order of euler tour
		for (int i = 0; i < euler_tour.size(); i++) {
			MPI_Send(&euler_tour[i],1,MPI_INT,i+1,EULER_TOUR_FIXED,MPI_COMM_WORLD);
		}
	} else {
		// recieve next edge in euler tour 
		// note that only for 1 process there is change
		MPI_Recv(&euler_next,1,MPI_INT,PROC_MAIN,EULER_TOUR_FIXED,MPI_COMM_WORLD,&status);
	}

	/**** SET WEIGHTS ****/
	// weight is defined only for non-main processes (holding edges)
	// also, save forward info for later, it is used in counting the
	// preorder node position and it should be more effective to save it
	// than to search for this property
	if (rank != PROC_MAIN) {
		if (utility::is_forward(edge_id, adj)) {
			weight = 1;
			forward = true;
		} else {
			weight = 0;
			forward = false;
		}
	}

	/**** SUM OF SUFFIX ****/
	if (rank != PROC_MAIN) {
		// defaultly, only first process is sleeping (no process will notice first process)
		bool sleeping = false || edge_id == 1;
		bool ending_edge = euler_next == edge_id;
		// last process must be able to answer more than 1 noticing
		// number of notice messages is counted as:
		// ending_edge_noticing_sum = min(((ending_edge_noticing_sum * 2) - 1), size - 1)
		int ending_edge_noticing_sum = STARTING_ENDING_EDGE_NOTICING_SUM;

		// set neutral element
		if (ending_edge) {
			weight = 0;
		}

		for (int i = 0; i <= ceil(log((double) size)); i++) {
			// notice process defined by euler_next that actual process
			// wants his value of weight and euler_next
			// note that ending process should not perform noticing
			MPI_Send(&rank,1,MPI_INT,euler_next,VALUES_WANTED,MPI_COMM_WORLD);
			// noticed can be only non-sleeping processes
			// first process cannot be noticed (it has no predcessor)
			// ending edge must handle noticing from more than 1 process
			if (!sleeping && !ending_edge) {
				// recieve info fromm process that it wanted 
				int notice_process_id;
				MPI_Recv(&notice_process_id,1,MPI_INT,MPI_ANY_SOURCE,VALUES_WANTED,MPI_COMM_WORLD,&status);
				// send wanted info
				MPI_Send(&weight,1,MPI_INT,notice_process_id,VALUES_WEIGHT,MPI_COMM_WORLD);
				MPI_Send(&euler_next,1,MPI_INT,notice_process_id,VALUES_NEXT_EDGE,MPI_COMM_WORLD);

			}
			// handling noticing of last process
			if (!sleeping && ending_edge) {
				for (int j = 0; j < ending_edge_noticing_sum; j++) {
					// recieve info fromm process that it wanted 
					int notice_process_id;
					MPI_Recv(&notice_process_id,1,MPI_INT,MPI_ANY_SOURCE,VALUES_WANTED,MPI_COMM_WORLD,&status);
					// send wanted info
					MPI_Send(&weight,1,MPI_INT,notice_process_id,VALUES_WEIGHT,MPI_COMM_WORLD);
					MPI_Send(&euler_next,1,MPI_INT,notice_process_id,VALUES_NEXT_EDGE,MPI_COMM_WORLD);
				}
				ending_edge_noticing_sum = utility::update_ending_edge_noticing_sum(ending_edge_noticing_sum, size);
			}

			// recieve new values from euler_next process
			int recieved_weight;
			int recieved_euler_next;
			MPI_Recv(&recieved_euler_next,1,MPI_INT,euler_next,VALUES_NEXT_EDGE,MPI_COMM_WORLD,&status);
			MPI_Recv(&recieved_weight,1,MPI_INT,euler_next,VALUES_WEIGHT,MPI_COMM_WORLD,&status);
			
			// send to main process new successor - main will inform that successor process, that it
			// cannot go to sleep, because there will be at least one process (calling process), 
			// which will send noticing to the successor process
			bool recieved_sleeping;
			MPI_Send(&recieved_euler_next,1,MPI_INT,PROC_MAIN,RECIEVED_EULER_NEXT,MPI_COMM_WORLD);
			MPI_Recv(&recieved_sleeping,1,MPI_C_BOOL,PROC_MAIN,VALUES_SLEEPING,MPI_COMM_WORLD,&status);

			/**** BARRIER ****/
			// all process must wait until every process has send and recieved new values
			// to handle RAW conflict, this is implemented as waiting to info about sleeping,
			// because main process sends this info after every process sends to main its 
			// next edge id, else main process waits and so waits other processes

			// update weight, identificator of next edge (and process) and if process should be sleeping
			weight = weight + recieved_weight;
			euler_next = recieved_euler_next;
			sleeping = recieved_sleeping;
		}
		// note, that correction is not needed due to behaviour of suffix sum algorithm for adding

	} else {
		// main process must answer in same duration as other processes sends data
		for (int i = 0; i <= ceil(log((double) size)); i++) {
			// accept from each process info about new successor and saves those references
			vector<int> next_active_processes;
			int recieved_id;
			for (int i = 1; i < size; i++) {
				MPI_Recv(&recieved_id,1,MPI_INT,i,RECIEVED_EULER_NEXT,MPI_COMM_WORLD,&status);
				next_active_processes.push_back(recieved_id);
			}
			// after recieving new successors references, send to processes, which id is in
			// net_active_processes, stay alive info, to others send go to sleep info
			bool stay_alive = false;
			bool go_to_sleep = true;
			// iterate over every processor id (except PROC_MAIN)
			for (int i = 1; i < size; i++) {
				if (find(next_active_processes.begin(), next_active_processes.end(), i) != next_active_processes.end()) {
					// if reference of id of process is in next_active_processes, then this process should be awake
					MPI_Send(&stay_alive,1,MPI_C_BOOL,i,VALUES_SLEEPING,MPI_COMM_WORLD);
				} else {
					// if nobody will notice processor od id == i, then it can go sleep
					MPI_Send(&go_to_sleep,1,MPI_C_BOOL,i,VALUES_SLEEPING,MPI_COMM_WORLD);
				}
			} 
		}
	}

	/**** PREORDER ****/
	// preorder position is counted only by non-main processes
	int preorder_position;
	if (rank != PROC_MAIN) {
		// preorder position is counted only for forward edges
		if (forward) {
			preorder_position = utility::preorder(weight, node_list.length());
			MPI_Send(&preorder_position,1,MPI_INT,PROC_MAIN,PREORDER,MPI_COMM_WORLD);
		}
	} else {
		// recieve edges
		for (int i = 0; i < node_list.length() - 1; i++) {
			int preorder_position;
			int edge_id;
			// recieve id of process (and edge in same time)
			MPI_Recv(&preorder_position,1,MPI_INT,MPI_ANY_SOURCE,PREORDER,MPI_COMM_WORLD,&status);
			edge_id = status.MPI_SOURCE;
			// search in list of edges for edge specified by given id (of processor == i)
			for (int j = 0; j < edge_list.size(); j++) {
				if (edge_list[j].get_id() == edge_id) {
					Nodes node = Nodes(preorder_position, edge_list[j].get_end_node());
					preorder_node_list.push_back(node);
				}
			}
		}
	}

	/**** PRINT RESULT ****/
	if (rank == PROC_MAIN) {
		// firstly, print root in position 1
		printf("%c", node_list[0]);
		for (int i = 1; i <= node_list.length(); i++) {
			for (int j = 0; j < preorder_node_list.size(); j++) {
				if (preorder_node_list[j].get_preorder_position() == i) {
					printf("%c", preorder_node_list[j].get_node_name());
					break;
				}
			}
		}
		cout << endl;
	}

	MPI_Finalize();
}
