// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include "muduo/base/BlockingQueue.h"
#include "muduo/base/BoundedBlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/LogStream.h"

#include <atomic>
#include <vector>

namespace muduo
{

class AsyncLogging : noncopyable
{
 public:

  AsyncLogging(const string& basename,
               off_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() NO_THREAD_SAFETY_ANALYSIS
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  void threadFunc();    // Alkaid 供后端消费者调用（将数据写到日志文件）

  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;   // Alkaid buffer的智能指针unique_ptr，能管理buffer的生存期

  const int flushInterval_;                     // Alkaid 在flushInterval_秒内，缓冲区即使没有写满，扔将缓冲区中的数据写入文件
  std::atomic<bool> running_;
  const string basename_;
  const off_t rollSize_;
  muduo::Thread thread_;
  muduo::CountDownLatch latch_;                 // Alkaid 用于等待线程启动
  muduo::MutexLock mutex_;
  muduo::Condition cond_ GUARDED_BY(mutex_);
  BufferPtr currentBuffer_ GUARDED_BY(mutex_);  // Alkaid 当前缓冲区
  BufferPtr nextBuffer_ GUARDED_BY(mutex_);     // Alkaid 预备缓冲区
  BufferVector buffers_ GUARDED_BY(mutex_);     // Alkaid 待写入文件的已填满缓冲区
};

}  // namespace muduo

#endif  // MUDUO_BASE_ASYNCLOGGING_H
