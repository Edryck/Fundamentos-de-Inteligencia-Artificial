# Trabalho 1 — Busca em grafos com agente navegador

Fundamentos de Sistemas Inteligentes — UTFPR

Um agente parte do início (`@`) de um mapa em grade, precisa chegar ao prêmio (`*`) e deve coletar
o máximo de recompensas (`$`) que conseguir pelo caminho. O terreno tem custos diferentes, e o
programa compara quatro estratégias de busca: **busca em largura (BFS)**, **busca em profundidade
(DFS)**, **busca gulosa** e **A\***. O trabalho está em C11, só com a biblioteca padrão, e roda em
terminal Linux com cores ANSI.

## Sumário

1. [Compilação e execução](#compilação-e-execução)
2. [O mapa](#o-mapa)
3. [Modelagem como grafo](#modelagem-como-grafo)
4. [Algoritmos e complexidade](#algoritmos-e-complexidade)
5. [Heurística](#heurística)
6. [Caminho inexistente, salas e becos](#caminho-inexistente-salas-e-becos)
7. [Resultados](#resultados)
8. [Discussão](#discussão)
9. [Recursos além do pedido na especificação](#recursos-além-do-pedido-na-especificação)
10. [Como foi testado](#como-foi-testado)
11. [Estrutura do código](#estrutura-do-código)

## Compilação e execução

Requisitos: `gcc` com suporte a C11, `make` e um terminal com UTF-8 e 256 cores.

```bash
make                      # compila (gcc -std=c11 -Wall -Wextra -Werror -O2)
make run                  # executa com um mapa gerado, seed aleatória
make run SEED=42          # mapa reproduzível
make run ARGS=--no-anim   # sem animação
make clean                # remove os arquivos gerados
```

Sem argumentos o programa gera um mapa, desenha a legenda, mostra a análise do grafo, anima a rota
de cada algoritmo e termina com a tabela comparativa. A animação só acontece quando a saída é um
terminal; redirecionada para um arquivo, o programa imprime só o quadro final de cada rota.

Principais opções (`./trabalho1 --help` lista todas):

| Opção | Efeito |
|---|---|
| `--seed N` | seed do gerador de mapas (padrão: horário atual; é sempre impressa) |
| `--rows R --cols C` | dimensões do mapa gerado (padrão 12×20) |
| `--map ARQUIVO` / `--save-map ARQUIVO` | carrega ou grava um mapa em arquivo texto |
| `--from L,C` / `--to L,C` | move o início ou o prêmio para outra célula (contagem a partir de 0) |
| `--allow-disconnected` | cerca o prêmio com paredes, para demonstrar "SEM CAMINHO" |
| `--algo LISTA` | `bfs`, `dfs`, `ucs`, `greedy`, `astar`, `astar-safe` ou `all`, separados por vírgula |
| `--bonus N` / `--radius N` | valor de uma recompensa e raio de atração na heurística (padrão 10 e 3) |
| `--no-anim` | não anima a rota |
| `--explored` | desenha também as células que cada algoritmo expandiu |
| `--repeat N` | execuções usadas na média de tempo (padrão 50) |
| `--bench` | compara os algoritmos em vários mapas (seção de resultados) |

Exemplos:

```bash
./trabalho1 --seed 42 --no-anim                    # o caso discutido abaixo
./trabalho1 --seed 7 --allow-disconnected          # prêmio inalcançável
./trabalho1 --seed 42 --algo bfs,astar --explored  # compara a área explorada
./trabalho1 --seed 42 --from 2,2 --to 9,17         # rota entre dois pontos quaisquer
./trabalho1 --bench --seed 1                       # comparação em 50 mapas por tamanho
```

## O mapa

O ambiente é uma grade cercada por paredes externas. Cada símbolo tem um significado e um custo de
entrada:

| Símbolo | Significado | Custo de entrada |
|---|---|---|
| `S` (ou `.`) | sólido e plano | 1 |
| `A` | arenoso | 4 |
| `R` | rochoso | 10 |
| `P` | pântano | 20 |
| `#` | parede | intransponível |
| `@` | início do agente | 1 |
| `*` | prêmio (objetivo) | 1 |
| `$` | recompensa | 1 |

**Geração.** O mapa não é fixo: o gerador usa a seed informada, de forma que a mesma seed sempre
produz o mesmo mapa. Os terrenos formam manchas contíguas (cada célula herda o terreno da semente
mais próxima), as paredes aparecem em segmentos curtos, que criam corredores e becos, e o início e
o prêmio ficam distantes um do outro. O gerador garante pelo menos 30 vértices e pelo menos 5
recompensas, e abre um corredor se o prêmio ficar isolado. Como o gerador de números aleatórios é
próprio (xorshift com estado local), o resultado não depende da versão da libc.

**Arquivo.** `--map` aceita arquivos texto com os símbolos acima, uma linha por linha do mapa, todas
com o mesmo comprimento. O programa recusa arquivos com símbolos desconhecidos, sem início ou
prêmio, com menos de 5 recompensas ou menos de 30 células caminháveis, e diz o motivo. Um mapa
gerado pode ser salvo com `--save-map` e editado à mão.

## Modelagem como grafo

Cada célula caminhável vira um vértice (as paredes não entram no grafo) e cada par de células
vizinhas na vertical ou horizontal vira duas arestas dirigidas. **O peso da aresta é o custo de
entrar no vértice de destino.** O custo de um caminho é a soma dos pesos das arestas percorridas, ou
seja, o início não conta. Cada vértice guarda linha, coluna, terreno, custo de entrada, se tem
recompensa, se é o início ou o prêmio, e até quatro arestas (listas de adjacência de tamanho fixo,
sem alocação por aresta).

As estruturas de apoio foram implementadas do zero: fila circular (BFS), pilha dinâmica (DFS) e
min-heap com desempate determinístico (custo uniforme, guloso e A\*).

## Algoritmos e complexidade

Seja **V** o número de vértices, **E ≤ 4V** o de arestas, **R** o raio da heurística e **P** o
comprimento do caminho corrente.

| Algoritmo | Tipo | Completo | Ótimo em custo | Tempo | Memória |
|---|---|---|---|---|---|
| BFS | cega | sim | não (só em número de arestas) | O(V + E) | O(V) |
| DFS | cega | sim, com controle de visitados | não | O(V + E) | O(E) por causa de entradas repetidas na pilha |
| Custo uniforme | cega | sim | sim | O((V + E) log V) | O(V) |
| Guloso | informada | sim, com controle de visitados | não | O((V + E) log V) mais o custo da heurística | O(V) |
| A\* seguro (bônus 0) | informada | sim | sim | O((V + E) log V) | O(V) |
| A\* (com bônus) | informada | sim | não (troca custo por recompensas) | sem limite polinomial garantido, pois nós podem ser reabertos | O(V) |

Detalhes de implementação:

- **BFS** marca o vértice como visitado ao enfileirá-lo e reconstrói o caminho pelos pais. Encontra
  o caminho com menos arestas, ignorando o custo do terreno.
- **DFS** usa uma pilha explícita (sem recursão, para não estourar a pilha em mapas grandes). O
  vértice só é marcado como visitado ao sair da pilha, o que mantém o comportamento de uma busca em
  profundidade de verdade, ao custo de entradas repetidas na pilha.
- **Custo uniforme** (Dijkstra com parada no prêmio) expande o vértice de menor custo acumulado.
  Entradas antigas ficam no heap e são descartadas ao sair.
- **Guloso** ordena a fronteira só pela heurística, ignorando o custo já pago.
- **A\*** ordena por `f = g + h`, desempatando pelo menor `h`, e reabre vértices quando acha um
  caminho melhor.
- **Nós expandidos** são os que saem da fronteira (o prêmio conta). **Nós gerados** são todas as
  entradas colocadas na fronteira, inclusive repetidas, e a **fronteira máxima** é o maior tamanho
  que ela atingiu; as duas medem o uso de memória.

## Heurística

### Base: distância de Manhattan

`h(n) = |linha(n) − linha(prêmio)| + |coluna(n) − coluna(prêmio)|`

- **Admissível.** Em uma grade com vizinhança de 4, cada passo muda a distância de Manhattan em no
  máximo 1, e cada passo custa no mínimo 1 (o menor custo de entrada é 1). Um caminho de k passos até
  o prêmio custa pelo menos k, e k é pelo menos a distância de Manhattan. Logo `h(n) ≤ h*(n)`.
- **Consistente.** Para vértices vizinhos, `|h(n) − h(n')| ≤ 1 ≤ c(n, n')`, então
  `h(n) ≤ c(n, n') + h(n')`. Por isso o A\* clássico nunca precisa reabrir um vértice e o primeiro
  caminho que ele encontra até o prêmio é ótimo.

Esse é o comportamento do **A\* seguro** (`astar-safe`).

### Considerando recompensas próximas

A especificação pede que Guloso e A\* avaliem se vale a pena sair da rota para pegar recompensas
perto dela. A heurística com bônus faz isso de duas maneiras, ambas controladas por `BONUS`, que é
o **valor de uma recompensa em unidades de custo de terreno** (padrão 10):

1. **Atração.** Cada recompensa ainda não coletada a distância `d ≤ RAIO` do vértice reduz a
   estimativa em `BONUS × (RAIO − d + 1) / (RAIO + 1)`. O peso vai de `BONUS` em cima da recompensa
   até uma fração dela na borda do raio. A atração soma no máximo duas recompensas, e a estimativa
   nunca fica negativa:

   `h'(n) = max(0, manhattan(n, prêmio) − atração(n))`

2. **Crédito por coleta.** Pegar uma recompensa nova abate `BONUS` do custo acumulado. No A\*, o
   custo acumulado passa a ser líquido: `g_líquido = custo do terreno − BONUS × recompensas coletadas`.
   No Guloso, a prioridade é `h'(n) − BONUS × recompensas coletadas no caminho`. Na prática, **o
   desvio compensa quando custa menos que uma recompensa**.

Com `BONUS = 0` as duas partes desaparecem e a heurística volta a ser Manhattan.

### Por que a versão com bônus perde as garantias

- `h'(n) ≤ manhattan(n, prêmio)`, então `h'` continua sem superestimar o custo real até o prêmio.
- Mas o A\* com bônus passa a minimizar outra função (custo de terreno menos o valor das
  recompensas), e para ela `h'` não é admissível: recompensas fora do raio também reduziriam o custo
  líquido e a heurística não as enxerga. Além disso, `h'` depende do caminho percorrido (o que já foi
  coletado), então não é consistente, e por isso o algoritmo reabre vértices.
- Para a recompensa contar uma única vez e a árvore de pais não formar ciclos, com bônus o caminho
  nunca repete vértice.
- Resultado: **o A\* com bônus e o Guloso não são ótimos em custo de terreno**. Isso é a escolha
  deliberada do trabalho: trocar um pouco de custo por mais recompensas. O A\* seguro e o custo
  uniforme continuam ótimos.

### Uma tentativa que não funcionou

A primeira versão seguia direto a fórmula `h − BONUS × (recompensas restantes dentro do raio)`.
Medindo em 200 mapas, ela coletava **menos** recompensas do que a mesma busca sem bônus:

| Versão (200 mapas) | Guloso, recompensas coletadas | A\*, recompensas coletadas |
|---|---|---|
| sem bônus | 0,70 | 1,08 |
| fórmula original, bônus 5 | 0,29 | 0,90 |

O motivo é que a contagem só premia estar perto de uma recompensa, não pisar nela: uma célula
vizinha valia tanto quanto a própria recompensa, e ao pisar nela o bônus sumia. O agente ficava
rondando e não coletava. A solução foi ponderar a atração pela proximidade e abater o valor das
recompensas já coletadas, como descrito acima.

## Caminho inexistente, salas e becos

Antes de buscar, o programa analisa o grafo:

- **Componentes conexos** (busca em largura a partir de cada vértice ainda não rotulado). Cada um é
  listado com tamanho, recompensas e se contém o início ou o prêmio. Um componente sem o início nem
  o prêmio é uma **sala isolada**, como uma sala sem porta.
- **Prêmio alcançável?** Se o início e o prêmio estão em componentes diferentes, aparece
  `SEM CAMINHO ATÉ O PRÊMIO`. As buscas ainda rodam, para a tabela mostrar quantos nós cada uma
  expandiu até perceber que não há caminho, e retornam "não encontrado".
- **Becos sem saída.** Remove-se repetidamente os vértices com um único vizinho, exceto o início e
  o prêmio. O que for removido faz parte de um beco, inclusive corredores compridos, não só a última
  célula. As coordenadas aparecem no relatório.

Exemplo com `./trabalho1 --seed 7 --allow-disconnected`:

```
vértices: 150 | recompensas: 6 | componentes: 3 | células de beco: 17
início (linha,coluna): 7,15 | prêmio: 2,3
  componente 0: 147 vértices, 6 recompensas [início]
  componente 1: 1 vértices, 0 recompensas [prêmio]
  componente 2: 2 vértices, 0 recompensas (sala isolada, sem acesso do agente)
  becos sem saída (linha,coluna): 1,1 1,2 1,4 1,5 1,6 2,1 2,5 8,6 8,7 9,16 9,18 10,12 ...
SEM CAMINHO ATÉ O PRÊMIO
```

## Resultados

Os tempos variam com a máquina; os demais números são determinísticos para uma dada seed. Em todos
os casos o tempo é a média de várias execuções (50 no caso individual, 5 no benchmark).

### Um mapa: `./trabalho1 --seed 42`

```
####################
#RAAAAAAAARRA#ASS$A#
#RAAAAA$AARR##A#SAA#
#RRAAAAAARRR##A#SA##
#RRAAAAARRR##AA#AA##
#####AA$ARRRRAA#AA##
#*RRAARRR##RRAAAS@##
#RRRRRR####RRSSSSSS#
#RRRRRRRRR$RSSSSSSS#
#RRRRRR#R$RRSSSSSSS#
#RRRRRR#RRRRSSS$SSS#
####################
```

153 vértices, 6 recompensas, 1 componente, 1 célula de beco. Início em (6,17), prêmio em (6,1).

| Algoritmo | Passos | Custo | Expandidos | Gerados | Fronteira máx. | $ coletados | $ vistos | Tempo (μs) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| BFS | 19 | 120 | 143 | 149 | 12 | 0 | 6 | 8,5 |
| DFS | 87 | 503 | 91 | 172 | 82 | 4 | 4 | 11,4 |
| Custo uniforme | 21 | 95 | 137 | 146 | 16 | 1 | 6 | 16,9 |
| Guloso | 25 | 123 | 106 | 130 | 38 | 2 | 6 | 87,3 |
| A\* (bônus 10) | 27 | 101 | 136 | 163 | 23 | 2 | 6 | 141,3 |
| A\* seguro | 21 | 95 | 130 | 143 | 18 | 1 | 6 | 25,0 |

"$ coletados" são as recompensas sobre o caminho final (as que o agente de fato recolhe); "$
vistos" são as que estavam em vértices expandidos pela busca.

### Vários mapas: `./trabalho1 --bench --seed 1`

Média de 50 mapas gerados por tamanho. Todas as buscas resolveram 50 de 50 mapas.

**Mapa 12×20 (153 vértices em média)**

| Algoritmo | Passos | Custo | Expandidos | Gerados | Fronteira máx. | $ coletados | Tempo (μs) |
|---|---:|---:|---:|---:|---:|---:|---:|
| BFS | 20,2 | 121,7 | 135,3 | 140,0 | 12,2 | 0,7 | 4,8 |
| DFS | 85,0 | 529,7 | 109,1 | 195,0 | 81,9 | 3,3 | 6,7 |
| Custo uniforme | 22,5 | 72,2 | 116,6 | 128,0 | 18,3 | 1,1 | 9,5 |
| Guloso | 28,5 | 154,9 | 44,6 | 74,7 | 32,4 | 3,1 | 22,3 |
| A\* | 25,6 | 77,3 | 86,5 | 115,7 | 30,4 | 2,5 | 36,3 |
| A\* seguro | 22,5 | 72,2 | 88,5 | 110,0 | 26,1 | 1,1 | 9,2 |

**Mapa 24×40 (711 vértices em média)**

| Algoritmo | Passos | Custo | Expandidos | Gerados | Fronteira máx. | $ coletados | Tempo (μs) |
|---|---:|---:|---:|---:|---:|---:|---:|
| BFS | 38,1 | 218,9 | 617,8 | 630,1 | 28,8 | 1,5 | 14,4 |
| DFS | 275,4 | 1719,0 | 417,1 | 771,2 | 302,0 | 10,8 | 17,5 |
| Custo uniforme | 44,6 | 104,6 | 515,1 | 551,8 | 55,2 | 2,5 | 45,6 |
| Guloso | 63,6 | 351,9 | 159,6 | 237,5 | 93,6 | 8,6 | 101,0 |
| A\* | 56,9 | 125,2 | 337,9 | 463,2 | 100,7 | 7,8 | 184,3 |
| A\* seguro | 44,6 | 104,6 | 390,1 | 469,2 | 72,3 | 2,4 | 43,9 |

**Mapa 48×80 (3050 vértices em média)**

| Algoritmo | Passos | Custo | Expandidos | Gerados | Fronteira máx. | $ coletados | Tempo (μs) |
|---|---:|---:|---:|---:|---:|---:|---:|
| BFS | 77,8 | 450,6 | 2600,6 | 2626,4 | 59,4 | 3,3 | 53,2 |
| DFS | 1028,3 | 6385,8 | 1790,2 | 3327,9 | 1216,7 | 40,6 | 66,4 |
| Custo uniforme | 93,6 | 179,5 | 2270,0 | 2375,3 | 145,8 | 4,7 | 219,6 |
| Guloso | 166,2 | 952,9 | 737,4 | 935,7 | 296,7 | 22,9 | 944,7 |
| A\* | 146,4 | 256,9 | 1027,5 | 1456,2 | 289,8 | 21,1 | 895,0 |
| A\* seguro | 93,6 | 179,5 | 1540,5 | 1832,2 | 206,8 | 4,7 | 173,8 |

### Efeito do bônus

Média de 200 mapas 12×20, raio 3. A última coluna conta em quantos mapas o A\* com bônus coletou
mais recompensas que o A\* seguro.

| Bônus | Guloso: recompensas / custo | A\*: recompensas / custo | A\* coletou mais que o seguro |
|---:|---|---|---:|
| 0 | 0,70 / 135,6 | 1,08 / 74,7 | 0 |
| 5 | 2,43 / 139,1 | 2,12 / 76,6 | 120 |
| 10 | 3,14 / 155,6 | 2,71 / 80,5 | 157 |
| 20 | 3,30 / 163,0 | 3,25 / 87,4 | 181 |
| 40 | 3,25 / 162,0 | 3,75 / 101,7 | 191 |

## Discussão

**Custo do terreno.** O BFS acha o caminho com menos passos (19 no mapa de exemplo), mas passa por
terreno caro e paga 120, contra 95 do caminho ótimo. Isso mostra que, em um grafo ponderado, "menos
arestas" e "menor custo" são objetivos diferentes. O custo uniforme e o A\* seguro encontram sempre
o mesmo custo mínimo, o que confirma a otimalidade e a admissibilidade de Manhattan.

**Direção da busca.** O custo uniforme expande em todas as direções. O A\* seguro chega ao mesmo
custo expandindo menos (nos 50 mapas 48×80, 1540 contra 2270 vértices, 32% a menos), porque a
heurística o aponta para o prêmio. A economia está nos nós expandidos, não na fronteira: o A\*
seguro guarda uma fronteira maior (207 contra 146 entradas). O Guloso expande ainda menos (737), mas paga caro
em qualidade: custo médio de 953, mais de cinco vezes o ótimo, porque ignora o que já gastou.

**DFS.** É o que menos expande entre as buscas cegas, mas encontra caminhos longos e muito caros
(custo médio 6386 no maior mapa, mais de 35 vezes o ótimo) e exige a maior fronteira (1217 entradas), pois
empilha vizinhos repetidos. Como o caminho dele é longo e serpenteia pelo mapa, coleta muitas
recompensas (40,6 em média nesse tamanho, em mapas com cerca de 120), mas a custa de rotas
impraticáveis. Coletar recompensas não pode ser avaliado sem olhar o custo ao lado.

**Recompensas.** As buscas cegas só pegam recompensas por acaso. Com o bônus, o A\* troca custo por
recompensa de forma controlada: com bônus 10 coleta 2,5 vezes mais recompensas que o seguro (2,71
contra 1,08) com 8% a mais de custo. Aumentar o bônus continua trocando custo por recompensa
(bônus 40: 3,75 recompensas, 36% a mais de custo). O parâmetro funciona como o preço que o agente
aceita pagar por uma recompensa. O Guloso com bônus coleta muitas recompensas, mas já parte de
caminhos ruins, e o bônus sozinho não corrige o custo.

**Tempo.** Nos mapas pequenos, todas as buscas levam de poucos a algumas dezenas de microssegundos,
diferenças pequenas demais para tirar conclusões. Nos maiores, BFS e DFS são os mais rápidos por não manterem heap. O Guloso e o A\* com bônus são os mais
lentos por iteração (cerca de 900 μs contra 170 do A\* seguro), porque cada avaliação da heurística
varre o raio e percorre o caminho percorrido até o vértice. É o preço de decidir sobre os desvios.

**Memória.** No mapa 48×80, o BFS tem a menor fronteira (59 entradas em média) e o DFS a maior
(1217, que cresce com o comprimento do caminho). O custo uniforme fica em 146, o A\* seguro em 207 e
o Guloso e o A\* com bônus em torno de 290. A heurística reduz o número de nós expandidos, mas não
a fronteira.

## Recursos além do pedido na especificação

A especificação pede os quatro algoritmos, o mapa com terrenos e recompensas, a detecção de caminho
inexistente, becos e salas, a animação da rota e o comparativo de tempo e nós expandidos. Foi
adicionado ainda:

- geração procedural de mapas, reproduzível por seed, com `--map`, `--save-map`, `--rows`, `--cols`
  e `--allow-disconnected`;
- **busca de custo uniforme** (Dijkstra) como referência de otimalidade;
- **A\* seguro** ao lado do A\* com bônus, com parâmetros `--bonus` e `--radius`;
- colunas de memória na tabela (nós gerados e fronteira máxima) e de recompensas vistas;
- **mapa de exploração** (`--explored`), que mostra as células expandidas por cada algoritmo;
- **benchmark** (`--bench`) com médias sobre vários mapas e tamanhos;
- origem e destino arbitrários (`--from`, `--to`);
- seleção de algoritmos (`--algo`) e média de tempo em várias execuções (`--repeat`).

## Como foi testado

Cada algoritmo foi comparado com uma implementação independente escrita em Python, rodando sobre os
mesmos mapas:

- o custo do A\* seguro e do custo uniforme foi comparado com um Dijkstra de referência em centenas
  de mapas e em pares aleatórios de origem e destino;
- o BFS foi comparado quanto ao número mínimo de arestas;
- caminho, nós expandidos, gerados e fronteira máxima de BFS, DFS, custo uniforme, Guloso e A\*
  (inclusive com bônus) foram comparados com reimplementações que seguem as mesmas regras;
- a decisão sobre haver ou não caminho foi conferida em mapas conectados e em mapas com o prêmio
  cercado;
- o programa foi executado com AddressSanitizer e UndefinedBehaviorSanitizer em centenas de mapas
  (inclusive em terminal com animação e nos caminhos de erro), sem erros nem vazamentos. Para
  repetir a verificação de vazamentos com Valgrind:
  `valgrind --leak-check=full ./trabalho1 --seed 42 --no-anim --repeat 1`;
- o código compila do zero sem nenhum aviso com `-Wall -Wextra -Werror`.

## Estrutura do código

```
Makefile
src/
  main.c        fluxo principal
  cli.[ch]      leitura dos argumentos
  map.[ch]      mapa em grade: leitura/gravação de arquivo, validação e reposicionamento
  generator.[ch] geração procedural do mapa (terrenos, paredes, recompensas)
  graph.[ch]    montagem do grafo e análise (componentes, becos)
  search.[ch]   BFS, DFS, custo uniforme, Guloso e A*
  heuristic.[ch] Manhattan e bônus de recompensas
  runner.[ch]   execução de um algoritmo com média de tempo
  bench.[ch]    modo de benchmark
  render.[ch]   desenho ANSI, legenda, animação e mapa de exploração
  metrics.[ch]  cronômetro e tabelas
  queue.[ch]  stack.[ch]  min_heap.[ch]   estruturas de dados
```
