#pragma once

//#define DEBUG

#include <vector>
#include <queue>
#include <utility>
#include <unordered_map>
#include <iostream>
#include <signal.h>
#include <map>

#define DEFAULT_TABLESIZE   16

using namespace std;

class node {
    public:
        int _id;
        map<int, pair<int, int>> _forward_table;
        // We could also use a hashmap, might be better
        unordered_map<int, pair<node*, int>> _neighbors;
        bool _active; // Use to prevent repeated recursive Bellman-Fords?

        node();

        node(int id);

        void update_neighbor(node* neighbor, int dist);
        
        string ft_str();
        
        void add_neighbor(node* neighbor, int dist);
        
        string send_message(int dest, string msg); 
        
        /* This function should update the forwarding table appropriately and
         * call it on all the neighbors */
        //void update_forward_table(int node_id, int dist);
        bool update_forward_table(int other_id, map<int, pair<int,int>>&other);
        void update_forward_table();
        
        void reset_forward_table();
        void dijkstra();
        void shimbel();
    private:
        class Compare {
            public:
                bool operator() (pair<node*, int>& a, pair<node*, int>& b) {
                    if (a.second < b.second) { return false; }
                    else if (b.second > a.second) { return true; }
                    else { return (a.first)->_id > (b.first)->_id; }
                }
            private:
        };
};

