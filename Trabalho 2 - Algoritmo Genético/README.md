# Trabalho 2 - Algoritmo Genético para a grade de horários

Fundamentos de Sistemas Inteligentes - UTFPR Santa Helena, Ciência da Computação.

O programa monta, com um algoritmo genético (AG) próprio em C11, a grade de horários do 1º ao 4º período, sem conflito de professor e sem conflito entre disciplinas e seus pré-requisitos. O relatório explica cada estratégia, da codificação do cromossomo à condição de parada, e mostra os resultados obtidos.

## Compilar e executar

```bash
make
./trabalho2 --seed 1 --runs 30
make test
```

Opções:

| Opção | Efeito |
|---|---|
| `--seed N` | seed inicial; a mesma seed gera sempre a mesma grade. Sem `--seed`, a seed vem da hora e do número do processo e o programa imprime a seed usada |
| `--periods 4` ou `8` | períodos considerados (padrão 4); com `8` também gera `results/schedule.html`; com `4` o programa informa que o HTML só é gerado com `--periods 8` |
| `--runs N` | executa N seeds consecutivas a partir de `--seed`; o relatório usa 30 |
| `--live` | mostra painel no terminal, atualizado a cada geração; a grade final não muda. Exige terminal de 80x24 ou maior; caso contrário, usa a saída normal |
| `--delay MS` | pausa por geração no painel, de 0 a 5000 ms; padrão 20; exige `--live` |
| `--live-period N` | período mostrado no painel; padrão 1; exige `--live` |
| `--population N` | tamanho da população (padrão 200) |
| `--crossover R` / `--mutation R` | taxas entre 0 e 1 (padrão 0.9 e 0.02) |
| `--generations N` | máximo de gerações (mínimo 500, padrão 2000) |

Saídas: terminal (seed, gerações, penalidade final, tempo e grade por turma), `results/history.csv` (melhor penalidade por geração da melhor seed) e `results/summary.csv` (resumo das execuções).

## O problema

Distribuir, em uma semana de 66 slots, as aulas das disciplinas dos períodos 1 a 4, de forma que:

1. um professor não ministre duas disciplinas no mesmo slot (exigido pelo enunciado);
2. uma disciplina não ocupe o mesmo slot de seu pré-requisito (exigido pelo enunciado);
3. a mesma turma não tenha duas aulas no mesmo slot (regra adicionada além do enunciado, pois uma grade sem ela é inutilizável);
4. nenhum professor seja alocado em slot em que já tem aula de outra turma (regra adicionada além do enunciado, a partir dos mapas de 2026/2).

Os 66 slots são segunda a sexta, de `M1` a `M6` e de `T1` a `T6` (60 slots), mais o sábado de `M1` a `M6` (6 slots). Apenas os Projetos Integradores e `TCC2` podem usar o sábado; eles também podem ocupar segunda a sexta. As demais disciplinas ficam de segunda a sexta.

## Dados e fontes

Os dados foram extraídos uma vez para CSV em `data/`:

| Arquivo | Conteúdo |
|---|---|
| `data/courses.csv` | código, período, aulas semanais, professores e se aceita sábado |
| `data/prerequisites.csv` | pares disciplina e pré-requisito |
| `data/blocked.csv` | slots em que um professor já tem aula de outra turma |
| `data/real_schedule.csv` | aulas reais de 2026/2 das turmas CC1 a CC8 (`code,class,professor,slot,room`), usadas só no HTML |
| `data/course_names.csv` | nome de cada código, da matriz curricular (`code,name`), usado só no HTML |

Fontes: matriz curricular 11 do curso e Mapas de Aulas e Permanências de 2026/2. As aulas semanais vêm da matriz curricular e os professores vêm dos mapas.

`data/real_schedule.csv` tem 219 aulas, extraídas dos 19 mapas de aulas: o professor vem do nome do arquivo, a turma, o código e a sala vêm do texto de cada aula, e o dia vem da posição horizontal da aula contra o cabeçalho do próprio PDF. Ficam de fora `TCC22` e as aulas de turmas fora de CC1 a CC8 (como `AG1`, `AG2`, `AG9` e `CB7`). A contagem por disciplina foi conferida com uma contagem por expressão regular na saída de `pdftotext -layout`, e os dois resultados coincidem.

Decisões sobre os dados:

- Disciplinas com 0 aula semanal, como `ENADE.I`, ficam fora.
- `TCC1` e `TCC2` usam as aulas da matriz curricular (3 e 2 aulas semanais), embora os mapas mostrem menos aulas.
- `TCC22` está fora.
- As optativas vêm da oferta de 2026/2 para a turma: `CC2` tem `OPHE`, `OPHRHL` e `OPHMP`; `CC4` tem `OPHIT2`. Elas contam como aulas da turma.
- Disciplinas com dois professores dividem os blocos ao meio: a primeira metade é do primeiro professor e a segunda metade, do segundo. Isso segue os mapas, em que cada professor dos Projetos Integradores tem 6 das 12 aulas.
- `data/blocked.csv` tem 13 slots bloqueados, de aulas de turmas de outros cursos (como `AG1`, `AG2` e `AG9`): 4 de `EVANDRO NAKAJIM`, 5 de `MEIRA` e 4 de `VANDERLEA`. As aulas noturnas da `CB7` ficam fora dos slots (a semana só tem manhã e tarde) e por isso não aparecem.
- Em `--periods 4`, as disciplinas dos períodos 5 a 8 são ignoradas e não bloqueiam professores.

Com `--periods 4` o AG trata 108 aulas semanais (período 1: 23, período 2: 26, período 3: 33, período 4: 26), organizadas em 48 blocos.

## Estratégias do algoritmo genético

### Codificação do cromossomo

Um cromossomo é um vetor de inteiros. Cada gene é um bloco de aulas consecutivas e seu valor é o slot inicial do bloco. As aulas semanais de cada disciplina são divididas em blocos: 2 aulas viram 1 bloco de 2, 3 viram 1 bloco de 3, 4 viram 2+2, 6 viram 3+3 e 12 viram 3+3+3+3.

Um bloco fica inteiro no mesmo turno do mesmo dia (pode atravessar o intervalo entre aulas do turno). Os genes só recebem inícios válidos, então essa regra nunca é violada e não precisa de penalidade.

Alternativa descartada: um gene por aula, com slot livre. Teria espaço de busca maior e geraria aulas de uma mesma disciplina espalhadas e quebradas. A codificação por bloco reduz o espaço de busca e produz aulas agrupadas, como em uma grade real. A desvantagem é que a divisão em blocos é fixa e o AG não explora outras divisões.

### População inicial

200 indivíduos, cada gene sorteado entre os inícios válidos do bloco. Uma população maior explora mais, mas cada geração custa mais; 200 foi suficiente para chegar a penalidade 0 nas 30 seeds. O sorteio usa um gerador pseudoaleatório próprio com estado local, o que torna a execução reprodutível por seed.

### Fitness

`fitness = 1 / (1 + penalidade)`. A penalidade soma 1 por slot em conflito, para cada regra violada, e vale 0 em uma grade válida. Os quatro tipos são os da seção "O problema". Um mesmo par de blocos que viola mais de uma regra soma uma penalidade por regra.

Contar por slot, em vez de por par de blocos, dá gradiente: um bloco que se sobrepõe em 1 slot é melhor que outro que se sobrepõe em 3. Como não há restrições suaves, penalidade 0 equivale a grade válida. A seleção compara a penalidade diretamente, o que é equivalente a comparar o fitness.

### Seleção

Torneio com `k = 3`. Sorteiam-se 3 indivíduos, com reposição (o mesmo indivíduo pode aparecer mais de uma vez), e vence o de menor penalidade. É simples, não exige ordenar a população nem normalizar o fitness, e `k` controla a pressão seletiva: `k` maior converge mais rápido, com risco de perder diversidade; `k = 1` seria aleatório.

### Crossover

Uniforme por gene, aplicado com taxa 0.9: cada gene do filho vem de um dos dois pais, com probabilidade igual. Como cada gene é independente e sempre um início válido, o filho é sempre válido. Sem crossover (probabilidade 0.1), o filho copia um pai e ainda passa pela mutação. A desvantagem é que o crossover uniforme pode separar genes que formavam uma boa combinação.

### Mutação

Cada gene troca, com taxa 0.02, para outro início válido do mesmo bloco. Com 48 genes, em média 0,96 gene por filho muda. Taxa baixa preserva boas soluções; taxa alta aproxima a busca de aleatória. A troca sempre produz um início válido, diferente do atual.

### Elitismo

Os 2 melhores indivíduos passam direto para a próxima geração. Isso garante que a melhor penalidade nunca piora, ao custo de uma pequena perda de diversidade.

### Condição de parada

O ciclo executa no mínimo 500 gerações, como pede o enunciado, mesmo se a penalidade 0 aparecer antes. Depois disso, para quando a melhor penalidade é 0 ou quando chega a 2000 gerações. O limite de 2000 evita laço infinito se o problema não tiver solução.

## Resultados

Comando: `./trabalho2 --seed 1 --runs 30`, que executa as seeds 1 a 30 com os valores padrão. Os resultados abaixo usaram `--seed 1 --runs 30`; sem `--seed`, a seed muda a cada execução.

Conteúdo de `results/summary.csv`:

| Execuções | Melhor penalidade | Média | Desvio padrão | Taxa de grades sem conflito |
|---|---|---|---|---|
| 30 | 0 | 0.0000 | 0.0000 | 1.0000 |

As 30 seeds terminaram com penalidade 0, e por isso o desvio padrão é 0. As 30 execuções levaram cerca de 12 s nesta máquina; o tempo varia conforme a máquina.

A melhor seed foi a 1 (a primeira com penalidade 0). Seu histórico em `results/history.csv` tem 500 linhas, uma por geração:

| Geração | Melhor penalidade |
|---|---|
| 1 | 13 |
| 10 | 8 |
| 30 | 0 (primeira vez) |
| 50 a 500 | 0 |

A penalidade 0 foi atingida na geração 30 e o AG continuou até a 500ª por causa do mínimo exigido. O arquivo `history.csv` guarda apenas a melhor seed, não as 30.

### Grade da melhor seed (períodos 1 a 4)

Saída do programa para a seed 1, penalidade final 0. As linhas são os slots `M1` a `M6` (manhã) e `T1` a `T6` (tarde); `-` é slot livre.

```
Periodo 1
Horario  SEG              TER              QUA              QUI              SEX              SAB             
M1       -                -                -                CC1MBD           -                -               
M2       -                -                -                CC1MBD           -                -               
M3       -                -                -                -                -                -               
M4       -                -                MA1FM            -                -                -               
M5       -                -                MA1FM            -                -                -               
M6       -                -                -                -                -                -               
T1       -                -                -                CC1AED1          -                -               
T2       MA1LM            -                CC1AED1          CC1AED1          -                -               
T3       MA1LM            CC1MBD           CC1AED1          CC1AED1          -                -               
T4       MA1LM            CC1MBD           CC1AED1          CC1ICC           HU1LA            -               
T5       -                MA1FM            -                CC1ICC           HU1LA            -               
T6       -                MA1FM            -                CC1ICC           HU1LA            -               

Periodo 2
Horario  SEG              TER              QUA              QUI              SEX              SAB             
M1       -                OPHRHL           -                CC2AED2          -                -               
M2       -                OPHRHL           -                CC2AED2          -                -               
M3       -                -                CC2ER            -                CC2ER            -               
M4       -                MA2MA            CC2ER            -                CC2ER            -               
M5       -                MA2MA            CC2CLD           -                CC2POO           -               
M6       -                -                CC2CLD           -                CC2POO           -               
T1       -                CC2POO           -                -                -                -               
T2       -                CC2POO           -                -                -                -               
T3       OPHMP            MA2MA            -                OPHE             -                -               
T4       OPHMP            MA2MA            -                OPHE             -                -               
T5       -                CC2AED2          -                CC2CLD           -                -               
T6       -                CC2AED2          -                CC2CLD           -                -               

Periodo 3
Horario  SEG              TER              QUA              QUI              SEX              SAB             
M1       CC3AED3          -                -                -                CC3PI1           -               
M2       CC3AED3          CC3ES            -                -                CC3PI1           CC3PI1          
M3       -                CC3ES            CC3AED3          -                CC3PI1           CC3PI1          
M4       -                -                CC3AED3          -                CC3AOC           CC3PI1          
M5       CC3ES            -                -                -                CC3AOC           -               
M6       CC3ES            -                -                -                CC3AOC           -               
T1       CC3PI1           -                -                -                MA3AL            -               
T2       CC3PI1           -                -                -                MA3AL            -               
T3       CC3PI1           -                MA3CA            -                MA3AL            -               
T4       CC3AOC           CC3PI1           MA3CA            MA3CA            -                -               
T5       CC3AOC           CC3PI1           -                MA3CA            -                -               
T6       CC3AOC           CC3PI1           -                -                -                -               

Periodo 4
Horario  SEG              TER              QUA              QUI              SEX              SAB             
M1       -                CC4SO            CC4RC            CC4SO            -                -               
M2       CC4RC            CC4SO            CC4RC            CC4SO            -                -               
M3       CC4RC            -                -                CC4IHC           CC4PO            -               
M4       -                -                CC4LBD           CC4IHC           CC4PO            -               
M5       -                MA4PE            CC4LBD           OPHIT2           CC4PO            -               
M6       -                MA4PE            -                OPHIT2           CC4PO            -               
T1       -                CC4IHC           -                -                -                -               
T2       -                CC4IHC           -                -                -                -               
T3       -                -                -                -                -                -               
T4       CC4LBD           -                -                -                MA4PE            -               
T5       CC4LBD           -                -                -                MA4PE            -               
T6       -                -                -                -                -                -
```

## Limitações

- O resultado é uma simulação com os professores de 2026/2. A grade não é oficial.
- Salas são ignoradas pelo AG; elas só aparecem na grade real do HTML.
- Não há restrições suaves: preferências de horário, janelas entre aulas e distribuição pela semana não são avaliadas. Por isso a grade só garante a ausência de conflitos.
- A comparação com a grade real de 2026/2 existe apenas como exibição lado a lado no HTML extra; não há métrica de comparação.
- `TCC1` e `TCC2` usam as aulas da matriz, e as aulas noturnas da `CB7` não entram nos slots.
- Os parâmetros (população, taxas, `k`, elitismo) não foram ajustados por experimentos comparativos; foram fixados no projeto e validados pelos resultados acima.
- Os resultados dependem do gerador pseudoaleatório próprio e das seeds 1 a 30.

## Extra: grade em HTML

Fora do escopo avaliado. `./trabalho2 --seed 1 --periods 8` roda o mesmo AG para os 8 períodos (222 aulas semanais), terminou com penalidade 0 em 500 gerações (cerca de 2 s, variável conforme a máquina) e escreve `results/schedule.html`. Com `--periods 4` o arquivo não é gerado e o programa avisa. Se `data/real_schedule.csv` ou `data/course_names.csv` não existir com `--periods 8`, o programa imprime o erro e sai com código 1.

O HTML tem um slide por período, com navegação por botões e setas do teclado. Cada slide mostra duas tabelas lado a lado (empilhadas em telas estreitas): `Grade real 2026/2` e `Grade gerada pelo AG`. As duas seguem o formato do Mapa de Aulas, com as linhas `M1` a `M6` e `T1` a `T6` com horário de início, os intervalos da manhã (10:00) e da tarde (15:30) e as colunas de segunda a sábado. Cada célula mostra nome, código e professor; na grade real também aparece a sala. A tabela do AG mostra a seed e a penalidade. Uma célula com várias aulas mostra todas, porque optativas podem coincidir na grade real.

O aviso `Grade simulada a partir dos professores de 2026/2. Não é a grade oficial.` vale para a tabela do AG. Nota factual: a grade real tem 219 aulas nas turmas CC1 a CC8 e a do AG tem 222, pois `TCC1` e `TCC2` usam as aulas da matriz curricular. Este trabalho não faz avaliação de qualidade de uma grade em relação à outra.

O HTML foi renderizado com `firefox --headless --screenshot` em 1400 px e 390 px de largura. A navegação por botões e teclado não foi executada em um navegador.

Observação: cada execução sobrescreve `results/history.csv` e `results/summary.csv`. Os CSV deste relatório foram gerados por `./trabalho2 --seed 1 --runs 30`, executado depois da execução com `--periods 8`.

## Testes

`make test` compila e executa `tests/run_tests`: 502 verificações, 0 falhas, resultado PASS. Os testes cobrem:

- contagem de cada tipo de conflito do fitness em casos montados à mão;
- crossover e mutação só produzem genes válidos;
- mesma seed produz o mesmo resultado;
- um verificador independente (`tests/verify_schedule.c`) recontabiliza os conflitos da melhor grade sem usar `src/fitness`;
- leitura dos CSV, incluindo arquivos inválidos (`tests/fixtures`), CLI (com e sem `--seed`) e relatório;
- leitura da grade real e dos nomes (arquivo válido, CRLF, slot inválido e arquivo ausente) e o conteúdo do HTML.

A compilação usa `-std=c11 -Wall -Wextra -Werror` e termina sem avisos.

## Estrutura do código

| Caminho | Responsabilidade |
|---|---|
| `src/data.c` | lê e valida os CSV |
| `src/chromosome.c` | cria blocos, inícios válidos e indivíduos |
| `src/fitness.c` | calcula a penalidade |
| `src/genetic.c` | seleção, crossover, mutação, elitismo e parada |
| `src/rng.c` | gerador pseudoaleatório próprio |
| `src/cli.c` | leitura das opções |
| `src/real_schedule.c` | lê a grade real e os nomes das disciplinas |
| `src/report.c` | terminal, CSV e HTML |
| `src/main.c` | coordena a execução |
| `tests/` | testes e verificador independente |
| `data/` | CSV de disciplinas, pré-requisitos, bloqueios, grade real e nomes |
| `results/` | `history.csv`, `summary.csv` e `schedule.html` |
