/*********************************************************
          File Name:thread_local.cpp
          Author: Abby Cin
          Mail: abbytsing@gmail.com
          Created Time: Thu 11 Aug 2016 07:34:25 PM CST
**********************************************************/

#include <iostream>
#include "threadpool.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

pid_t gettid()
{
  return static_cast<pid_t>(syscall(SYS_gettid));
}

#ifdef THREAD_LOCAL
#pragma message "`thread_local`"
thread_local int data = 2;
#else
#pragma message "`__thread`"
__thread int data = 2;
#endif

int main()
{
    // int data = 2;
  std::mutex mtx;
  auto func = [&](const int x)
  {
    std::lock_guard<std::mutex> lck(mtx);
    data *= x;
    fprintf(stderr, "tid: %d\t data: %d\n", gettid(), data);
  };
  ThreadPool pool(4);
  pool.enqueue(func, 2);
  pool.enqueue(func, 4);
  pool.enqueue(func, 5);
  pool.enqueue(func, 6);
  fprintf(stderr, "main: %d\t data: %d\n", gettid(), data);
}