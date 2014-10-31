#ifndef threadpool_h
#define threadpool_h
class ThreadPool 
{
	public:
		ThreadPool(size_t threadCount); // This is the constructor
		~ThreadPool(); // This is the destructor
		int dispatch_thread(void dispatch_function(void*), void *arg);
		bool thread_avail();
	private:
		void *thread_work(void);
		static void *work_helper(void *instance) {
			return ((ThreadPool*)instance)->thread_work();
		}
		struct dispatch_struct {

			void (*dispatch_function)(void*);
			void *arg;

		};
		vector<pthread_t> active_threads;
		queue<dispatch_struct*> workQueue;
		pthread_mutex_t queue_mutex;
		unsigned int threads_available;
		bool program_is_over;
};

ThreadPool::ThreadPool(size_t threadCount) 
{
	active_threads.resize(threadCount);
	int pthread_create_error, queue_mutex_error;
	unsigned int i;
	threads_available = 0;
	program_is_over = false;
	queue_mutex_error = pthread_mutex_init(&queue_mutex, NULL);
	if(queue_mutex_error) {
		fprintf(stderr, "Error creating queue mutex\n");
		exit(EXIT_FAILURE);
	}
	for(i = 0; i < threadCount; i++) {
		printf("Creating thread |%d|!\n", i);
		pthread_create_error = pthread_create(&active_threads[i], NULL, work_helper, this);
		if(pthread_create_error) {
			fprintf(stderr, "Error creating thread |%d|\n", i);
			exit(EXIT_FAILURE);
		}
	}
}

ThreadPool::~ThreadPool(void) 
{
	int pthread_join_error, queue_mutex_error;
	unsigned int i;
	long int num_vector_threads = active_threads.size();
	while(!workQueue.empty() && (threads_available < active_threads.size())) {
		/* Wait until all the threads have completed their work */
	}
	program_is_over = true;
	for(i = 0; i < num_vector_threads; i++) {
		pthread_join_error = pthread_join(active_threads[i], NULL);
		if(pthread_join_error) {
			fprintf(stderr, "Error joining thread |%d|\n", i);
		}
		else {
			printf("Thread |%d| destroyed!\n", i);
		}
	}
	active_threads.clear();
	queue_mutex_error = pthread_mutex_destroy(&queue_mutex);
	if(queue_mutex_error) {
		fprintf(stderr, "Error destroying queue mutex\n");
	}
}

int ThreadPool::dispatch_thread(void dispatch_function(void*), void *arg) 
{
	dispatch_struct *thread_dispatch = new dispatch_struct;
	thread_dispatch->arg = arg;
	thread_dispatch->dispatch_function = dispatch_function;
	workQueue.push(thread_dispatch);
	return 0;
}

bool ThreadPool::thread_avail(void) 
{
	if((threads_available > 0) && (threads_available > workQueue.size())) {
		return true;
	}
	else {
		return false;
	}
}

void *ThreadPool::thread_work(void) 
{
	int mutex_trylock_error, mutex_unlock_error;
	threads_available++;
	while(1) {
		if(!workQueue.empty()) {
			mutex_trylock_error = pthread_mutex_trylock(&queue_mutex);
			if(mutex_trylock_error) { // A thread has already locked the queue
			}
			else { // This thread has acquired the lock on the queue
				threads_available--;
				void (*dispatch)(void*);
				dispatch_struct *work;
				if(workQueue.size() != 0) {
					work = workQueue.front();
					workQueue.pop();
					dispatch = work->dispatch_function;
					dispatch(work->arg);
					delete work;
				}
				mutex_unlock_error = pthread_mutex_unlock(&queue_mutex);
				if(mutex_unlock_error) {
					fprintf(stderr, "Error unlocking queue!\n");
				}
				threads_available++;
			}
		}
		if(program_is_over && workQueue.empty()) {
			break;
		}
	}
	return NULL;
}
#endif
