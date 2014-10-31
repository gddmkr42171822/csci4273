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
		vector<pthread_t> event_threads;
		int new_eventId;
		unsigned int max_events;
		pthread_mutex_t queue_mutex;
		bool program_is_over;
		unsigned int threads_available;
};

EventScheduler::EventScheduler(size_t maxEvents) 
{
	threads_available = 0;
	program_is_over = false;
	event_threads.resize(maxEvents);
	new_eventId = 0;
	pthread_mutex_init(&queue_mutex, NULL);
	for(unsigned int i = 0; i < maxEvents; i++) {
		pthread_create(&event_threads[i], NULL, work_helper, this);
	}
	max_events = maxEvents;
};

EventScheduler::~EventScheduler(void) 
{
	while(!event_queue.empty() && (threads_available < max_events)) {
	}
	program_is_over = true;
	for(unsigned int i = 0; i < event_threads.size(); i++) {
		pthread_join(event_threads[i], NULL);
	}
	event_threads.clear();
	pthread_mutex_destroy(&queue_mutex);
};

int EventScheduler::eventSchedule(void evFunction(void *),void *arg, int timeout) 
{
	if(event_queue.size() < max_events) {
		new_eventId++;
		event *new_event = new event;	
		new_event->evFunction = evFunction;
		new_event->arg = arg;
		new_event->timeout = timeout;
		new_event->event_id = new_eventId;
		event_queue.insert(event_queue.begin(), new_event);
		return new_event->event_id;
	}
	else {
		return -1;
	}
};

void EventScheduler::eventCancel(int eventId) 
{
	for(vector<event*>::iterator it = event_queue.begin(); it != event_queue.end(); it++) {
		if((*it)->event_id == eventId) {
			event_queue.erase(it);
			break;
		}
	}
};

void *EventScheduler::event_work(void) 
{
	void (*dispatch)(void*);
	struct event *execute_event;	
	struct timeval event_timeout = { 0, 0}; 
	threads_available++;
	while(1) {
		if(pthread_mutex_trylock(&queue_mutex)) {
		}
		else {
			threads_available--;
			if(event_queue.size() > 0) {
				if(event_queue.size() > 1) {
					sort(event_queue.begin(), event_queue.end(), vector_comparator);
				}
				execute_event = *(event_queue.end()-1);
				event_timeout.tv_usec = execute_event->timeout;
				select(0, NULL, NULL, NULL, &event_timeout);
				event_queue.pop_back();
				dispatch = execute_event->evFunction;
				dispatch(execute_event->arg);
				delete execute_event;
			}
			pthread_mutex_unlock(&queue_mutex);
			threads_available++;
			}
		
		if(program_is_over && event_queue.empty()) {
			pthread_exit(NULL);
		}
	}
	return NULL;
};
#endif
