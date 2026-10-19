#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#include "graph.h"
#include "map.h"
#include "search.h"

void render_legend(void);

/* Resume o grafo: vértices, recompensas, componentes isolados, becos e se o prêmio é alcançável. */
void render_analysis(const Graph *graph, const GraphAnalysis *analysis);

/* Desenha o mapa sem rota, com o agente na posição inicial. */
void render_map(const Map *map);

/* Mostra a rota encontrada. Com `animate`, o agente anda passo a passo, mas só quando a
   saída é um terminal: redirecionada para arquivo ou pipe, imprime apenas o quadro final. */
void render_route(const Map *map, const Graph *graph, const SearchResult *result, const char *title,
                  bool animate);

/* Desenha o mapa final com as células que o algoritmo expandiu marcadas, além da rota. */
void render_exploration(const Map *map, const Graph *graph, const SearchResult *result, const char *title);

#endif
