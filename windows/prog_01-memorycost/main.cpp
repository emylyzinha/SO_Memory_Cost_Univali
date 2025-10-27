// main.cpp — stress de memória portátil + progresso no terminal
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include <atomic>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>   // sysconf
#endif

// ===== util =====
class Timer {
public:
  Timer(): t0(std::chrono::steady_clock::now()) {}
  double elapsed() const {
    auto d = std::chrono::steady_clock::now() - t0;
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count()/1000.0;
  }
  std::chrono::steady_clock::time_point t0;
};

static void busy_wait_ms(int ms){
  auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  while (std::chrono::steady_clock::now() < end) {}
}

// ===== memory touching =====
static void touch_memory(char* buf, size_t len, int touch_every_n_pages){
  size_t page = 4096;
#ifdef _WIN32
  SYSTEM_INFO si; GetSystemInfo(&si);
  page = si.dwPageSize ? si.dwPageSize : 4096;
#else
  long p = sysconf(_SC_PAGESIZE);
  if (p > 0) page = (size_t)p;
#endif
  if (touch_every_n_pages <= 0) touch_every_n_pages = 1;
  size_t stride = page * (size_t)touch_every_n_pages;

  for (size_t off = 0; off < len; off += stride) buf[off] ^= 1; // força page-fault (escrita)
  if (len) buf[len-1] ^= 1;
}

// ===== worker =====
static void worker_func(
  int id, int iterations, int touch_every_n_pages, int free_after_touch,
  const std::vector<size_t>& sizes, const Timer& globalT, std::atomic<bool>& stop_flag)
{
  std::mt19937_64 rng((unsigned)std::time(nullptr) ^ (unsigned)(id * 1103515245u));
  std::uniform_int_distribution<size_t> pick(0, sizes.size()-1);

  for (int it = 0; it < iterations && !stop_flag.load(); ++it){
    const size_t sz = sizes[pick(rng)];
    char* p = nullptr;
    try { p = new char[sz]; }
    catch (...) {
      std::fprintf(stderr, "thread %d: new(%zu) falhou (iter=%d)\n", id, sz, it);
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      continue;
    }

    touch_memory(p, sz, touch_every_n_pages);

    // mostra progresso a cada 10 iterações (somente a thread 0, para evitar “spam”)
    if (id == 0 && (it % 10 == 0)) {
      std::printf("[t=%.2fs] progresso: iter=%d/%d (thr0)\n", globalT.elapsed(), it, iterations);
      std::fflush(stdout);
    }

    // pequena espera para misturar alocações entre threads
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(rng()%50 + 1)));

    if (!free_after_touch) {
      // mantém um pouco para inflar working-set
      std::this_thread::sleep_for(std::chrono::milliseconds((int)(rng()%400 + 100)));
    }
    delete[] p;
  }
}

// ===== driver =====
static void FastMeasureMultithreaded(int num_threads, int iterations, int touch_every_n_pages, int free_after_touch){
  std::printf("Starting stress: threads=%d iterations=%d touch_n_pages=%d free_after=%d\n",
              num_threads, iterations, touch_every_n_pages, free_after_touch);

  // tamanhos moderados (ajuste se tiver pouca RAM/pagefile)
  std::vector<size_t> sizes = {
      1ULL  * 1024 * 1024,   // 1 MB
      32ULL * 1024 * 1024,   // 32 MB
      64ULL * 1024 * 1024,   // 64 MB
      128ULL* 1024 * 1024    // 128 MB
      // adicione 256MB/512MB/1GB se tiver folga
  };

  std::vector<std::thread> pool;
  pool.reserve(num_threads);
  std::atomic<bool> stop_flag(false);
  Timer total;

  for (int i = 0; i < num_threads; ++i){
    pool.emplace_back(worker_func, i, iterations, touch_every_n_pages,
                      free_after_touch, std::cref(sizes), std::cref(total), std::ref(stop_flag));
  }
  for (auto& t : pool) if (t.joinable()) t.join();

  std::printf("Done. Total time: %.4f s\n", total.elapsed());
}

int main(int argc, char* argv[]){
  int num_threads = 8, iterations = 100, touch_every_n_pages = 1, free_after_touch = 1;
  if (argc > 1) num_threads = std::atoi(argv[1]);
  if (argc > 2) iterations = std::atoi(argv[2]);
  if (argc > 3) touch_every_n_pages = std::atoi(argv[3]);
  if (argc > 4) free_after_touch = std::atoi(argv[4]);

  if (num_threads <= 0) num_threads = 1;
  if (iterations  <= 0) iterations  = 1;
  if (touch_every_n_pages <= 0) touch_every_n_pages = 1;

  std::puts("Ramping CPU briefly...");
  busy_wait_ms(300);
  FastMeasureMultithreaded(num_threads, iterations, touch_every_n_pages, free_after_touch);
  return 0;
}
