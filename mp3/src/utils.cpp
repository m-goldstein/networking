#ifndef _UTILS_CPP
#define _UTILS_CPP
#include "utils.hpp"
#define MAX_MSG_LENGTH 128

using namespace std;
vector<Message> createMessages(char* path)
{
    ifstream inf = ifstream(path);
    vector<Message> result;
    string a_string;
    string tmp;
    char cmsg[MAX_MSG_LENGTH];
    int node_1;
    int node_2;
    for (getline(inf, a_string); !inf.eof(); getline(inf, a_string)) {
        stringstream line(a_string);
        if (line >> node_1 >> node_2) {
            sscanf(a_string.c_str(), "%*d%*d%*c%[^\n]%*s", cmsg); /* regular expression to ignore first 2 integers and ' ' (whitespace) and parse out message */
            tmp = std::string(cmsg); /* cast char* to a string */
            //cout << "Node 1: " << node_1 << "\tNode 2: "<<node_2 <<"\tMsg: "<<tmp<<"\n";
            Message msg(node_1, node_2, tmp); /* from, to, msg */
            result.push_back(msg);
        }
    }
    inf.close();
    return result;
}

void print_messages(vector<Message>& messages) 
{
    for (auto it : messages) {
        cout << "Source: " << it.from << "\tDestination: "<<it.to <<"\tMsg: "<<it.msg<<"\n";
    }
}

unordered_map<int, node*> createGraph(char* path) {
    ifstream inf = ifstream(path);
    
    int node_1;
    int node_2;
    int cost;
    //string in;

    unordered_map<int, node*> result;
    
    inf >> node_1 >> node_2 >> cost;

    while (!inf.eof()) {
        //cout << node_1 << " " << node_2 << " " << cost << endl;
        
        if (result[node_1] == nullptr) { 
            result[node_1] = new node(node_1); 
        }
        if (result[node_2] == nullptr) {
            result[node_2] = new node(node_2);
        }
        
        result[node_1]->update_neighbor(result[node_2], cost);
        result[node_2]->update_neighbor(result[node_1], cost);

        inf >> node_1 >> node_2 >> cost;
    }
    
    //cout << in << endl;
    
    /*
    node* curNode;
    for (auto it = result.begin(); it != result.end(); ++it) {
        curNode = (*it).second;

        int x = 10 + 2;
        (void)x;
    }
    */
    inf.close();
    return result;
}


#endif // _UTILS_CPP
