

#include "system_threading.h"
#include "system_task.h"
#include <time.h>
#include <string.h>


#if PLATFORM_THREADING
void system_thread_idle()
{
    Spark_Idle_Events(true);
}

ActiveObjectThreadQueue SystemThread(ActiveObjectConfiguration(system_thread_idle, 100, 1024*3));

/**
 * Implementation to support gthread's concurrency primitives.
 */
namespace std {

#if 0
	/**
	 * Imlementation of conditional varaible in terms of the HAL. This has
	 * a potential race condition due to the placement of the critical section,
	 * so has been commented out. https://github.com/spark/firmware/pull/614#discussion-diff-39882530
	 */
    condition_variable::~condition_variable()
    {
        os_condition_variable_destroy(_M_cond);
    }

    condition_variable::condition_variable()
    {
        os_condition_variable_create(&_M_cond);
    }

    void condition_variable::wait(unique_lock<mutex>& lock)
    {
        os_condition_variable_wait(_M_cond, &lock);
    }

    void condition_variable::notify_one()
    {
        os_condition_variable_notify_one(_M_cond);
    }

    void condition_variable::notify_all()
    {
        os_condition_variable_notify_all(_M_cond);
    }
#endif

    mutex& __get_once_mutex() {
        static mutex __once;
        return __once;
    }

    function<void()> __once_functor;

    __future_base::_Result_base::_Result_base() = default;
    __future_base::_Result_base::~_Result_base() = default;

     #if __GNUC__ == 4 && __GNUC_MINOR__ == 8
    __future_base::_State_base::~_State_base() = default;
    #endif

    struct thread_startup
    {
    	thread::_Impl_base* call;
    	volatile bool started;
    };

    /**
     * static Startup function for threads.
     * @param ptr   A pointer to the _Impl_base value which exposes the virtual
     *  run() method.
     */
    void invoke_thread(void* ptr)
    {
        thread_startup* startup = (thread_startup*)ptr;
        thread::__shared_base_type local(startup->call);
        thread::_Impl_base* call = (thread::_Impl_base*)local.get();
        startup->started = true;
        call->_M_run();
    }

    void thread::_M_start_thread(thread::__shared_base_type base)
    {
        thread_startup startup;
        startup.call = base.get();
        startup.started = false;
        // FIXME: if the priority of the new thread is low enough not to cause `os_thread_create` to
        // preempt the current thread to run the thread start function, by the time `invoke_thread`
        // executes `call->_M_run()` will cause a pure virtual error
        if (os_thread_create(&_M_id._M_thread, "std::thread", OS_THREAD_PRIORITY_DEFAULT, invoke_thread, &startup, 1024*3)) {
            PANIC(AssertionFailure, "%s %s", __FILE__, __LINE__);
        }
        else {  // C++ ensure the thread has started execution, as required by the standard
            while (!startup.started) os_thread_yield();
        }
    }

    inline std::unique_lock<std::mutex>*& __get_once_functor_lock_ptr()
    {
      static std::unique_lock<std::mutex>* __once_functor_lock_ptr = 0;
      return __once_functor_lock_ptr;
    }

    void __set_once_functor_lock_ptr(unique_lock<mutex>* __ptr)
    {
        __get_once_functor_lock_ptr() = __ptr;
    }
}

#endif


void* system_internal(int item, void* reserved)
{
    switch (item) {
#if PLATFORM_THREADING
    case 0: return &ApplicationThread;
    case 1: return &SystemThread;
#endif
    }
    return nullptr;
}
