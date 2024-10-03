#include <functional>
#include <mutex>
#include <deque>
#include <unordered_set>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <chrono>

template<typename T>
struct atomic_deque
{
	explicit	atomic_deque() noexcept = default;
	bool empty() noexcept
	{
		return !head_.load(std::memory_order_consume);
	}
	T&& pop_and_front() noexcept
	{
		node* oldhead = head_.load(std::memory_order_consume);
		head_.store(oldhead->next, std::memory_order_release);
		return std::move(oldhead->data);
	}
	void emplace_back(T&& data) noexcept
	{
		std::this_thread::sleep_for(std::chrono::microseconds(1));
		node* newnode = new node(std::forward<T>(data));
		if (!head_.load(std::memory_order_relaxed))
		{
			head_.store(newnode, std::memory_order_release);
			tail_.store(newnode->next, std::memory_order_relaxed);
		}
		else
		{
			tail_.store(newnode, std::memory_order_relaxed);
			tail_.store(newnode->next, std::memory_order_relaxed);
		}
		newnode = nullptr;
	}

private:
	struct node
	{
		explicit node(T&& data) noexcept : data(std::forward<T>(data)), next(nullptr)
		{}
		~node() noexcept
		{
			next.store(nullptr, std::memory_order_release);
			free(this);
		}
		T data;
		std::atomic<node*> next;
	};

	std::atomic<node*> head_;
	std::atomic<node*> tail_;
};

struct TP_ID
{
	explicit TP_ID(const uint_fast8_t tnum = std::thread::hardware_concurrency()) noexcept
	{
		threads_.reserve(tnum);
		thread_id_set_->reserve(tnum);
		for (uint_fast8_t i = 0; i < tnum; i++)
		{
			threads_.emplace_back([this]
				{
					while (true)
					{
						{
							std::unique_lock lock(m_);
							condition_variable_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
						}

						if (stop_ && tasks_.empty()) return;
						auto [thread_id, function] = tasks_.pop_and_front();

						function();
						thread_id_set_->erase(thread_id);
						condition_variable2_.notify_one();
					}
				});
		}
	}
	~TP_ID() noexcept
	{
		stop_ = true;
		condition_variable_.notify_all();
		if (thread_id_set_ != nullptr)
			for (const thread_id_type thread_id : *thread_id_set_)
				wait(thread_id);
		for (auto& thread : threads_)
			thread.join();
	}
	template<typename Type>
	void enqueue(Type function, const int_fast8_t thread_id) noexcept
	{
		{
			std::unique_lock lock(m_);
			condition_variable2_.wait(lock, [this, thread_id] { return !thread_id_set_->contains(thread_id); });
		}
		tasks_.emplace_back(std::make_pair(thread_id, std::forward<Type>(function)));
		condition_variable_.notify_one();
		thread_id_set_->emplace(thread_id);
	}
	template<typename... ThreadIdList, typename = int_fast8_t>
	void wait(ThreadIdList ...thread_ids) noexcept
	{
		std::unique_lock lock(m2_);
		condition_variable2_.wait(lock, [this, thread_ids...] { return (!thread_id_set_->contains(thread_ids) && ...); });
	}
private:
	typedef int_fast8_t thread_id_type;
	atomic_deque<std::pair<thread_id_type, std::function<void()>>> tasks_;
	std::vector<std::thread> threads_;
	std::condition_variable condition_variable2_;

	bool stop_ = false;
	std::shared_ptr<std::unordered_set<thread_id_type>> thread_id_set_ = std::make_shared<std::unordered_set<thread_id_type>>();
	std::mutex m_;
	std::mutex m2_;
	std::condition_variable condition_variable_;
};

int main()
{
	std::ios::sync_with_stdio(false);
	TP_ID thread_pool;

	// Enqueues a task and waits for its completion
	int result = 0;
	thread_pool.enqueue([&result] ()
		{
			result = 42;
		}, 10);
	thread_pool.wait(10);
	std::cout << (result == 42) << '\n';

	// Enqueues multiple tasks and waits for their completion
	int result1 = 0;
	int result2 = 0;
	thread_pool.enqueue([&result1] ()
		{
			result1 = 10;
		}, 1);
	thread_pool.enqueue([&result2] ()
		{
			result2 = 20;
		}, 2);
	thread_pool.wait(1, 2);
	std::cout << (result1 == 10) << '\n';
	std::cout << (result2 == 20) << '\n';

	// Enqueues tasks from multiple threads and waits for their completion
	result1 = 0;
	result2 = 0;
	std::thread thread1([&] ()
		{
			thread_pool.enqueue([&result1] ()
				{
					result1 = 100;
				}, 1);
		});
	std::thread thread2([&] ()
		{
			thread_pool.enqueue([&result2] ()
				{
					result2 = 200;
				}, 2);
		});
	thread1.join();
	thread2.join();
	thread_pool.wait(1, 2);
	std::cout << (result1 == 100) << '\n';
	std::cout << (result2 == 200) << '\n';
	return 0;
}
