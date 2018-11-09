/*
	author		: Rahul Yadav , krishna keshav
	date		: 23/05/2018

	Description	: M tasks will be executed on a N number of threads.
	Client can add tasks as a functions and function are executed by available thread.
*/
#ifndef INCLUDE_TaskPool_H_
#define INCLUDE_TaskPool_H_

#include <thread>
#include <future>
#include <queue>
#include <vector>
#include <iostream>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <string>
#include "enum.h"

class TaskPool
{
public:
	/*
	 * Creates N threads
	 * Runs each thread to execute a Task
	 * (function provided by the Client)
	*/
	TaskPool(size_t);
	/*
		input - 
		returns a future object 
	*/
	template< class T ,class F , class... Args>
	auto addTask(T ,F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>;
	~TaskPool();

	int get_task_queue_size()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_tasks.size() + m_prioty_tasks.size();
	};

	int set_priority_flag();

	int reset_priority_flag();

	//int stop_thread(long int task_id);

	int restart_thread(std::string thread_name);

private:

	void show_all_thread();

	void runThread (std::string);

	std::unordered_map< std::string ,std::thread >  m_workers;
	std::queue< std::pair<int,std::function<void() > > > m_tasks;
	std::unordered_map<std::string ,long int > m_thread_task;

	std::mutex m_mutex;
	std::condition_variable m_cv;
	bool stop;

	// priority flag for priority documents............
	bool m_prioty_flag;
	std::queue< std::pair<int,std::function<void() > > > m_prioty_tasks;


};
inline TaskPool::TaskPool(size_t num_tasks)
	:stop(false) ,m_prioty_flag(false)
{

	for (size_t i = 0; i < num_tasks; i++)
	{
		std::string threadk_name = "Task_Pool_thread_" + std::to_string(i);
		std::thread threadobj(&TaskPool::runThread,this, threadk_name);
		m_workers[threadk_name] = std::move( threadobj );
	}
}

inline TaskPool::~TaskPool()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		stop = true;
	}
	m_cv.notify_all();

	for (auto &worker : m_workers)
		worker.second.join();
}
template <typename T,class F , class... Args>
auto TaskPool::addTask(T id ,F&& f, Args&&... args)
	->std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;

	auto task = std::make_shared<std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f),std::forward<Args>(args)...)
		);

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		if (stop)
			throw std::runtime_error("Enqueue on stopped Taskpool");
		if( m_prioty_flag )
		{
			m_prioty_tasks.emplace( std::make_pair(id,[task]() {(*task)(); }) );
		}else
		{
			m_tasks.emplace( std::make_pair(id,[task]() {(*task)(); }) );
		}
	}
	m_cv.notify_one();
	return res;
}

#endif /* INCLUDE_TaskPool_H_ */



