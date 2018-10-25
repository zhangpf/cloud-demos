#include <deque>
#include <iostream>
#include <atomic>
#include <windows.h>

HANDLE CreateUserThread(SIZE_T stack_size, LPTHREAD_START_ROUTINE function, void *parameter) {
    SIZE_T attribute_list_size = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attribute_list_size);
    SetLastError(0);
    PPROC_THREAD_ATTRIBUTE_LIST attribute_list = (PPROC_THREAD_ATTRIBUTE_LIST) HeapAlloc(GetProcessHeap(), 0, attribute_list_size);
    InitializeProcThreadAttributeList(attribute_list, 1, 0, &attribute_list_size);

    HANDLE thread = CreateRemoteThreadEx(GetCurrentProcess(), NULL, stack_size, function, parameter, STACK_SIZE_PARAM_IS_A_RESERVATION, attribute_list, NULL);

    DeleteProcThreadAttributeList(attribute_list);
    HeapFree(GetProcessHeap(), 0, attribute_list);

    return thread;
}

int num_threads = -1;
int num_yields = -1;

std::atomic<int> counter = 0;
HANDLE finished_counting_event;

DWORD WINAPI UserThreadFunction(void *parameter) {
    int thread_number = (int) (ULONG_PTR) parameter;
    for (int i = 0; i < num_yields; i ++) {
        SwitchToThread();
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

    finished_counting_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    HANDLE *user_threads = new HANDLE[num_threads];
    for (int i = 0; i < num_threads; i++) {
        user_threads[i] = CreateUserThread(0, UserThreadFunction, (void *)(ULONG_PTR) i);
    }

    
    LARGE_INTEGER timeStart, timeEnd, frequency;

    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;

    QueryPerformanceCounter(&timeStart);

    WaitForSingleObject(finished_counting_event, INFINITE);

    QueryPerformanceCounter(&timeEnd);

    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;

    std::cout << "Duration: " << elapsed << 's.' << std::endl;
    std::cout << "Average execution time: " <<  
        (int)(elapsed / num_threads / num_yields * 1e9) << "ns." << std::endl;

    for (int i = 0; i < num_threads; i++) {
        WaitForSingleObject(user_threads[i], INFINITE);
    }

    delete user_threads;

    return 0;
}
