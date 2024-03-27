# task_queue

一个简单的消息/任务队列，基于C++11实现，用法如下：

A simple message/task queue implemented in C++11, usage is as follows:

```c++

TaskQueue task_queue;
 
 int value = 0;
 
 int n = task_queue.invoke<int>([&]{
    printf("test\n");
    value += 10;
    return 1;
 });
 
 assert(n == 1);
 assert(value == 10);
 
 std::string str = task_queue.invoke<int>([&]{
    return "123";
 });
 
 assert(str == "123");
 
 task_queue.post([&]{
    printf("test2\n");
 });
 
 task_queue.post_delayed([&]{
    printf("test3\n");
 }, 1000);

```
