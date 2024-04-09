#include <functional>
#include <mutex>
#include <deque>
#include <unordered_set>

struct TP_ID
{
 explicit TP_ID(const uint&& tnum = std::thread::hardware_concurrency()) noexcept
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
  if (thread_id_set_ != nullptr)
   for (const thread_id_type thread_id : *thread_id_set_)
    wait(std::move(thread_id));
  for (auto& thread : threads_)
   thread.join();
 }
 template<typename type>
 void enqueue(type&& function, const int_fast8_t&& thread_id) noexcept
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
  std::unique_lock lock(m2_);
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
 std::mutex m2_;
 std::condition_variable condition_variable_;
};
