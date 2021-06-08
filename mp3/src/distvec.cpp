#ifndef _DISTVEC_CPP
#define _DISTVEC_CPP

#include "utils.hpp"

using namespace std;

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage ./distvec topofile messagefile changesfile\n");
        return -1;
    }
    ofstream fpOut = ofstream("output.txt");    
    unordered_map<int, node*> node_map = createGraph(argv[1]);
    // Create initial topology file
    for (auto it : node_map) {
        it.second->update_forward_table();
    }
    for (auto it : node_map) {
        it.second->update_forward_table();
    }
    
    // Get all messages
    vector<Message> messages = createMessages(argv[2]);
    
    // Write out initial topology and send messagse
    for (int i = 1; i <= node_map.size(); i++) {
        string table = node_map[i]->ft_str();
        fpOut << table;
    }
    
    string msg_out;
    for (auto it : messages) {
        if (node_map.find(it.from) != node_map.end()) {
            msg_out = node_map[it.from]->send_message(it.to, it.msg);
        }
        else {
            msg_out = "from " + to_string(it.from) + " to " + to_string(it.to) +
                     " cost infinite hops unreachable message " + it.msg + "\n";
        }
        fpOut << msg_out;
    }
    
    ifstream changefile = ifstream(argv[3]);
    int node_1, node_2, delta;
    while (changefile >> node_1 >> node_2 >> delta) { 
        #ifdef DEBUG
        fpOut << "\n*** Change: " << node_1 << " " << node_2 << " " << delta << " ***\n";
        #endif
        if (node_map[node_1] == nullptr) {
            node_map[node_1] = new node(node_1);
        }
        if (node_map[node_2] == nullptr) {
            node_map[node_2] = new node(node_2);
        }
        
        node_map[node_1]->update_neighbor(node_map[node_2], delta);
        node_map[node_2]->update_neighbor(node_map[node_1], delta);
        
        for (auto it : node_map) {
            (it.second)->reset_forward_table();
        }
        // Update topologies
        for (auto it : node_map) {
            it.second->update_forward_table();
        }
        for (auto it : node_map) {
            it.second->update_forward_table();
        }
        
        // Write topology
        for (int i = 1; i <= node_map.size(); i++) {
            string table = node_map[i]->ft_str();
            fpOut << table;
        
        }
        // Send messages
        for (auto it : messages) {
            if (node_map.find(it.from) != node_map.end()) {
                msg_out = node_map[it.from]->send_message(it.to, it.msg);
            }
            else {
                msg_out = "from " + to_string(it.from) + " to " + to_string(it.to) +
                         " cost infinite hops unreachable message " + it.msg + "\n";
            }
            fpOut << msg_out;
        }
    }
    
    fpOut.close();
    return 0;

    /*
    // Write out initial topology and send messagse
    for (int i = 1; i <= node_map.size(); i++) {
        string table = node_map[i]->ft_str();
        fpOut << table;
        cout << table ;
    }

    //for (auto it : node_map) {
        //it.second->dijkstra();
        //it.second->shimbel();
     //   it.second->update_forward_table();
      //  break;
    //}
    string msg_out;

     write message to output file */
    /*
    for (auto it : messages) {
        msg_out = node_map[it.from]->send_message(it.to, it.msg);
        fpOut << msg_out;
    }
    fpOut.close();
    //for (auto it : node_map) {
        //it.second->shimbel();
    //}
    //print_messages(messages);
    //FILE *fpOut;
    //fpOut = fopen("output.txt", "w");
    //fclose(fpOut);
    return 0;*/
}

#endif // _DISTVEC_CPP
