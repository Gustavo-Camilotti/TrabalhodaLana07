#ifndef OCORRENCIAS_H
#define OCORRENCIAS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // para PRId64
#include "bplus.h"

// Estrutura de ocorrancia
typedef struct {
    int id_ocorrencia;            // Identificador anico da ocorrancia (PK)
    char data_ocorrencia[11];     // Data no formato AAAA-MM-DD
    char fonte_dados[100];        // Fonte do dado (artigo, banco de dados, etc.)
    char observador[100];         // Nome da pessoa que observou/registrou
    int id_planta;                // FK: ID da planta associada
} ocorrencia_t;

/* ========= OPERAaaES DE ALTO NaVEL ========= */

// Inseraao
__int64 salvar_ocorrencia(const ocorrencia_t *oc);

// Remoaao
int apagar_ocorrencia(__int64 *raiz_offset, int id_alvo);

// Busca
__int64 buscar_ocorrencia(FILE *fp_bplus, __int64 raiz_offset, int id_ocorrencia);

// Listagem
void listar_ocorrencias(__int64 raiz_offset);

// Ediaao
void editar_ocorrencia(__int64 raiz_offset, int id_alvo);

#endif
