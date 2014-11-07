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
    struct character_buffer {
	    char *message_buffer;
	    size_t message_len;
    };
    list<char> message_container;
    size_t msglen;
    char *msg_content;
};

    Message::Message()
    {
    }

    Message::Message(char* msg, size_t len)
    {
	for(unsigned int i = 0; i < len; i++) {
		message_container.push_back(msg[i]);
	}
    }

    Message::~Message( )
    {
    }

    void Message::msgAddHdr(char *hdr, size_t length)
    {
	for(unsigned int i = 0; i < length; i++) {
		message_container.push_front(hdr[i]);
	}
    }

    char* Message::msgStripHdr(int len)
    {
	char *new_msg_content;
	char *stripped_content;
	
        if ((msglen < len) || (len == 0)) return NULL;

	new_msg_content = new char[msglen - len];
	stripped_content = new char[len];
	memcpy(stripped_content, msg_content, len);
	memcpy(new_msg_content, msg_content + len, msglen - len);
	msglen -= len;
	delete msg_content;
	msg_content = new_msg_content;
	return stripped_content;
    }

    int Message::msgSplit(Message& secondMsg, size_t len)
    {
	char *content = msg_content;
	size_t length = msglen;

	if ((len < 0) || (len > msglen)) return 0;

	msg_content = new char[len];
	msglen = len;
	memcpy(msg_content, content, len);
	secondMsg.msglen = length - len;
	secondMsg.msg_content = new char[secondMsg.msglen];
	memcpy(secondMsg.msg_content, content + len, secondMsg.msglen);
	delete content;
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

