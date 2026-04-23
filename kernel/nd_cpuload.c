#include "nerd.h"
#include "nd_internal.h"
#include "nd_lock.h"

static nd_uint32_t k_cpuload;
static nd_uint64_t k_last_total_time;
static nd_uint64_t k_last_idle_time;

static nd_uint64_t nd_thread_runtime_get(nd_thread_t *thread)
{
    nd_uint64_t runtime = thread->usage.total;

    if (thread == nd_current_thread) {
        runtime += nd_hw_get_current() - last_switch_time;
    }

    return runtime;
}

static void nd_cpuload_update(void)
{
    nd_kernel_def();
    nd_kernel_lock();

    nd_uint64_t now = nd_hw_get_current();
    nd_uint64_t idle_runtime = nd_idle_runtime_get();
    nd_uint64_t total_delta = now - k_last_total_time;
    nd_uint64_t idle_delta = idle_runtime - k_last_idle_time;

    nd_thread_t *thread;

    nd_list_t *list_head = nd_thread_list_get();

    nd_list_for_each_entry(thread, list_head, tlist) {
        nd_uint64_t runtime = nd_thread_runtime_get(thread);

        thread->usage.load =
            (nd_uint32_t)(((runtime - thread->usage.last_total) * 100ULL) / total_delta);
        thread->usage.last_total = runtime;
    }

    k_cpuload = (nd_uint32_t)(100ULL - (idle_delta * 100ULL) / total_delta);
    k_last_total_time = now;
    k_last_idle_time = idle_runtime;

    nd_kernel_unlock();
}

nd_uint32_t nd_cpuload_get(void)
{
    nd_cpuload_update();
    return k_cpuload;
}

nd_uint32_t nd_cpuload_thread_get(nd_thread_t *thread)
{
    return thread->usage.load;
}
