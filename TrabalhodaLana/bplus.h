#ifndef BPLUS_H
#define BPLUS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // para PRId64

#define _fseeki64 fseek
#define _ftelli64 ftell

#define FSEEK fseek
#define FTELL ftell

#define ORDEM 4

// Estrutura de n� da �rvore B+
typedef struct {
    int eh_folha;             // 1 se for folha, 0 se for n� interno
    int num_chaves;           // n�mero atual de chaves no n�
    int chaves[ORDEM];        // vetor de chaves
    __int64 offsets[ORDEM];   // offsets no arquivo plantas.dat (v�lido s� em folhas)
    __int64 filhos[ORDEM+1];  // offsets dos filhos (v�lido s� em n�s internos)
    __int64 proximo;          // usado em folhas para encadear
} bplus_no_t;

/* ========= OPERA��ES DE ALTO N�VEL ========= */

// Inser��o
int inserir_bplus(const char* entidade, __int64* raiz_offset, int id, __int64 dado_offset);
__int64 salvar_na_arvore_bplus(const char* entidade, int id, __int64 offset, __int64 raiz_offset);

// Remo��o
__int64 remover_bplus(const char* entidade, __int64 raiz_offset, int id);

// Busca
__int64 buscar_bplus(const char* entidade, __int64 raiz_offset, int id);

// Impress�o e navega��o
void imprimir_bplus(const char* entidade, __int64 raiz_offset, int nivel);
void listar_bplus_pagina(const char* entidade, __int64 raiz_offset, int tamanho_pagina, int pagina_desejada);
void listar_bplus_pagina_total(const char* entidade, __int64 raiz_offset, int tamanho_pagina, int pagina_desejada);

// Estat�sticas
int contar_registros_bplus(const char* entidade, __int64 raiz_offset);

/* ========= UTILIT�RIOS DE N� ========= */

int carregar_no(const char* entidade, __int64 offset, bplus_no_t *no_out);
int salvar_no(const char* entidade, __int64 offset, const bplus_no_t *no);
__int64 criar_no_folha(const char* entidade);
__int64 criar_no_interno(const char* entidade);

/* ========= PERSIST�NCIA ========= */

FILE* fp_bplus(const char* entidade);
void salvar_raiz(__int64 raiz_offset, const char* entidade);
__int64 carregar_raiz(const char* entidade);
void fechar_bplus(const char* entidade);

void editar_campo_string(const char *rotulo, char *campo, int tamanho);

#endif
