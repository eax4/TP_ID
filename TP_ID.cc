#include <functional>
#include <mutex>
#include <deque>
#include <unordered_set>
struct TP_ID
{
 explicit TP_ID(const uint tnum = std::thread::hardware_concurrency()) noexcept
 {
  threads_.reserve(tnum);
  thread_id_set_->reserve(tnum);
  for (uint_fast8_t i = 0; i < tnum; i++)
  {
   threads_.emplace_back([this]
    {
     std::function<void()> function;
     thread_id_type thread_id;
     while (true)
     {
      {
       std::unique_lock lock(m_);
       condition_variable_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
       if (stop_ && tasks_.empty()) return;
       thread_id = tasks_.front().first;
       function = std::move(tasks_.front().second);
       tasks_.pop_front();
      }
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
  for (auto& thread : threads_)
   thread.join();
 }
 template<typename type>
 void enqueue(type&& function, const int_fast8_t thread_id) noexcept
 {
  {
   std::unique_lock lock(m_);
   condition_variable2_.wait(lock, [this, &thread_id] {return !thread_id_set_->contains(thread_id); });
   tasks_.emplace_back(std::make_pair(thread_id, std::forward<type>(function)));
  }
  thread_id_set_->emplace(thread_id);
  condition_variable_.notify_one();
 }
 template<class ... thread_id_list>
 void wait(const thread_id_list&& ...thread_ids) noexcept
 {
  std::unique_lock lock(m_);
  condition_variable2_.wait(lock, [this, &thread_ids...] {return (!thread_id_set_->contains(thread_ids) && ...); });
 }
private:
 typedef int_fast8_t thread_id_type;
 std::deque<std::pair<thread_id_type, std::function<void()>>> tasks_;
 std::vector<std::thread> threads_;
 std::condition_variable condition_variable2_;
 bool stop_ = false;
 std::shared_ptr<std::unordered_set<thread_id_type>> thread_id_set_ = std::make_shared<std::unordered_set<thread_id_type>>();
 std::mutex m_;
 std::condition_variable condition_variable_;
};

// Test Cases
int main()
{
 std::ios::sync_with_stdio(false);
 // Enqueues a task and waits for its completion
 TP_ID threadPool;
 int result = 0;
 threadPool.enqueue([&result]() {
  result = 42;
  }, 1);
 threadPool.wait(1);
 std::cout << (result == 42) << '\n';

 // Enqueues multiple tasks and waits for their completion
 int result1 = 0;
 int result2 = 0;
 threadPool.enqueue([&result1]() {
  result1 = 10;
  }, 1);
 threadPool.enqueue([&result2]() {
  result2 = 20;
  }, 2);
 threadPool.wait(1, 2);
 std::cout << (result1 == 10) << '\n';
 std::cout << (result2 == 20) << '\n';

 // Enqueues tasks from multiple threads and waits for their completion
 result1 = 0;
 result2 = 0;
 std::thread thread1([&]() {
  threadPool.enqueue([&result1]() {
   result1 = 100;
   }, 1);
  });
 std::thread thread2([&]() {
  threadPool.enqueue([&result2]() {
   result2 = 200;
   }, 2);
  });
 thread1.join();
 thread2.join();
 threadPool.wait(1, 2);
 std::cout << (result1 == 100) << '\n';
 std::cout << (result2 == 200) << '\n';
 return 0;
}
