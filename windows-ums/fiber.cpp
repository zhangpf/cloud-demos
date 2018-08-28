#include <deque>
#include <iostream>
#include <atomic>
#include <windows.h>

int num_fibers = -1;
int num_yields = -1;

std::atomic<int> counter = 0;
HANDLE finished_counting_event;
LPVOID primary_fiber = NULL;
BOOL terminated = FALSE; 

VOID WINAPI UserThreadFunction(void *parameter) {
    int thread_number = (int) (ULONG_PTR) parameter;
    for (int i = 0; i < num_yields; i ++) {
        terminated = FALSE;
        SwitchToFiber(primary_fiber);
    }
    counter++;
    if (counter == num_fibers) {
        SetEvent(finished_counting_event);
    }
    terminated = TRUE;
    SwitchToFiber(primary_fiber);
}

int main(int argc, char* argv[]) {
    if (argc != 3) { // We expect 3 arguments: the program name, number of threads and number of yields
        std::cerr << "Usage: " << argv[0] << " <number-of-fibers> <num-of-yields>" << std::endl;
        return 1;
    }

    num_fibers = atoi(argv[1]);
    num_yields = atoi(argv[2]);

    finished_counting_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    primary_fiber = ConvertThreadToFiber(&terminated);
    std::deque<PUMS_CONTEXT> ready_queue;

    HANDLE *user_fibers = new HANDLE[num_fibers];
    for (int i = 0; i < num_fibers; i++) {
        user_fibers[i] = CreateFiber(0, UserThreadFunction, (void *)(ULONG_PTR) i);
        ready_queue.push_back(user_fibers[i]);
    }
    
    LARGE_INTEGER timeStart, timeEnd, frequency;

    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;

    QueryPerformanceCounter(&timeStart);

    while (!ready_queue.empty()) {
        HANDLE runnable_fiber = ready_queue.front();
        ready_queue.pop_front();
        SwitchToFiber(runnable_fiber);
        BOOL terminated = *((PBOOL)GetFiberData());
        if (!terminated) {
            ready_queue.push_back(runnable_fiber);
        }
    }

    QueryPerformanceCounter(&timeEnd);

    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;

    std::cout << "Duration: " << elapsed << 's.' << std::endl;
    std::cout << "Average execution time: " <<  
        (int)(elapsed / num_fibers / num_yields * 1e9) << "ns." << std::endl;

    for (int i = 0; i < num_fibers; i++) {
        WaitForSingleObject(user_fibers[i], INFINITE);
    }

    delete user_fibers;

    return 0;
}
