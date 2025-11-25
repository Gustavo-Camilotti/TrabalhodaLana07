#ifndef INV_BIOMA_H
#define INV_BIOMA_H

#include <stdio.h>
#include <stdlib.h>
#include "bplus.h" // Para usar os #defines de FSEEK/FTELL corretos

// Estrutura que relaciona um Bioma a uma Planta
typedef struct {
    int id_bioma;
    int id_planta;
} inv_bioma_t;

// Adiciona uma nova relação ao arquivo invertido
// Deve ser chamada sempre que uma Distribuição for criada
int adicionar_indice_bioma(int id_bioma, int id_planta);

// Lista todas as plantas que pertencem a um determinado bioma
// Retorna a quantidade encontrada
int buscar_plantas_por_bioma(int id_bioma_alvo);

#endif