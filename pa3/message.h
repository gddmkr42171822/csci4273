class Message {

  public:
    Message();
    Message(char* msg, size_t len);
    ~Message();
    void msgAddHdr(char *hdr, size_t len);
    char *msgStripHdr(int len);
    int msgSplit(Message& secondMsg, int len);
    void msgJoin(Message& secondMsg);
    size_t msgLen();
    void msgFlat(char *buffer);
};

Message::Message() {

};

Message::Message(char* msg, size_t len) {

};

Message::~Message() {

};

void Message::msgAddHdr(char *hdr, size_t len) {

};

char *Message::msgStripHdr(int len) {

};

int Message::msgSplit(Message& secondMsg, int len) {

};

void Message::msgJoin(Message& secondMesg) {

};

size_t Message::msgLen() {

};


void Message::msgFlat(char *buffer) {

};
