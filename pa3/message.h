/*********************/
/* This is the most primitive implementation of the Message library.
*/

/* Written by: Shiv Mishra on October 20, 2014 */
/* Last update: October 20, 2014 */

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
    char *msg_content;
};

    Message::Message()
    {
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
        if ((msglen < len) || (len == 0)) return NULL;
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
	if ((len < 0) || (len > msglen)) return 0;
	list<char>::iterator it = message_container.begin();
	advance(it, len);
	secondMsg.message_container.splice(secondMsg.message_container.begin(), message_container, \
	it, message_container.end());
	return 1;
    }

    void Message::msgJoin(Message& secondMsg)
    {
	char *content = msg_content;
	size_t length = msglen;
	
	msg_content = new char[msglen + secondMsg.msglen];
	msglen += secondMsg.msglen;
	memcpy(msg_content, content, length);
	memcpy(msg_content + length, secondMsg.msg_content, secondMsg.msglen);
	delete content;
	delete secondMsg.msg_content;
	secondMsg.msg_content = NULL;
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

