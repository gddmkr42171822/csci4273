#ifndef eventscheduler_h
#define eventscheduler_h
class EventScheduler 
{
	public:
		EventScheduler(size_t maxEvents);
    		~EventScheduler();
    		int eventSchedule(void evFunction(void *),void *arg, int timeout);
    		void eventCancel(int eventId);
		void test(void);
	private:
		void *event_work(void);
		static void *work_helper(void *instance) {
			return ((EventScheduler*)instance)->event_work();
		}
		struct event {
			void (*evFunction)(void*);
			void	*arg;
			int	timeout;
			int event_id;
		};
		struct myComparator {
			bool operator() (event *lhs, event *rhs) {
				return lhs->timeout > rhs->timeout;
			}
		} vector_comparator;
		vector<event*> event_queue;
		int new_eventId;
		int vector_size;
		int max_events;
		ThreadPool *event_pool_pointer;
};

void EventScheduler::test(void) 
{
	new_eventId = 0;
	vector_size = 4;
	max_events = 3;
	//(void) maxEvents;
	cout << "Testing vector queue" << endl;

	event* event1 = new event;
	event1->timeout = 5;
	event1->event_id = new_eventId;

	event* event2 = new event;
	event2->timeout = 6;
	new_eventId++;
	event2->event_id = new_eventId;

	event* event3 = new event;
	event3->timeout = 4;
	new_eventId++;
	event3->event_id = new_eventId;

	event* event4 = new event;
	event4->timeout = 4;
	new_eventId++;
	event4->event_id = new_eventId;

	if(vector_size <= max_events) {
		cout << "queue is not full" << endl;
	}
	else {
		cout << "queue is full" << endl; 
	}
	event_queue.push_back(event4);
	event_queue.push_back(event1);
	event_queue.push_back(event3);
	event_queue.push_back(event2);
	int eventId = 5;
	sort(event_queue.begin(), event_queue.end(), vector_comparator);
	for(vector<event*>::iterator it = event_queue.begin(); it != event_queue.end(); it++) {
		cout << "EventId |" << (*it)->event_id << "|; timeout |" << (*it)->timeout << "|" << endl;
	}
	bool event_is_in_queue = false;
	for(vector<event*>::iterator it = event_queue.begin(); it != event_queue.end(); it++) {
		if((*it)->event_id == eventId) {
			cout << "Erasing event id " << (*it)->event_id << endl; 
			event_queue.erase(it);
			event_is_in_queue = true;
			break;
		}
	}
	if(event_is_in_queue == false) {
		cerr << "Event id was not found in queue!" << endl;
	}
	for(vector<event*>::iterator it = event_queue.begin(); it != event_queue.end(); it++) {
		cout << "EventId |" << (*it)->event_id << "|; timeout |" << (*it)->timeout << "|" << endl;
	}
	cout << "Popping event id |" << (*(event_queue.end()-1))->event_id << "| with lowest time |"\
	<< (*(event_queue.end()-1))->timeout << "| off queue" << endl;
	event_queue.pop_back();
	cout << "Size of queue is " << event_queue.size() << endl;
	cout << "End test" << endl;
};


EventScheduler::EventScheduler(size_t maxEvents) 
{
	pthread_t event_thread;
	new_eventId = 0;
	vector_size = 0;
	max_events = maxEvents;
	ThreadPool event_pool(max_events);
	event_pool_pointer = &event_pool;
	pthread_create(&event_thread, NULL, work_helper, this);
}

EventScheduler::~EventScheduler(void) 
{
	cout << "Destroying eventScheduler object" << endl;	
};

int EventScheduler::eventSchedule(void evFunction(void *),void *arg, int timeout) 
{
	vector_size++;
	if(vector_size <= max_events) {
		new_eventId++;
		event *new_event = new event;	
		new_event->evFunction = evFunction;
		new_event->arg = arg;
		new_event->timeout = timeout;
		new_event->event_id = new_eventId;
		event_queue.push_back(new_event);
		sort(event_queue.begin(), event_queue.end(), vector_comparator);
		return new_event->event_id;
	}
	else {
		cerr << "Queue is full.  Cannot schedule anymore events!" << endl;
		vector_size--;
		return -1;
	}
};

void EventScheduler::eventCancel(int eventId) 
{
	bool event_is_in_queue = false;
	for(vector<event*>::iterator it = event_queue.begin(); it != event_queue.end(); it++) {
		if((*it)->event_id == eventId) {
			cout << "Erasing event id " << (*it)->event_id << endl; 
			event_queue.erase(it);
			vector_size--;
			event_is_in_queue = true;
			break;
		}
	}
	if(event_is_in_queue == false) {
		cerr << "Event id was not found in queue!" << endl;
	}
};

void *EventScheduler::event_work(void) 
{
	int select_error;
	void (*dispatch)(void*);
	struct event *execute_event;	
	struct timeval event_timeout = { 0, 0}; 
	while(1) {
		if(!event_queue.empty()) {
			execute_event = *(event_queue.end()-1);
			event_timeout.tv_usec = execute_event->timeout;//(*(event_queue.end()-1)->timeout);
			cout << "Select will wait |" << event_timeout.tv_usec <<"| microseconds" << endl;
			cout << "Event id is |" << execute_event->event_id << "|" << endl;
			select_error = select(0, NULL, NULL, NULL, &event_timeout);
			if(select_error == 0) {
				cout << "Event will now be executed!" << endl;
			}
			else {
				cerr << "select did not work for the event!" << endl;
			}
			if(event_pool_pointer->thread_avail()) {
				event_queue.pop_back();
				dispatch = execute_event->evFunction;
				event_pool_pointer->dispatch_thread(*dispatch, execute_event->arg);
				delete execute_event;
			}
			else {
				cerr << "Could not schedule event!" << endl;
			}
		}
	}
	return NULL;
};


#endif
