#include <stdlib.h>
#include <stdio.h>
#include "bplus.h"

//#define ORDEM 4
#define TAM_PAGINA 6

extern long raiz_offset; // variavel global

typedef struct {
    int id_planta;
    char nome_cientifico[100];
    char nome_popular[100];
    char tipo[50];
    char status_conservacao[50];
    char descricao[200];
    char data_registro[11]; // "AAAA-MM-DD"
} planta_t;

typedef struct {
    char nome_popular[100];
    long offset; // posicao no plantas.dat
} indice_nome_t;

// Prototipos atualizados
__int64 salvar_planta(const planta_t *planta);
void listar_plantas(const char* entidade, __int64 raiz_offset);
void editar_planta(const char* entidade, __int64 raiz_offset, int id_alvo);
int apagar_planta(const char* entidade, __int64 *raiz_offset, int id_alvo);
void buscar_planta(FILE *fp_bplus, __int64 raiz_offset, int id_alvo);

int comparar_nome_popular(const void *a, const void *b);
void exibir_nomes_ordenados(int pagina);

int comparar_nome_cientifico(const void *a, const void *b);
void exibir_nomes_cientificos(int pagina);

int comparar_data_registro(const void *a, const void *b);
void exibir_por_data(int pagina);

void navegar_nomes_populares();
void salvar_indice_nome_popular(const char *nome, long offset);
int comparar_indice_nome(const void *a, const void *b);
void navegar_por_nome_popular_indexado();
