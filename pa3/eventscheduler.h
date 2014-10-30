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
	int queue_mutex_error, pthread_create_error;
	threads_available = 0;
	program_is_over = false;
	event_threads.resize(maxEvents);
	new_eventId = 0;
	queue_mutex_error = pthread_mutex_init(&queue_mutex, NULL);
	if(queue_mutex_error) {
		fprintf(stderr, "Error creating queue mutex\n");
		exit(EXIT_FAILURE);
	}
	for(unsigned int i = 0; i < maxEvents; i++) {
		pthread_create_error = pthread_create(&event_threads[i], NULL, work_helper, this);
		if(pthread_create_error) {
			fprintf(stderr, "Error creating thread |%d|\n", i);
			exit(EXIT_FAILURE);
		}
		else {
			printf("Creating thread |%d|\n", i);
		}
	}
	max_events = maxEvents;
};

EventScheduler::~EventScheduler(void) 
{
	int pthread_join_error, queue_mutex_error;
	while(!event_queue.empty() && (threads_available < max_events)) {
	}
	program_is_over = true;
	for(unsigned int i = 0; i < event_threads.size(); i++) {
		pthread_join_error = pthread_join(event_threads[i], NULL);
		if(pthread_join_error) {
			fprintf(stderr, "Error joining thread |%d|\n", i);
		}
		else {
			printf("Thread |%d| destroyed!\n", i);
		}
	}
	event_threads.clear();
	queue_mutex_error = pthread_mutex_destroy(&queue_mutex);
	if(queue_mutex_error) {
		fprintf(stderr, "Error destroying queue mutex\n");
	}
};

int EventScheduler::eventSchedule(void evFunction(void *),void *arg, int timeout) 
{
	if(event_queue.size() < max_events) {
		new_eventId++;
		printf("putting event %d in queue\n", new_eventId);
		event *new_event = new event;	
		new_event->evFunction = evFunction;
		new_event->arg = arg;
		new_event->timeout = timeout;
		new_event->event_id = new_eventId;
		event_queue.insert(event_queue.begin(), new_event);
		sort(event_queue.begin(), event_queue.end(), vector_comparator);
		return new_event->event_id;
	}
	else {
		fprintf(stderr, "Queue is full!  Cannot schedule event |%d|!\n", new_eventId + 1);
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
	int select_error, mutex_trylock_error, mutex_unlock_error;
	void (*dispatch)(void*);
	struct event *execute_event;	
	struct timeval event_timeout = { 0, 0}; 
	threads_available++;
	while(1) {
		if(!event_queue.empty()) {
			mutex_trylock_error = pthread_mutex_trylock(&queue_mutex);
			if(mutex_trylock_error) {
			}
			else {
				threads_available--;
				execute_event = *(event_queue.end()-1);
				event_queue.pop_back();
				event_timeout.tv_usec = execute_event->timeout;
				cout << "Select will wait |" << event_timeout.tv_usec <<"| microseconds" \
				" on Event id |" << execute_event->event_id << "|" << endl;
				select_error = select(0, NULL, NULL, NULL, &event_timeout);
				if(select_error == 0) {
					dispatch = execute_event->evFunction;
					dispatch(execute_event->arg);
					mutex_unlock_error = pthread_mutex_unlock(&queue_mutex);
					if(mutex_unlock_error) {
						fprintf(stderr, "error unlocking queue!\n");
					}
					delete execute_event;
				}
				else {
					cerr << "select did not work for the event!" << endl;
					mutex_unlock_error = pthread_mutex_unlock(&queue_mutex);
					if(mutex_unlock_error) {
						fprintf(stderr, "error unlocking queue!\n");
					}
				}
				threads_available++;
			}
		}
		if(program_is_over && event_queue.empty()) {
			pthread_exit(NULL);
		}
	}
	return NULL;
};
#endif
