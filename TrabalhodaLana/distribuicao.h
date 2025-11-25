#ifndef CLASSIFICACAO_H
#define CLASSIFICACAO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // para PRId64
#include "bplus.h"

typedef struct {
    int id_distribuicao;         // Identificador �nico da distribui��o
    char continente[30];         // Continente onde a planta ocorre
    char pais[50];               // Pa�s onde a planta ocorre
    int id_bioma;                // Chave estrangeira para o bioma
    int id_planta;               // Chave estrangeira para a planta
} distribuicao_t;

__int64 salvar_distribuicao(const distribuicao_t *d);
void listar_distribuicoes();
int apagar_distribuicao(int id_alvo);

#endif
