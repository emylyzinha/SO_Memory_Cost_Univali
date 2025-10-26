#include <iostream>
#include <chrono>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#ifdef _MSC_VER
#include <stdafx.h>
#endif

class Timer
{
public:
    Timer()
    {
        start = std::chrono::steady_clock::now();
    }
    // Returns the duration in seconds.
    double GetElapsed()
    {
        auto end = std::chrono::steady_clock::now();
        auto duration = end - start;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() * 1.e-9;
    }
private:
    std::chrono::steady_clock::time_point start;

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
};

void BusyWait(int ms)
{
    auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    for (;;)
    {
        if (std::chrono::steady_clock::now() > end)
            break;
    }
}

static void touch_memory(char *buf, size_t len, int touch_every_n_pages)
{
    size_t page = sysconf(_SC_PAGESIZE);
    if (page == 0) page = 4096;
    size_t stride = page * (size_t)(touch_every_n_pages > 0 ? touch_every_n_pages : 1);
    for (size_t off = 0; off < len; off += stride)
    {
        buf[off] ^= 1; // write to force page fault
    }
    if (len) buf[len - 1] ^= 1;
}

void worker_func(int id, int iterations, int touch_every_n_pages, int free_after_touch,
                 const std::vector<size_t>& sizes, std::atomic<bool>& stop_flag)
{
    std::mt19937_64 rng((unsigned)time(NULL) ^ (unsigned)(id * 1103515245));
    std::uniform_int_distribution<size_t> dist(0, sizes.size() - 1);

    for (int it = 0; it < iterations && !stop_flag.load(); ++it)
    {
        size_t sz = sizes[dist(rng)];
        char* p = nullptr;
        try {
            p = new char[sz];
        } catch (...) {
            fprintf(stderr, "thread %d: new(%zu) failed at iter %d\n", id, sz, it);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        touch_memory(p, sz, touch_every_n_pages);

        // optional short pause to increase concurrency and mixing
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(rng() % 50 + 1)));

        if (free_after_touch) {
            delete [] p;
        } else {
            // keep allocation to grow working set briefly
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(rng() % 500 + 100)));
            delete [] p;
        }
    }
}

void FastMeasureMultithreaded(int num_threads, int iterations, int touch_every_n_pages, int free_after_touch)
{
    printf("Starting multithreaded memory stress: threads=%d iterations=%d touch_every_n_pages=%d free_after_touch=%d\n",
           num_threads, iterations, touch_every_n_pages, free_after_touch);

    // preset sizes (bytes) - mix small and large allocations
    /*std::vector<size_t> sizes = {
        4 * 1024,           // 4 KB
        64 * 1024,          // 64 KB
        256 * 1024,         // 256 KB
        1 * 1024 * 1024,    // 1 MB
        4 * 1024 * 1024,    // 4 MB
        16 * 1024 * 1024,   // 16 MB
        64 * 1024 * 1024    // 64 MB
    };*/

    std::vector<size_t> sizes = {
        1 * 1024 * 1024,    // 1 MB
        64 * 1024 * 1024,   // 64 MB
        256 * 1024 * 1024,  // 256 MB
        512 * 1024 * 1024,  // 512 MB
        1024 * 1024 * 1024  // 1 GB
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    std::atomic<bool> stop_flag(false);

    Timer totalTimer;
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(worker_func, i, iterations, touch_every_n_pages, free_after_touch, std::cref(sizes), std::ref(stop_flag));
    }

    for (auto &t : threads) if (t.joinable()) t.join();

    printf("Done. Total time: %1.4f s\n", totalTimer.GetElapsed());
}

int main(int argc, char* argv[])
{
    // Default parameters
    int num_threads = 8;
    int iterations = 100;
    int touch_every_n_pages = 1;
    int free_after_touch = 1;

    if (argc > 1) num_threads = atoi(argv[1]);
    if (argc > 2) iterations = atoi(argv[2]);
    if (argc > 3) touch_every_n_pages = atoi(argv[3]);
    if (argc > 4) free_after_touch = atoi(argv[4]);

    if (num_threads <= 0) num_threads = 1;
    if (iterations <= 0) iterations = 1;
    if (touch_every_n_pages <= 0) touch_every_n_pages = 1;

    printf("Ramping CPU briefly...\n");
    BusyWait(500);

    FastMeasureMultithreaded(num_threads, iterations, touch_every_n_pages, free_after_touch);

    return 0;
}
