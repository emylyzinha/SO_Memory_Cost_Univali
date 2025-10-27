# 🧩 Programa 01 — Monitoramento de Memória e Threads


## 🔹 Linux

**Linguagem:** C++  
**Compilador:**
```bash
g++ -O2 -std=c++17 -pthread -o main /workspaces/SO_Memory_Cost_Univali/main.cpp
```
**Execução:**
```bash
./main <quantidade_threads> <numero_iteracoes> <frequencia_acesso_paginas> <liberar_memoria_apos_toque>
```

Onde <liberar_memoria_apos_toque>:
0 → mantém a memória alocada
1 → libera a memória após o toque

**Exemplo de execução simples:**
```bash
./main 8 10000 2 0
```

**Script completo de monitoramento (Linux):**
```bash
CMD="./main 8 10000 2 0"

# Inicia o comando em background e salva o PID
$CMD &
PID=$!
echo "Monitorando processo PID=$PID ..."

printf "%-10s | %-9s | %-10s | %-10s | %-7s | %-6s | %-5s | %-5s\n" \
"hora" "VmRSS(kB)" "VmSize(kB)" "VmSwap(kB)" "Thr" "minflt" "majflt" "vctxt" "nvctxt"

# Loop de monitoramento enquanto o processo existir
while kill -0 "$PID" 2>/dev/null; do
  ts=$(date +%H:%M:%S)

  status="/proc/$PID/status"
  stat="/proc/$PID/stat"

  VmRSS=""; VmSize=""; VmSwap=""; Thr=""; vctxt=""; nvctxt=""; minflt=""; majflt=""

  [[ -r $status ]] && {
    VmRSS=$(awk '/^VmRSS:/ {print $2}' "$status")
    VmSize=$(awk '/^VmSize:/ {print $2}' "$status")
    VmSwap=$(awk '/^VmSwap:/ {print $2}' "$status")
    Thr=$(awk '/^Threads:/ {print $2}' "$status")
    vctxt=$(awk '/^voluntary_ctxt_switches:/ {print $2}' "$status")
    nvctxt=$(awk '/^nonvoluntary_ctxt_switches:/ {print $2}' "$status")
  }

  [[ -r $stat ]] && read -r minflt majflt < <(awk '{print $10 " " $12}' "$stat")

  printf "%-10s | %-9s | %-10s | %-10s | %-7s | %-6s | %-5s | %-5s\n" \
  "$ts" "$VmRSS" "$VmSize" "$VmSwap" "$Thr" "$minflt" "$majflt" "$vctxt" "$nvctxt"

  sleep 1
done

[[ -r $stat ]] && read -r minflt majflt < <(awk '{print $10 " " $12}' "$stat")
echo "✅ Processo $PID finalizou. VmRSS: $VmRSS kB | Threads: $Thr | minflt: $minflt | majflt: $majflt | vctxt: $vctxt | nvctxt: $nvctxt"
```

| Tipo de análise                      | Comando / Ferramenta                                             |       |                               |
| ------------------------------------ | ---------------------------------------------------------------- | ----- | ----------------------------- |
| **Monitoramento de processos**       | `htop`                                                           |       |                               |
| **Tamanho da memória e threads**     | `watch -n 1 "grep -E 'VmSize                                     | VmRSS | Threads' /proc/<PID>/status"` |
| **Uso detalhado de memória**         | `watch -n 1 "pmap -x <PID>"`                                     |       |                               |
| **Falta de páginas (page faults)**   | `sudo perf stat -e minor-faults,major-faults -p <PID> sleep 10`  |       |                               |
| **Gráfico simples de monitoramento** | `sudo apt update && sudo apt install htop glances -y && glances` |       |                               |

---

## 🔹 Windows

**Linguagem: C++**
**Compilador:**
```bash
g++ -O2 -std=c++17 -o "main.exe" "main.cpp"
```

**Execução:**
```bash
./main <quantidade_threads> <numero_iteracoes> <frequencia_acesso_paginas> <liberar_memoria_apos_toque>
```

**Exemplo:**
```bash
./main 8 10000 2 0
```
🔍 Tipos de análise e ferramentas (Windows)
| Tipo de análise                    | Comando / Ferramenta                               |
| ---------------------------------- | -------------------------------------------------- |
| **Monitoramento de processos**     | Process Explorer                                   |
| **Tamanho da memória e threads**   | Process Explorer → aba *Memory*                    |
| **Uso detalhado de memória**       | Process Explorer → *Properties → Threads*          |
| **Falta de páginas (page faults)** | Process Explorer → aba *Performance → Page Faults* |

# 🧩 Programa 02 — Processamento de Imagens (Negativo e Limiarização)

---

## 🔹 Linux

**Linguagem:** C++  
**Compiladores:**

```bash
# Compilação dos programas
g++ src/sender.cpp src/pgm_utils.cpp -o sender -lpthread
g++ src/worker.cpp src/pgm_utils.cpp src/filters.cpp -o worker -lpthread
```

🖤 Execução — Filtro Negativo

Terminal 1:
```bash
rm /tmp/imgpipe
mkfifo /tmp/imgpipe
./worker saida_negativo.pgm negativo <quantidade_de_threads>
```

Terminal 2:
```bash
./sender img/templo_do_sol.pgm
```

⚪ Execução — Filtro de Limiarização

Terminal 1:
```bash
rm /tmp/imgpipe
mkfifo /tmp/imgpipe
./worker resultado-limiarizacao.pgm slice 80 160 <quantidade_de_threads>
```

Terminal 2:
```bash
./sender img/templo_do_sol.pgm
```

Script de Monitoramento (Linux)
```bash
#!/bin/bash

# Executa o programa e captura o PID
( ./worker saida_negativo.pgm negativo 8 & echo $! > pid.txt )

PID=$(cat pid.txt)
echo "Monitorando processo PID=$PID ..."
echo "Hora       | VmRSS(KB) | VmSize(KB) | minflt | majflt"

# Loop de monitoramento
while kill -0 "$PID" 2>/dev/null; do
  ts=$(date +%H:%M:%S)

  status="/proc/$PID/status"
  stat="/proc/$PID/stat"

  VmRSS=$(awk '/^VmRSS:/ {print $2}' "$status")
  VmSize=$(awk '/^VmSize:/ {print $2}' "$status")
  read -r minflt majflt < <(awk '{print $10 " " $12}' "$stat")

  printf "%-8s | %-9s | %-9s | %-6s | %-6s\n" "$ts" "$VmRSS" "$VmSize" "$minflt" "$majflt"
  sleep 1
done

echo "✅ Processo $PID finalizado."
```

🔹 Windows

Linguagem: C++
Compiladores:
```bash
g++ -O2 -std=c++17 -pthread -o sender.exe src\sender.cpp src\pgm_utils.cpp
g++ -O2 -std=c++17 -pthread -o worker.exe src\worker.cpp src\pgm_utils.cpp src\filters.cpp
```

🖤 Execução — Filtro Negativo
```bash
sender.exe img\templo_do_sol.pgm | worker.exe out\saida_negativo.pgm negativo 4
```

⚪ Execução — Filtro de Limiarização
```bash
sender.exe img\templo_do_sol.pgm | worker.exe out\resultado-limiarizacao.pgm slice 80 160 4
```

Análise (Windows)
| Tipo de análise                    | Comando / Ferramenta                               |
| ---------------------------------- | -------------------------------------------------- |
| **Monitoramento de processos**     | Process Explorer                                   |
| **Tamanho da memória e threads**   | Process Explorer → aba *Memory*                    |
| **Uso detalhado de memória**       | Process Explorer → *Properties → Threads*          |
| **Falta de páginas (page faults)** | Process Explorer → aba *Performance → Page Faults* |


# 🧩 Programa 03 — Custo de Memória (C++ e Java)

## 🔹 C++ | Linux

**Linguagem:** C++  
**Compilação:**
```bash
g++ -O2 -std=c++17 -o main main.cpp
```

Execução:
```bash
./main
```

Monitoramento de Memória — Terminal (C++)
```bash
./main &
PID=$!
SECONDS=0

while kill -0 $PID 2>/dev/null; do
    RSS=$(grep VmRSS /proc/$PID/status | awk '{print $2}')
    VSZ=$(grep VmSize /proc/$PID/status | awk '{print $2}')
    read minflt majflt < <(awk '{print $10, $12}' /proc/$PID/stat)
    MEM=$(ps -p $PID -o %mem=)
    echo "Tempo(s): $SECONDS | VmRSS: ${RSS}KB | VmSize: ${VSZ}KB | MEM: ${MEM}% | minflt: $minflt | majflt: $majflt"
    SECONDS=$((SECONDS+1))
    sleep 1
done
```

🔹 Java | Linux

Linguagem: Java
Compilação:
```bash
javac Matriz.java
```
Execução:
```bash
java Matriz
```

Se recisar de mais memória heap:
```bash
java -Xmx1G Matriz
```

Monitoramento de Memória — Terminal (Java)
```bash
PID=$!
echo "Monitoring PID $PID..."
SECONDS=0

while kill -0 $PID 2>/dev/null; do
    RSS=$(grep VmRSS /proc/$PID/status | awk '{print $2}')
    VSZ=$(grep VmSize /proc/$PID/status | awk '{print $2}')
    read minflt majflt < <(awk '{print $10, $12}' /proc/$PID/stat)
    MEM=$(ps -p $PID -o %mem=)
    echo "Tempo(s): $SECONDS | VmRSS: ${RSS}KB | VmSize: ${VSZ}KB | MEM: ${MEM}% | minflt: $minflt | majflt: $majflt"
    SECONDS=$((SECONDS+1))
    sleep 1
done

echo "✅ Processo finalizado."
```

Comandos Monitoramento de Memória
| Tipo de análise                     | Ferramenta / Comando                          |
| ----------------------------------- | --------------------------------------------- |
| **Uso geral de processos**          | `htop` ou `top`                               |
| **Detalhes de memória e threads**   | `cat /proc/<PID>/status`                      |
| **Faltas de páginas (page faults)** | `sudo perf stat -e minor-faults,major-faults` |
| **Gráfico interativo**              | `glances`                                     |


