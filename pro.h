/**
 * @file pro.h
 * @author Jiri Kristof, <xkrist22@stud.fit.vutbr.cz>
 * @brief header file of project "preorder tree"
 */

#ifndef PRO_H
#define PRO_H

#include <stdio.h>
#include <mpi.h>
#include <string>
#include <vector>
#include <utility>
#include <math.h>
#include <algorithm>

// rank of main process
#define PROC_MAIN 0

// mpi tags
#define EDGE_ID 0
#define NODE_LIST_NUM 1
#define NODE_ADJ_NUM 2
#define ADJ_FORWARD 3
#define ADJ_REVERSE 4
#define ADJ_NODE 5
#define EULER_TOUR 6
#define ADJ_BOOL 7
#define EULER_TOUR_FIXED 8
#define VALUES_WANTED 9
#define VALUES_WEIGHT 10
#define VALUES_NEXT_EDGE 11
#define RECIEVED_EULER_NEXT 12
#define VALUES_SLEEPING 13
#define PREORDER 14

#define STARTING_ENDING_EDGE_NOTICING_SUM 2

using namespace std;

/**
 * Class instances encapsulates data of one edge
 *
 * each edge is represented by 2 chars (start and end node
 * of edge) and generated id
 */
class Edge {
	private:
		int id;
		char start_node;
		char end_node;
		static int id_counter;
		int preorder_position;
	public:
		/**
		 * Getter of edge id
		 * @return id of edge object
		 */
		int get_id();

		/**
		 * Getter of start node
		 * @return name (char) of node from which edge is starting
		 */
		char get_start_node();

		/**
		 * Getter of end node
		 * @return name (char) of node in which edge is ending
		 */
		char get_end_node();

		/**
		 * Constructor of edge object
		 * @param start_node node from which edge is starting
		 * @param end_node node in which edge is ending
		 */
		Edge(char start_node, char end_node);
};

/**
 * class implements container of forward and reverse 
 * edges (represented by id) stored in adj list
 */
class Adj_elem {
	private:
		int forward_id;
		int reverse_id;
		char node_id;
		bool inserted_as_forward;
	public:
		/**
		 * Constructor of adj elem object
		 * @param node_id name of node from which forward edge is going
		 * @param forward_id id of edge in forward direction
		 * @param reverse_id id of edge in reverse direction
		 * @param inserted_as_forward flag is true, if edge on forward place is trully forward
		 */
		Adj_elem(char node_id, int forward_id, int reverse_id, bool inserted_as_forward);

		/**
		 * Method returns id of forward edge
		 * @return id of edge in forward direction
		 */
		int get_forward_id();

		/**
		 * Method returns id of reverse edge
		 * @return id of edge in reverse direction
		 */
		int get_reverse_id();

		/**
		 * Getter of the saved name of node
		 * @return name of node from which forward node is going
		 */
		char get_node_id();

		/**
		 * Getter of inserted_as_forward flag
		 * @return value of flag inserted_as_forward
		 */
		bool is_forward_elem();
};

class Nodes {
	private:
		int preorder_position;
		char node_name;
	public:
		int get_preorder_position();
		char get_node_name();
		Nodes(int preorder_position, char name);
};

/// type of adj list (lists implemented using vectors)
typedef vector<vector<Adj_elem>> adj_t;

/// type of list of edges (list implemented as vector)
typedef vector<Edge> edge_list_t;

typedef vector<Nodes> node_list_t;

class utility {
	public:
		/**
		 * Class method checks, wheter edge (specified by id) is 
		 * forward edge, or reverse edge
		 * @param edge_id id of edge
		 * @param adj adj list, where edge is stored
		 * @return true, if edge specified by id is forward, else return false
		 */
		static bool is_forward(int edge_id, adj_t adj) {
			for (int i = 0; i < adj.size(); i++) {
				for (int j = 0; j < adj[i].size(); j++) {
					if (!adj[i][j].is_forward_elem()) {
						continue;
					}
					if (adj[i][j].get_forward_id() == edge_id) {
						return true;
					}
					if (adj[i][j].get_reverse_id() == edge_id) {
						return false;
					}
				}
			}
			throw "Unknown edge id";
		}

		/**
		 * Method counts value of euler tour elem for one edge
		 * 
		 * 1.  Find the edge in adj by edge id
		 * 2.  take id of reverse edge of searched edge
		 * 3.  find the reverse edge as forward edge in adj
		 * 3a. if there is any element in list after edge found in (3),
		 *			then result is id of next forward edge
		 * 3b. if there is no element in list after edge found in (3),
		 *			then result is id of forward edge in first element of list
		 *
		 * @param edge_id id of edge
		 * @param adj adj list, where edge is stored
		 * @return id of edge, which is next in euler tour, in adj
		 */
		static int euler_tour(int edge_id, adj_t adj) {
			// find reverse edge id for given forward edge id
			int reverse_id = -1;
			for (int i = 0; i < adj.size(); i++) {
				for (int j = 0; j < adj[i].size(); j++) {
					if (adj[i][j].get_forward_id() == edge_id) {
						reverse_id = adj[i][j].get_reverse_id();
					}
				}
			}
			if (reverse_id == -1) {
				throw "Edge id not found in adj";
			}

			// find id of reverse edge as forward edge
			for (int i = 0; i < adj.size(); i++) {
				for (int j = 0; j < adj[i].size(); j++) {
					if (adj[i][j].get_forward_id() == reverse_id) {
						if (j == adj[i].size() - 1) {
							// if element is at last index, return
							// id of first forward edge in given list
							return adj[i][0].get_forward_id();

						} else {
							// if element is not at last index, return
							// id of first forward edge in given list
							return adj[i][j + 1].get_forward_id();
						}
					}
				}
			}
			throw "Id of reverse edge not found in adj";
		}

		/**
		 * Method adds vector of adj elements into adj
		 * @param edge_vector vector containing adj elements
		 * @param adj adjacency list representing graph (aka tree with reverse edges)
		 * @return adjacency list containing given edge vector and previously added vectors
		 */
		static adj_t push_edge_vector(vector<Adj_elem> edge_vector, adj_t adj) {
			for (int i = 0; i < adj.size(); i++) {
				// if there is list for given node, then append it
				if (adj[i][0].get_node_id() == edge_vector[0].get_node_id()) {
					adj[i].insert(adj[i].end(), edge_vector.begin(), edge_vector.end());
					return adj;
				}
			}
			// if no list found for given node, then create new list
			adj.push_back(edge_vector);
			return adj;
		}

		/**
		 * Method prints adjacency list in readable format
		 * @param adj adjacency list to be printed
		 */
		static void print_adj(adj_t adj) {
			for (int i = 0; i < adj.size(); i++) {
				cout << adj[i][0].get_node_id() << ": ";
				for (int j = 0; j < adj[i].size(); j++) {
					cout << "(" << adj[i][j].get_forward_id() << ";" << adj[i][j].get_reverse_id() << ") ";
				}
				cout << endl;
			}
		}

		/**
		 * Method for printing edge list
		 * @param edge_list list of edges
		 */
		static void print_edge_list(edge_list_t edge_list) {
			for (int i = 0; i < edge_list.size(); i++) {
				cout << edge_list[i].get_id() << ":" << edge_list[i].get_start_node() << "->" << edge_list[i].get_end_node() << endl;
			}
		}

		/**
		 * Method finds last edge going to root and set its euler next edge to itself
		 * 
		 * @param euler_tour vector containing on index i+1 (aka id of edge) next edge in euler tour (as id of that edge)
		 * @param edge_list list of edges in which search can be done
		 * @param root_name name (aka id) of root of tree
		 * @return fixed euler tour (last edge going to root pointing to itself)
		 */
		static vector<int> fix_euler_tour(vector<int> euler_tour, vector<Edge> edge_list, char root_name) {
			// find last edge ending in root node
			int edge_id;
			for (int i = 0; i < edge_list.size(); i++) {
				if (edge_list[i].get_end_node() == root_name) {
					edge_id = edge_list[i].get_id();
				}
			}

			// for searched edge, replace id of euler tour by self id
			euler_tour[edge_id - 1] = edge_id;
			return euler_tour;
		}

		static int update_ending_edge_noticing_sum(int current_value, int size) {
			return min(size - 1, ((current_value * 2) - 1));
		}

		static int preorder(int weight, int size) {
			return size - weight;
		}
};

#endif