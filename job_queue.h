#pragma once

#include <functional>
#include <deque>
#include <mutex>
#include <utility>
#include <thread>
#include <vector>
#include <condition_variable>
#include <boost/heap/fibonacci_heap.hpp>
using boost::heap::fibonacci_heap;
typedef std::function<void ()> job_type;

struct heap_data {
	fibonacci_heap<heap_data>::handle_type handle;
	int rnd_n;
	job_type myjob;
	heap_data(int i, job_type&& mj):
		rnd_n(i), myjob(mj)
	{}
	bool operator<(heap_data const & rhs) const
	{
		return rnd_n < rhs.rnd_n;
	}
};

class job_queue {
public:
	job_queue(int num_thread) : unfinished_jobs_(0), num_thread_(num_thread), num_jobs_(0) {}
	void push_job(job_type job) {
		
		//heap_data h_data(rand()%100000001,std::move(job));

		lock_t lock(mutex_);
		insert_job(std::move(job));
		++unfinished_jobs_;
		++num_jobs_;
	}
	void run_jobs() {
		std::vector<std::thread> threads;
		for (int id = 0; id < num_thread_; ++id) {
			threads.emplace_back(functor(this, id));
		}
		for (int id = 0; id < num_thread_; ++id) {
			threads[id].join();
		}
	}
	int get_num_jobs() const {
		return num_jobs_;
	}
	~job_queue() {
//		std::cout << "num_jobs: " << num_jobs_ << std::endl;
	}
private:
	struct functor {
		job_queue* thiz_;
		int id_;
		functor(job_queue* thiz, int id) : thiz_(thiz), id_(id) {
		}
		void operator()() {
			job_type job;
			for (;;) {
				size_t queue_size = 0;
//				bool should_sleep = false;
				{
					lock_t lock(thiz_->mutex_);
					for (;;) {
						if (thiz_->unfinished_jobs_ == 0) return;
						if (thiz_->jobs_.size() > 50) break;
						if (!thiz_->jobs_.empty() && id_ == 0) break;
						thiz_->cond_var_.wait(lock);
					}
					job = thiz_->pop_job();
//					if (thiz_->jobs_.size() < 100) should_sleep = true;
					queue_size = thiz_->jobs_.size();
				}
//				std::cout << "Queue Size: " << queue_size << std::endl;
				job();
//				std::this_thread::sleep_for(std::chrono::milliseconds(0));
				{
					lock_t lock(thiz_->mutex_);
					--thiz_->unfinished_jobs_;
					if (thiz_->unfinished_jobs_ == 0
						|| thiz_->jobs_.size() > 100) thiz_->cond_var_.notify_all();
				}
			}
		}
	};
	void insert_job(job_type&& job) {
		
		heap_data h_data(rand()%1000001,std::move(job));
		//jobs_.push_back(std::move(job));
		jobs_.push(h_data);	
	}
	job_type pop_job() {
		//size_t idx = rand() % jobs_.size();
		//job_type job = std::move(jobs_[idx]);
		heap_data h_data = jobs_.top();
		jobs_.pop();
		job_type job = std::move(h_data.myjob);
		//jobs_.erase(jobs_.begin() + idx);
		return job;
/*		job_type job = std::move(jobs_.front());
		jobs_.pop_front();
		return job;*/
	}
	typedef std::mutex mutex_t;
	typedef std::unique_lock<mutex_t> lock_t;
	std::condition_variable cond_var_;
	mutex_t mutex_;
	//std::deque<job_type> jobs_;
	fibonacci_heap<heap_data> jobs_;
	int unfinished_jobs_;
	int num_thread_;
	int num_jobs_;
};
