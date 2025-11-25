#ifndef CLASSIFICACAO_H
#define CLASSIFICACAO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // para PRId64
#include "bplus.h"


typedef struct {
    int id_classificacao;       // Identificador anico da classificaaao
    char reino[30];             // Ex: Plantae
    char filo[30];              // Ex: Magnoliophyta
    char classe[30];            // Ex: Magnoliopsida
    char ordem[30];             // Ex: Rosales
    char familia[30];           // Ex: Rosaceae
    char genero[30];            // Ex: Rosa
    int id_planta;              // Chave estrangeira para planta_t
} classificacao_t;

__int64 salvar_classificacao(const classificacao_t *c);
void listar_classificacoes();
int apagar_classificacao(int id_alvo);

#endif
