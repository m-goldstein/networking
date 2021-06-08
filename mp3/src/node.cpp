//#pragma once

#include "node.h"
#define POS_INF 999
#define NEG_INF -999
#define INVAL_NODE -1
class node;
class Compare;

node::node() 
{

}

node::node(int id) 
{
    _id = id;
    _forward_table[_id] = pair<int, int>(id, 0);
    //_forward_table = vector<pair<int, int>>(max(id, DEFAULT_TABLESIZE), pair<int, int>(-1, -1));
    //_forward_table[id] = pair<int, int>(id, 0);
}

void node::update_neighbor(node* neighbor, int dist) 
{
    /*
    if (neighbor->id <= _forward_table.size()) {
        _forward_table.resize(neighbor->id);
    }
    */
    if (dist == NEG_INF) {
        _neighbors.erase(neighbor->_id);
        _forward_table.erase(neighbor->_id);
    }
    else {
        _neighbors[neighbor->_id].first = neighbor,
        _neighbors[neighbor->_id].second = dist;
        if (_forward_table.find(neighbor->_id) == _forward_table.end()
            || _forward_table[neighbor->_id].second > dist) {
            _forward_table[neighbor->_id] = pair<int, int>(neighbor->_id, dist);
            // Update neighbors?
        }
    }
    // Probably wrong, gotta do the updatin stuffs
    //_forward_table[neighbor->id] = dist;

}

string node::ft_str() {
    string result;
    // Debug message
    #ifdef DEBUG
    result += "\nForward Table " + to_string(_id) + ": \n";
    #endif
    for (auto it = _forward_table.begin(); it != _forward_table.end(); ++it) {
        result += to_string(it->first) + " " + to_string((it->second).first) 
                + " " + to_string((it->second).second) + "\n";
    }

    return result;
}

void node::dijkstra() 
{
    // make priority queue pq
    priority_queue<pair<node*, int>, vector<pair<node*, int>>, Compare> pq;
    unordered_map<int, int> dist, prev;

    pq.push(pair<node*, int>(this, INVAL_NODE));
    node* u;
    while (!pq.empty()) {
        u = pq.top().first;
        pq.pop();
        for (auto it = u->_neighbors.begin(); it != u->_neighbors.end(); ++it) {
            pair<node*, int> entry = it->second;
            node* v = (entry.first);
            if (dist[v->_id] == 0 || dist[v->_id] > (dist[u->_id] + entry.second)) {
                prev[v->_id] = u->_id;
                dist[v->_id] = dist[u->_id] + entry.second;
                pq.push(pair<node*, int>(v, dist[v->_id]));
            }
        }
    }
    dist[_id] = 0;
    prev[_id] = _id;
    
    //if (dist.size() > 1) {
        _forward_table = map<int, pair<int,int>>();
        _forward_table[_id] = pair<int, int>(_id, 0);

        int curNode;
        for (auto it = dist.begin(); it != dist.end(); ++it) {
            if (it->first != _id) {
                //else if (_forward_table[i].first == -1) { break; }
                //cout << "Finding entry for node " << it->first << endl;
                curNode = it->first;
                while (prev[curNode] != this->_id) { 
                    curNode = prev[curNode]; 
                    //cout << "   curNode = " << curNode << endl;
                }
                _forward_table[it->first].first = curNode;
                _forward_table[it->first].second = it->second;
            }
        }
    //}
    //raise(SIGINT);
}
/* helper functions */
bool node::update_forward_table(int other_id, map<int, pair<int,int>> &other) // gets a forward table as input
{
    //cout << "Cur ID: " << _id << endl;
    //cout << "   other_id: " << other_id << endl;
    bool updated = false;
    for (auto it : other) {
        if ((it.first) != other_id && (it.second).first != this->_id) {
            //cout << "      Checking entry for " << it.first << endl;
            if (this->_forward_table.find(it.first) == this->_forward_table.end()) /* does not exist */ {
                this->_forward_table[it.first].first = other_id;
                this->_forward_table[it.first].second = (it.second).second + this->_neighbors[other_id].second;
                updated = true;
            } else {
                if (this->_forward_table[it.first].second > (it.second).second + this->_neighbors[other_id].second) {
                    this->_forward_table[it.first].second = (it.second).second+ this->_neighbors[other_id].second;
                    this->_forward_table[it.first].first = other_id;
                    updated = true;
                }
            }
        }
    }
    return updated;
}

/* generalized update_forward_table function */
void node::update_forward_table()
{
    bool updated = false;
    for (auto it : this->_neighbors) {
        pair<node*, int> entry = it.second;
        node*v = entry.first;
        updated = (updated || this->update_forward_table(v->_id, v->_forward_table));
    }
    if (updated) { /* propagate changes */
        for (auto it : this->_neighbors) {
            ((it.second).first)->update_forward_table(this->_id, this->_forward_table);
        }
    }
}

void node::reset_forward_table() {
    _forward_table = map<int, pair<int, int>>();
    for (auto it : _neighbors) {
        _forward_table[it.first] = pair<int,int>(it.first, (it.second).second);
    }
    _forward_table[_id] = pair<int, int>(_id, 0);
}

/* Bellman-Ford (Shimbel) algorithm implementation for DistVec */
void node::shimbel()
{
    // pseudocode:
    //   shimbel(s):
    //      dist[s] <-- 0
    //      for every vertex v != s:
    //          dist[v] <-- INF
    //      for i <-- 1 to V-1:
    //         for every edge u->v:
    //             if dist[v] > dist[u] + w(u->v):
    //                dist[v] <-- dist[u] + w(u->v)

    if (this->_active == true) {
        return;
    }
    this->_active = true;
    unordered_map<int,int> dist, prev;
    dist[this->_id] = 0;
    for (auto it : this->_forward_table) { // preprocessing
        if (this->_id != it.first && it.first != INVAL_NODE) {
            dist[it.first] = POS_INF;
        }
    }
    for (auto it = _neighbors.begin(); it != _neighbors.end(); ++it) {
        pair<node*, int> entry = it->second;
        node* v = (entry.first);
        //for (auto it2 = v->_neighbors.begin(); it2 != v->_neighbors.end(); ++it2) {
        //    pair<node*, int> entry2 = it2->second;
        //    node* u = (entry2.first);
        //    u->shimbel();
        //    if (dist[v->_id] > dist[u->_id] + entry2.second) {
        //        dist[v->_id] = dist[u->_id] + entry2.second;
        //    }
        //}
        v->shimbel();
        if (v->_id != this->_id && dist[v->_id] > dist[this->_id] + entry.second) {
            dist[v->_id] = dist[this->_id] + entry.second;
        }
        pair<int,int> p(v->_id, dist[v->_id]);
        if (_forward_table.find(v->_id) == _forward_table.end()) {
            _forward_table[v->_id] = p;
        }
        //v->shimbel();
    }

    //pair<int,int> p(_id, dist[_id]);
    //if (_forward_table.find(_id) == _forward_table.end()) {
    //    _forward_table[_id] = p;
    //}
    //_forward_table.push_back(p);
    //_forward_table[entry.second].first = v->_id;
    //_forward_table[entry.second].second = dist[v->_id];

    //    v->shimbel();
    //}
    this->_active = false;
    // Debug
    cout << "Source: " << to_string(this->_id) << "\n";
    for (auto it : dist) {
        cout << "Dest: " << to_string(it.first) << "\t Cost: "<<to_string(it.second) << "\n";
    }

}

/* 
 * return string containing source, dest, path cost, path taken (hops), 
 * and message in specified format.
 */
string node::send_message(int dest, string msg)
{
    //cout << "Sending message: \"" << msg << "\" from "<< this->_id << " to "<< dest << "\n"; 
    string result = "";
    result.append("from " + to_string(this->_id));
    result += " to " + to_string(dest);
    if (_forward_table.find(dest) != _forward_table.end()) {
        result += " cost " + to_string(_forward_table[dest].second);
        result += " hops ";
        node* curNode = this;
        //while (curNode->_id != dest) {
        while (curNode != NULL) {
            if (curNode->_id == dest)
                break;
            result += to_string(curNode->_id);
            result += " ";
            curNode = (curNode->_neighbors[(curNode->_forward_table[dest]).first]).first;
        }
    }
    else {
        result += " cost infinite hops unreachable ";
    }
    result += "message " + msg + "\n";
    //cout << "[Debug] Writing to output: " << result <<"\n";
    return result;
    }

