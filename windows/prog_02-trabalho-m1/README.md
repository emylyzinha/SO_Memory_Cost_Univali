# SO - Processamento de Imagens

Projeto em C++ que aplica filtros em imagens PGM usando **FIFO** e **threads**.
--

##Comandos - Gerar imagem com filtro negativo:

### Terminal 1:
- rm /tmp/imgpipe
- mkfifo /tmp/imgpipe
- g++ src/sender.cpp src/pgm_utils.cpp -o sender -lpthread
- g++ src/worker.cpp src/pgm_utils.cpp src/filters.cpp -o worker -lpthread
- ./worker saida_negativo.pgm negativo <quantidade de threads>

### Termminal 2:
- ./sender img/templo_do_sol.pgm

--

##Comandos - Gerar imagem com filtro de limiarização com fatiamento:

### Terminal 1:
- rm /tmp/imgpipe
- mkfifo /tmp/imgpipe
- g++ src/sender.cpp src/pgm_utils.cpp -o sender -lpthread
- g++ src/worker.cpp src/pgm_utils.cpp src/filters.cpp -o worker -lpthread
- ./worker resultado-limiarizacao.pgm slice 80 160 <quantidade de threads>

### Termminal 2:
- ./sender img/templo_do_sol.pgm

*Observações:*
- Inicie sempre o worker antes do sender.
- No modo slice, pixels fora de [t1, t2] viram branco (255); dentro do intervalo, mantêm o tom original.

--
#!/bin/bash

# Executa o programa e captura o PID
( ./worker saida_negativo.pgm negativo <quantidade_threads> & echo $! > pid.txt )

PID=$(cat pid.txt)
echo "Monitorando processo PID=$PID ..."
echo "Hora       | VmRSS(KB) | VmSize(KB) | minflt | majflt"

# Loop de monitoramento
while kill -0 "$PID" 2>/dev/null; do
  ts=$(date +%H:%M:%S)
  stat="/proc/$PID/stat"
  status="/proc/$PID/status"

  VmRSS=$(awk '/VmRSS/ {print $2}' "$status")
  VmSize=$(awk '/VmSize/ {print $2}' "$status")
  read -r minflt majflt < <(awk '{print $10" "$12}' "$stat")

  printf "%-10s | %-9s | %-9s | %-6s | %-6s\n" "$ts" "$VmRSS" "$VmSize" "$minflt" "$majflt"
  sleep 1
done

echo "✅ Processo $PID finalizado."

-----
Limiarização:
#!/bin/bash

# Executa o programa e captura o PID
( ./worker resultado-limiarizacao.pgm slice 80 160 16 & echo $! > pid.txt )

PID=$(cat pid.txt)
echo "Monitorando processo PID=$PID ..."
echo "Hora       | VmRSS(KB) | VmSize(KB) | minflt | majflt"

# Loop de monitoramento
while kill -0 "$PID" 2>/dev/null; do
  ts=$(date +%H:%M:%S)
  stat="/proc/$PID/stat"
  status="/proc/$PID/status"

  VmRSS=$(awk '/VmRSS/ {print $2}' "$status")
  VmSize=$(awk '/VmSize/ {print $2}' "$status")
  read -r minflt majflt < <(awk '{print $10" "$12}' "$stat")

  printf "%-10s | %-9s | %-9s | %-6s | %-6s\n" "$ts" "$VmRSS" "$VmSize" "$minflt" "$majflt"
  sleep 1
done

echo "✅ Processo $PID finalizado."