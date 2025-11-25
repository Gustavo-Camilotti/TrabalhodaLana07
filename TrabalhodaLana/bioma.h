#ifndef BIOMA_H
#define BIOMA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // para PRId64
#include "bplus.h"

typedef struct {
    int id_bioma;                // Identificador �nico do bioma
    char nome_bioma[50];        // Nome do bioma (ex: Amaz�nia, Cerrado)
    char tipo_vegetacao[50];    // Tipo predominante de vegeta��o (ex: Floresta, Savana)
    char clima[30];             // Clima predominante (ex: Equatorial, Tropical)
    char descricao[200];        // Descri��o geral do bioma
} bioma_t;

__int64 salvar_bioma(const bioma_t *b);
void listar_biomas();
int apagar_bioma(int id_alvo);

#endif
