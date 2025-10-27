#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <ctime>

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
  #include <direct.h>
  #define mkdir(p,m) _mkdir(p)
#else
  #include <sys/stat.h>
  #include <unistd.h>
#endif

#include "pgm_utils.h"
#include "filters.h"

struct Task { int rs, re; };
static const Task SENT = {-1,-1};
#define QMAX 128
static Task Q[QMAX]; static int Qh=0, Qt=0;
static sem_t Sitems, Sspace, Slock;

static void qpush(Task t){ sem_wait(&Sspace); sem_wait(&Slock); Q[Qt]=t; Qt=(Qt+1)%QMAX; sem_post(&Slock); sem_post(&Sitems); }
static Task qpop(){ sem_wait(&Sitems); sem_wait(&Slock); Task t=Q[Qh]; Qh=(Qh+1)%QMAX; sem_post(&Slock); sem_post(&Sspace); return t; }

static PGM inImg, outImg;
static int g_mode=0, g_t1=0, g_t2=0, g_nthr=4;
static const int ROWS=64;

static bool read_exact(void* dst, size_t n){
  size_t got = 0; unsigned char* p = (unsigned char*)dst;
  while (got < n){
    size_t r = std::fread(p+got, 1, n-got, stdin);
    if (r == 0) return false;
    got += r;
  }
  return true;
}

static void* thr(void*){
  for(;;){
    Task t = qpop();
    if (t.rs < 0) break;
    if (g_mode==0) apply_negative(&inImg, &outImg, t.rs, t.re);
    else           apply_slice   (&inImg, &outImg, t.rs, t.re, g_t1, g_t2);
  }
  return nullptr;
}

int main(int argc, char** argv){
  // Uso: worker <saida.pgm> <negativo|slice> [t1 t2] [nthreads]
  if (argc < 3){
    std::fprintf(stderr, "Uso: %s <saida.pgm> <negativo|slice> [t1 t2] [nthreads]\n", argv[0]);
    return 1;
  }
  const char* outPath = argv[1];
  const char* mode    = argv[2];

  if (!std::strcmp(mode,"negativo")){
    g_mode = 0;
    if (argc >= 4) g_nthr = std::atoi(argv[3]);
  } else if (!std::strcmp(mode,"slice")){
    if (argc < 5){ std::fprintf(stderr, "Para slice: %s <saida.pgm> slice <t1> <t2> [nthreads]\n", argv[0]); return 1; }
    g_mode = 1; g_t1 = std::atoi(argv[3]); g_t2 = std::atoi(argv[4]);
    if (argc >= 6) g_nthr = std::atoi(argv[5]);
  } else {
    std::fprintf(stderr, "Modo invalido! Use negativo|slice\n");
    return 1;
  }

#ifdef _WIN32
  _setmode(_fileno(stdin),  _O_BINARY);
#endif

  // Lê metadados do STDIN (exatamente 3 inteiros)
  if (!read_exact(&inImg.w, sizeof(inImg.w)) ||
      !read_exact(&inImg.h, sizeof(inImg.h)) ||
      !read_exact(&inImg.maxv, sizeof(inImg.maxv))){
    std::fprintf(stderr, "Erro ao ler metadados do STDIN\n");
    return 1;
  }
  std::fprintf(stderr, "Worker: metadados -> w=%d h=%d maxv=%d\n", inImg.w, inImg.h, inImg.maxv);

  outImg.w = inImg.w; outImg.h = inImg.h; outImg.maxv = inImg.maxv;
  size_t total = (size_t)inImg.w * inImg.h;

  inImg.data  = (unsigned char*)std::malloc(total);
  outImg.data = (unsigned char*)std::malloc(total);
  if (!inImg.data || !outImg.data){
    std::fprintf(stderr, "Falha ao alocar memoria\n");
    return 1;
  }
  if (!read_exact(inImg.data, total)){
    std::fprintf(stderr, "Erro ao ler pixels do STDIN\n");
    return 1;
  }
  std::fprintf(stderr, "Worker: recebimento concluido. Processando com %d threads...\n", g_nthr);

  timespec a{}, b{}; clock_gettime(CLOCK_MONOTONIC, &a);

  sem_init(&Sitems,0,0); sem_init(&Sspace,0,QMAX); sem_init(&Slock,0,1);
  std::vector<pthread_t> th(g_nthr);
  for (int i=0;i<g_nthr;++i) pthread_create(&th[i],nullptr,thr,nullptr);

  for (int rs=0; rs<inImg.h; rs+=ROWS) qpush({rs, std::min(rs+ROWS, inImg.h)});
  for (int i=0;i<g_nthr;++i) qpush(SENT);
  for (auto& t: th) pthread_join(t,nullptr);

  clock_gettime(CLOCK_MONOTONIC, &b);
  double secs = (b.tv_sec-a.tv_sec) + (b.tv_nsec-a.tv_nsec)/1e9;
  std::fprintf(stderr, "Tempo de processamento paralelo: %.6f s\n", secs);

  mkdir("out", 0755);
  std::string base = outPath;
  size_t p = base.find_last_of("/\\"); if (p!=std::string::npos) base = base.substr(p+1);
  std::string save = std::string("out/") + base;

  std::fprintf(stderr, "Worker: salvando em %s...\n", save.c_str());
  if (write_pgm(save.c_str(), &outImg) != 0) std::fprintf(stderr, "Erro ao salvar %s\n", save.c_str());
  else std::fprintf(stderr, "Worker: imagem salva em %s\n", save.c_str());

  std::free(inImg.data); std::free(outImg.data);
  sem_destroy(&Sitems); sem_destroy(&Sspace); sem_destroy(&Slock);
  return 0;
}
