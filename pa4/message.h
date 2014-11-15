#ifndef __MESSAGE_H
#define __MESSAGE_H
#include <stdio.h>
#include <list>
#include <string.h>
using namespace std;
class Message {
public:
   
    Message( );
    Message(char* msg, size_t len);
    ~Message( );
    void msgAddHdr(char *hdr, size_t length);
    char* msgStripHdr(int len);
    int msgSplit(Message& secondMsg, size_t len);
    void msgJoin(Message& secondMsg);
    size_t msgLen( );
    void msgFlat(char *buffer);

private:
    size_t msglen;
    class field{
        public:
            field(char* data, size_t len){
                this->data  = data;
                this->len   = len;
            }
            char* data;
            size_t len;
    };
    list<field*> fieldList;
};
#endif
