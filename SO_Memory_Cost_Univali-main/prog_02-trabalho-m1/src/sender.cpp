#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#endif

#include "pgm_utils.h"

int main(int argc, char** argv){
  if (argc < 2){
    std::fprintf(stderr, "Uso: %s <imagem_entrada.pgm>\n", argv[0]);
    return 1;
  }
  const char* inpath = argv[1];

#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stdin),  _O_BINARY);
#endif

  PGM img{};
  if (read_pgm(inpath, &img) != 0){
    std::fprintf(stderr, "Erro ao ler a imagem: %s\n", inpath);
    return 1;
  }

  // NÃO imprimir nada no stdout! Qualquer log abaixo vai para stderr:
  std::fprintf(stderr, "Sender: l=%d a=%d maxv=%d\n", img.w, img.h, img.maxv);

  // Envia metadados + pixels para STDOUT (binário)
  if (std::fwrite(&img.w,    sizeof(img.w),    1, stdout) != 1 ||
      std::fwrite(&img.h,    sizeof(img.h),    1, stdout) != 1 ||
      std::fwrite(&img.maxv, sizeof(img.maxv), 1, stdout) != 1 ||
      std::fwrite(img.data,  (size_t)img.w * img.h, 1, stdout) != 1){
    std::fprintf(stderr, "Erro ao escrever no STDOUT\n");
    std::free(img.data);
    return 1;
  }
  std::fflush(stdout);
  std::free(img.data);
  return 0;
}
