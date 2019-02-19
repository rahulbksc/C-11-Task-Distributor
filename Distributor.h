/*
	author		: Rahul Yadav 
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
class TaskPool
{
public:
	TaskPool(int count);
	/*
		input - 
		returns a future object 
	*/
	template< class F , class... Args>
	auto addTask(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>;
	~TaskPool();

private:
	std::vector< std::thread > m_workers;
	std::queue< std::function<void()> > m_tasks;

	std::mutex m_mutex;
	std::condition_variable m_cv;
	bool stop;
};

inline TaskPool::TaskPool(int num_tasks)
	:stop(false)
{
	for (int i = 0; i < num_tasks; i++)
		m_workers.emplace_back(
			[this]
			{
				for (;;) 
				{
					std::function<void()>task;
					{
						std::unique_lock<std::mutex>lock(this->m_mutex);

						this->m_cv.wait(lock,
							[this] {return this->stop || !this->m_tasks.empty(); });

						if (this->stop && this->m_tasks.empty())
							return;

						task = std::move(this->m_tasks.front());
						this->m_tasks.pop();
					}
					task();
				}
			}
	);
}

template <class F , class... Args>
auto TaskPool::addTask(F&& f, Args&&... args)
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

		m_tasks.emplace([task]() {(*task)(); });
	}
	m_cv.notify_one();
	
	return res;
}

inline TaskPool::~TaskPool()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		stop = true;
	}
	m_cv.notify_all();

	for (auto &worker : m_workers)
		worker.join();
}
