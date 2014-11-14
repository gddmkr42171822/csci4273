#ifndef threadpool_h
#define threadpool_h

#include <vector>
#include <queue>
#include <errno.h>
#include <cstring>

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
		unsigned int max_threads;
};

ThreadPool::ThreadPool(size_t threadCount) 
{
	active_threads.resize(threadCount);
	max_threads = threadCount;
	threads_available = 0;
	program_is_over = false;
	pthread_mutex_init(&queue_mutex, NULL);
	for(unsigned int i = 0; i < threadCount; i++) {
		pthread_create(&active_threads[i], NULL, work_helper, this);
	}
}

ThreadPool::~ThreadPool(void) 
{
	while(!workQueue.empty() && (threads_available < active_threads.size())) {
		/* Wait until all the threads have completed their work */
	}
	program_is_over = true;
	for(unsigned int i = 0; i < max_threads; i++) {
		pthread_join(active_threads[i], NULL);
	}
	active_threads.clear();
	pthread_mutex_destroy(&queue_mutex);
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
	if(threads_available > workQueue.size() && workQueue.size() < max_threads) {
		return true;
	}
	else {
		return false;
	}
}

void *ThreadPool::thread_work(void) 
{
	threads_available++;
	while(1) {
		if(pthread_mutex_trylock(&queue_mutex)) { 
		}
		else { 
			threads_available--;
			if(workQueue.size() > 0) {
				void (*dispatch)(void*);
				dispatch_struct *work;
				work = workQueue.front();
				workQueue.pop();
				dispatch = work->dispatch_function;
				dispatch(work->arg);
				delete work;
			}
			pthread_mutex_unlock(&queue_mutex);
			threads_available++;
		}
		if(program_is_over && workQueue.empty()) {
			pthread_exit(NULL);	
		}
	}
	return NULL;
}
#endif
