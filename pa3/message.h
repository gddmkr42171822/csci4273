/*********************/
/* This is the most primitive implementation of the Message library.
*/

/* Written by: Shiv Mishra on October 20, 2014 */
/* Last update: October 20, 2014 */
#include <list>
#include <iterator>

class Message
{
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
    list<char> message_container;
    size_t msglen;
};

    Message::Message()
    {
	    msglen = 0;
    }

    Message::Message(char* msg, size_t len)
    {
	msglen = len;
	for(unsigned int i = 0; i < len; i++) {
		message_container.push_back(msg[i]);
	}
    }

    Message::~Message( )
    {
	    message_container.clear();
    }

    void Message::msgAddHdr(char *hdr, size_t length)
    {
	msglen += length;
	for(unsigned int i = 0; i < length; i++) {
		message_container.push_front(hdr[i]);
	}
    }

    char* Message::msgStripHdr(int len)
    {
        if ((msglen < len) || (len == 0)) {
		return NULL;
	}
	char *stripped_content = new char[len];
	list<char> stripped_header_list;
	list<char>::iterator last_char_of_header = message_container.begin();
	advance(last_char_of_header, len);
	stripped_header_list.splice(stripped_header_list.begin(), message_container, message_container.begin(), \
	last_char_of_header);
	int i = 0;
	for(list<char>::iterator it = stripped_header_list.begin(); it != stripped_header_list.end(); it++) {
		stripped_content[i] = *it;
		i++;
	}
	msglen -= len;
	return stripped_content;
    }

    int Message::msgSplit(Message& secondMsg, size_t len)
    {
	if (len > msglen) {
		return 0;
	}
	list<char>::iterator it = message_container.begin();
	advance(it, len);
	secondMsg.message_container.splice(secondMsg.message_container.begin(), message_container, \
	it, message_container.end());
	return 1;
    }

    void Message::msgJoin(Message& secondMsg)
    {
	message_container.splice(message_container.end(), secondMsg.message_container, \
	secondMsg.message_container.begin(), secondMsg.message_container.end());
	msglen += secondMsg.msglen;
	secondMsg.msglen = 0;
    }

    size_t Message::msgLen( )
    {
	return message_container.size();
    }

    void Message::msgFlat(char *buffer)
    {
	int i = 0;
	for(list<char>::iterator it = message_container.begin(); it != message_container.end(); it++) {
		buffer[i] = *it;
		i++;
	}
    }

