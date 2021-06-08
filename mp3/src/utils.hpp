#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <unordered_map>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <queue>
#include <utility>
#include <sstream>

#include "node.h"

using namespace std;
class Message {
    public:
        int from;
        int to;
        string msg;
        Message();
        Message(int _from, int _to, string _msg) : from(_from), to(_to), msg(_msg) {} /* initializer list :) */
    private:
};

unordered_map<int, node*> createGraph(char* path);
vector<Message> createMessages(char* path);
void print_messages(vector<Message>&messages);
#endif // _UTILS_HPP
