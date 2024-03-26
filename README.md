# TP_ID

This is an open-source implementation of a thread pool in C++ that allows tasks to be enqueued with a specific thread ID and provides a mechanism to wait for the completion of tasks associated with specific thread IDs to turn ANY C++ application into a multi-threaded application with ease, providing the application catastrophic performance improvements.

## How to use
Include the necessary headers:
   ```cpp
   #include <functional>
   #include <mutex>
   #include <deque>
   #include <unordered_set>
   ```

2. Create an instance of the `TP_ID` struct, specifying the desired number of worker threads is optional, by default the amount of threads is the max number of threads possible on your system:
   ```cpp
   // TP_ID threadPool(8);
   TP_ID threadPool;
   ```
   

3. Enqueue tasks using the `enqueue` function, provide the task as a lambda or a function object and specifying the thread ID:
   ```cpp
   int result = 0;
   threadPool.enqueue(/* Function: */ [&result] { result = 42; }, /* Thread ID: */ 1);
   ```

4. Run another `enqueue` function with the same thread ID if race conditions is a problem:
   ```cpp
   threadPool.enqueue([&result] { result = 75; }, 1);
   ```
5. Run another `enqueue` function with a different thread ID on a function that accesses a different object for proper parallelization:
   ```cpp
   int other = 2;
   threadPool.enqueue([&other] { other += 94; }, 2);
   ```
6.  Wait for the completion of tasks associated with specific thread IDs using the `wait` function:
   ```cpp
   threadPool.wait(1, 2);
   ```
6. The thread pool will automatically be destroyed when the `TP_ID` object goes out of scope, waiting for all worker threads to end to avoid errors.

## Example test cases:

```cpp
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
```

## Notes

- The number of worker threads is specified during the construction of the `TP_ID` object. If not provided, it defaults to the number of hardware threads available on the system.
- Tasks are enqueued using the `enqueue` function, which takes a lambda or a function object with a number, representing both the task to be executed and the thread ID assigned to that very task.
- The `wait` function is used to wait for the completion of tasks associated with specific thread IDs. It can take multiple thread IDs as arguments.
- The thread pool uses `std::deque` to store the tasks, `std::unordered_set` to keep track of active thread IDs, and `std::condition_variable` for thread synchronization to avoid runtime errors.
- The thread pool is thread-safe and can be used from multiple threads simultaneously.

Feel free to use and modify this code according to your needs.
