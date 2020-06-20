// This file is part of Poseidon.
// Copyleft 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "worker_pool.hpp"
#include "main_config.hpp"
#include "../core/abstract_async_job.hpp"
#include "../core/config_file.hpp"
#include "../xutilities.hpp"

namespace poseidon {
namespace {

size_t
do_get_size_config(const Config_File& file, const char* name, long max, size_t def)
  {
    const auto qval = file.get_int64_opt({"worker",name});
    if(!qval)
      return def;

    int64_t rval = ::rocket::clamp(*qval, 1, max);
    if(*qval != rval)
      POSEIDON_LOG_WARN("Config value `worker.poll.$1` truncated to `$2`\n"
                        "[value `$3` out of range]",
                        name, rval, *qval);

    return static_cast<size_t>(rval);
  }

struct Worker
  {
    ::rocket::once_flag init_once;
    ::pthread_t thread;

    mutex queue_mutex;
    condition_variable queue_avail;
    ::std::deque<rcptr<Abstract_Async_Job>> queue;
  };

}  // namespace

POSEIDON_STATIC_CLASS_DEFINE(Worker_Pool)
  {
    // constant data
    ::std::vector<Worker> m_workers;

    static
    void
    do_worker_init_once(Worker* qwrk)
      {
        size_t index = static_cast<size_t>(qwrk - self->m_workers.data());
        auto name = format_string("worker $1", index);
        POSEIDON_LOG_INFO("Creating new worker thread: $1", name);

        mutex::unique_lock lock(qwrk->queue_mutex);
        qwrk->thread = create_daemon_thread<do_worker_thread_loop>(name.c_str(), qwrk);
      }

    static
    void
    do_worker_thread_loop(void* param)
      {
        auto qwrk = static_cast<Worker*>(param);
        rcptr<Abstract_Async_Job> job;

        // Await a job and pop it.
        mutex::unique_lock lock(qwrk->queue_mutex);
        for(;;) {
          job.reset();
          if(qwrk->queue.empty()) {
            // Wait until an element becomes available.
            qwrk->queue_avail.wait(lock);
            continue;
          }

          // Pop it.
          job = ::std::move(qwrk->queue.front());
          qwrk->queue.pop_front();

          if(job.unique() && !job->resident()) {
            // Delete this job when no other reference of it exists.
            POSEIDON_LOG_DEBUG("Killed orphan asynchronous job: $1", job);
            continue;
          }

          // Use it.
          break;
        }
        lock.unlock();

        // Execute the job.
        ROCKET_ASSERT(job->state() == async_state_pending);
        job->do_set_state(async_state_running);
        POSEIDON_LOG_TRACE("Starting execution of asynchronous job `$1`", job);

        try {
          job->do_execute();
        }
        catch(exception& stdex) {
          POSEIDON_LOG_WARN("Caught an exception thrown from asynchronous job: $1\n"
                            "[job class `$2`]",
                            stdex.what(), typeid(*job).name());
        }

        ROCKET_ASSERT(job->state() == async_state_running);
        job->do_set_state(async_state_finished);
        POSEIDON_LOG_TRACE("Finished execution of asynchronous job `$1`", job);
      }
  };

void
Worker_Pool::
reload()
  {
    // Load worker settings into temporary objects.
    auto file = Main_Config::copy();

    size_t thread_count = do_get_size_config(file, "thread_count", 256, 1);

    // Create the pool without creating threads.
    // Note the pool cannot be resized, so we only have to do this once.
    // No locking is needed.
    if(self->m_workers.empty())
      self->m_workers = ::std::vector<Worker>(thread_count);
  }

size_t
Worker_Pool::
thread_count()
noexcept
  {
    return self->m_workers.size();
  }

rcptr<Abstract_Async_Job>
Worker_Pool::
insert(uptr<Abstract_Async_Job>&& ujob)
  {
    // Take ownership of `ujob`.
    rcptr<Abstract_Async_Job> job(ujob.release());
    if(!job)
      POSEIDON_THROW("Null job pointer not valid");

    if(!job.unique())
      POSEIDON_THROW("Job pointer must be unique");

    // Assign the job to a worker.
    if(self->m_workers.empty())
      POSEIDON_THROW("No worker available");

    auto qwrk = ::rocket::get_probing_origin(self->m_workers.data(),
                             self->m_workers.data() + self->m_workers.size(),
                             job->m_key);

    // Initialize the worker as necessary.
    qwrk->init_once.call(self->do_worker_init_once, qwrk);

    // Perform some initialization. No locking is needed here.
    job->do_set_state(async_state_pending);

    // Insert the job.
    mutex::unique_lock lock(qwrk->queue_mutex);
    qwrk->queue.emplace_back(job);
    qwrk->queue_avail.notify_one();
    return job;
  }

}  // namespace poseidon
