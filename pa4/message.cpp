#include "message.h"
using namespace std;
Message::Message()
{
    msglen = 0;
}

Message::Message(char* msg, size_t len){
    field * init = new field(msg, len);
    fieldList.push_front(init);
    msglen = len;
}

Message::~Message(){
     for(list<field*>::iterator it = fieldList.begin();it != fieldList.end() ;++it){
        delete *it;
    }
}

void Message::msgAddHdr(char *hdr, size_t length)
{
    fieldList.push_front(new field(hdr, length));
    msglen += length;
}

char* Message::msgStripHdr(int len)
{
    size_t found = 0;
    char* buf = new char[len];
    list<field*>::iterator it;
    for(it = fieldList.begin();(it != fieldList.end()) && (found < (size_t)len);){
        found += (*it)->len;
        if(found < (size_t)len){
            memcpy(buf + found, (*it)->data, (*it)->len);
            delete *it;
            it = fieldList.erase(it);
        }else{
            break;
        }
    }

    if(found < (size_t)len){
        delete[] buf;
        return NULL;
    }else if(found > (size_t)len){
        char* origData = (*it)->data;
        //Need to copy part of a header 
        size_t prefixLen = (*it)->len - (found - len);
        size_t remainderLen = found - len;
        memcpy(buf + found - ((*it)->len), origData, prefixLen);
        char* remainder = new char[remainderLen];
        memcpy(remainder, origData + prefixLen, remainderLen);
        fieldList.pop_front();
        fieldList.push_front(new field(remainder, remainderLen));
    }
    msglen -= len;
    return buf;
}

int Message::msgSplit(Message& secondMsg, size_t len) {
    list<field*>::iterator it;
    size_t found = 0;
    for(it = fieldList.begin();(it != fieldList.end()) && (found < len);++it){
        found += (*it)->len;
    }
    if(found < len){
        return 0;
    }
    if(found > len){
        //Need to split a field
        --it;
        char* origData = (*it)->data;
        ++it;
        secondMsg.fieldList.splice(secondMsg.fieldList.begin(), fieldList, it, fieldList.end());
        --it;
        size_t prefixLen = (*it)->len - (found - len);
        char* prefix = new char[prefixLen];
        size_t remainderLen = found - len;
        char* remainder = new char[remainderLen];
        memcpy(prefix, origData, prefixLen);
        memcpy(remainder, origData + prefixLen, remainderLen);
        secondMsg.fieldList.push_back(new field(prefix, prefixLen));
        fieldList.pop_front();
        fieldList.push_front(new field(remainder, remainderLen));
        delete[] origData;
    }else{
        secondMsg.fieldList.splice(secondMsg.fieldList.begin(), fieldList, it, fieldList.end());
    }
    secondMsg.msglen = msglen - len;
    msglen = len;

    return 1;
}

void Message::msgJoin(Message& secondMsg) {
    msglen += secondMsg.msglen;
    secondMsg.msglen = 0;
    fieldList.splice(fieldList.end(), secondMsg.fieldList);
}

size_t Message::msgLen( ) {
    return msglen;
}

void Message::msgFlat(char *buffer) {
    //Assume that sufficient memory has been allocated in buffer
    size_t offset = 0;
    for(list<field*>::iterator it = fieldList.begin();it != fieldList.end() ;++it){
        size_t len = (*it)->len;
        memcpy(buffer + offset, (*it)->data, len);
        offset += len;
    }
}

