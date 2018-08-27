#include <deque>
#include <atomic>
#include <iostream>
#include <windows.h>

HANDLE scheduler_thread;
PUMS_COMPLETION_LIST scheduler_completion_list;
HANDLE scheduler_completion_event;
HANDLE scheduler_initialized_event;
HANDLE scheduler_shutdown_event;

std::deque<PUMS_CONTEXT> ready_queue;

void WINAPI SchedulerCallback(UMS_SCHEDULER_REASON reason, ULONG_PTR payload, void *parameter) {
    switch (reason) {
        case UmsSchedulerStartup:
            SetEvent(scheduler_initialized_event);
            break;
        case UmsSchedulerThreadBlocked: {
            break;
        }
        case UmsSchedulerThreadYield: {
            PUMS_CONTEXT yielded_thread = (PUMS_CONTEXT) payload;
            void *yielded_parameter = parameter;
            ready_queue.push_back(yielded_thread);
            break;
        }
    }

    for (;;) {
        while (ready_queue.size() > 0) {
            PUMS_CONTEXT runnable_thread = ready_queue.front();
            ready_queue.pop_front();

            BOOLEAN terminated = FALSE;
            ExecuteUmsThread(runnable_thread);
        }

        HANDLE handles[2] = {scheduler_shutdown_event, scheduler_completion_event};
        DWORD handle_index = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        if (handle_index == 0) {
            TerminateThread(GetCurrentThread(), 0);
        } else if (handle_index == 1) {
            PUMS_CONTEXT unblocked_thread = NULL;
            if (DequeueUmsCompletionListItems(scheduler_completion_list, 0, &unblocked_thread)) {
                while (unblocked_thread) {
                    ready_queue.push_back(unblocked_thread);
                    unblocked_thread = GetNextUmsListItem(unblocked_thread);
                }
            }
        }
    }
}

DWORD WINAPI SchedulerThreadFunction(void *parameter) {
    UMS_SCHEDULER_STARTUP_INFO scheduler_info;
    scheduler_info.UmsVersion = UMS_VERSION;
    scheduler_info.CompletionList = scheduler_completion_list;
    scheduler_info.SchedulerProc = SchedulerCallback;
    scheduler_info.SchedulerParam = NULL;
    BOOL result = EnterUmsSchedulingMode(&scheduler_info);
    return 0;
}

void InitializeScheduler() {
    scheduler_initialized_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    scheduler_shutdown_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    CreateUmsCompletionList(&scheduler_completion_list);
    GetUmsCompletionListEvent(scheduler_completion_list, &scheduler_completion_event);

}

void StartScheduler() {
    scheduler_thread = CreateThread(NULL, 0, SchedulerThreadFunction, NULL, 0, NULL);
    WaitForSingleObject(scheduler_initialized_event, INFINITE);
}

void StopScheduler() {
    SetEvent(scheduler_shutdown_event);
    WaitForSingleObject(scheduler_thread, INFINITE);
 
    DeleteUmsCompletionList(scheduler_completion_list);
}

HANDLE CreateUserThread(SIZE_T stack_size, LPTHREAD_START_ROUTINE function, void *parameter) {
    PUMS_CONTEXT ums_context;
    if (!CreateUmsThreadContext(&ums_context)) {
        return INVALID_HANDLE_VALUE;
    }

    SIZE_T attribute_list_size = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attribute_list_size);
    SetLastError(0);
    PPROC_THREAD_ATTRIBUTE_LIST attribute_list = (PPROC_THREAD_ATTRIBUTE_LIST) HeapAlloc(GetProcessHeap(), 0, attribute_list_size);
    InitializeProcThreadAttributeList(attribute_list, 1, 0, &attribute_list_size);

    UMS_CREATE_THREAD_ATTRIBUTES ums_thread_attributes;
    ums_thread_attributes.UmsVersion = UMS_VERSION;
    ums_thread_attributes.UmsContext = ums_context;
    ums_thread_attributes.UmsCompletionList = scheduler_completion_list;
    UpdateProcThreadAttribute(attribute_list, 0, PROC_THREAD_ATTRIBUTE_UMS_THREAD, &ums_thread_attributes, sizeof(ums_thread_attributes), NULL, NULL);

    HANDLE thread = CreateRemoteThreadEx(GetCurrentProcess(), NULL, stack_size, function, parameter, STACK_SIZE_PARAM_IS_A_RESERVATION, attribute_list, NULL);

    DeleteProcThreadAttributeList(attribute_list);
    HeapFree(GetProcessHeap(), 0, attribute_list);

    return thread;
}

// Example usage

int num_threads = -1;
int num_yields = -1;
std::atomic<int> counter = 0;
HANDLE finished_counting_event;

DWORD WINAPI UserThreadFunction(void *parameter) {
    int thread_number = (int) (ULONG_PTR) parameter;
    for (int i = 0; i < num_yields; i ++) {
        UmsThreadYield(0);
    }
    counter++;
    if (counter == num_threads) {
        SetEvent(finished_counting_event);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) { // We expect 3 arguments: the program name, number of threads and number of yields
        std::cerr << "Usage: " << argv[0] << " <number-of-threads> <num-of-yields>" << std::endl;
        return 1;
    }

    num_threads = atoi(argv[1]);
    num_yields = atoi(argv[2]);

    InitializeScheduler();

    finished_counting_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    HANDLE *user_threads = new HANDLE[num_threads];
    for (int i = 0; i < num_threads; i++) {
        user_threads[i] = CreateUserThread(0, UserThreadFunction, (void *)(ULONG_PTR) i);
    }

    StartScheduler();
    LARGE_INTEGER timeStart, timeEnd, frequency;

    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;

    QueryPerformanceCounter(&timeStart);

    WaitForSingleObject(finished_counting_event, INFINITE);

    QueryPerformanceCounter(&timeEnd);

    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;

    printf("Duration: %fs.\n", elapsed);
    printf("Average execution time: %dns.\n", 
        (int)(elapsed / num_threads / num_yields * 1e9));

    for (int i = 0; i < num_threads; i++) {
        WaitForSingleObject(user_threads[i], INFINITE);
    }

    delete user_threads;

    StopScheduler();

    return 0;
}
