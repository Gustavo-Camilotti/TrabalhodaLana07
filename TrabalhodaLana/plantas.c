#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plantas.h"

//#define ORDEM 4

__int64 salvar_planta(const planta_t *planta) {
    FILE *fp_dados = fopen("data/plantas.dat", "ab");
    if (fp_dados == NULL) {
        perror("Erro ao abrir plantas.dat");
        return -1;
    }

    if (_fseeki64(fp_dados, 0, SEEK_END) != 0) {
        fclose(fp_dados);
        return -1;
    }
    __int64 offset = _ftelli64(fp_dados);

    if (fwrite(planta, sizeof(planta_t), 1, fp_dados) != 1) {
        perror("Erro ao salvar planta");
        fclose(fp_dados);
        return -1;
    }
    fclose(fp_dados);

    // Inserir na árvore B+ de plantas
    FILE *fp_idx = fp_bplus("plantas");
    __int64 raiz_offset = carregar_raiz("plantas");
    raiz_offset = salvar_na_arvore_bplus("plantas", planta->id_planta, offset, raiz_offset);
    salvar_raiz(raiz_offset, "plantas");
    fclose(fp_idx);

    return offset;
}

void listar_plantas(const char* entidade, __int64 raiz_offset) {
    FILE *fp_dados = fopen("data/plantas.dat", "rb");
    if (fp_dados == NULL) {
        perror("Erro ao abrir o arquivo plantas.dat");
        return;
    }

    if (raiz_offset == -1) {
        printf("Nenhuma planta cadastrada.\n");
        fclose(fp_dados);
        return;
    }

    FILE *fp_idx = fp_bplus(entidade);  // abre índice correto
    bplus_no_t no;

    if (_fseeki64(fp_idx, raiz_offset, SEEK_SET) != 0) { fclose(fp_dados); fclose(fp_idx); return; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp_idx) != 1) { fclose(fp_dados); fclose(fp_idx); return; }

    // Navega até a folha mais à esquerda
    while (!no.eh_folha) {
        if (_fseeki64(fp_idx, no.filhos[0], SEEK_SET) != 0) { fclose(fp_dados); fclose(fp_idx); return; }
        if (fread(&no, sizeof(bplus_no_t), 1, fp_idx) != 1) { fclose(fp_dados); fclose(fp_idx); return; }
    }

    int contador = 0;
    int pagina = 1;

    printf("\n--- Lista de Plantas (ordenadas por ID) ---\n");

    // Percorre folhas encadeadas
    while (1) {
        for (int i = 0; i < no.num_chaves; i++) {
            __int64 offset = no.offsets[i];
            if (offset < 0) continue;

            planta_t p;
            if (_fseeki64(fp_dados, offset, SEEK_SET) != 0) continue;
            if (fread(&p, sizeof(planta_t), 1, fp_dados) != 1) continue;

            if (p.id_planta == -1) continue;

            printf("ID: %d\n", p.id_planta);
            printf("Nome Científico: %s\n", p.nome_cientifico);
            printf("Nome Popular: %s\n", p.nome_popular);
            printf("Tipo: %s\n", p.tipo);
            printf("Status de Conservação: %s\n", p.status_conservacao);
            printf("Descrição: %s\n", p.descricao);
            printf("Data de Registro: %s\n", p.data_registro);
            printf("------------------------\n");

            contador++;

            if (contador % TAM_PAGINA == 0) {
                printf("---- Página %d concluída ----\n", pagina);
                pagina++;
                printf("Pressione ENTER para continuar ou Q para sair...\n");

                int c = getchar();
                if (c == 'q' || c == 'Q') { fclose(fp_dados); fclose(fp_idx); return; }
            }
        }

        if (no.proximo == -1) break;

        if (_fseeki64(fp_idx, no.proximo, SEEK_SET) != 0) break;
        if (fread(&no, sizeof(bplus_no_t), 1, fp_idx) != 1) break;
    }

    if (contador % TAM_PAGINA != 0) {
        printf("---- Página %d concluída ----\n", pagina);
    }

    printf("Total de registros exibidos: %d\n", contador);

    fclose(fp_dados);
    fclose(fp_idx);
}

void editar_planta(const char* entidade, __int64 raiz_offset, int id_alvo) {
    FILE *fp = fopen("data/plantas.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir o arquivo plantas.dat");
        return;
    }

    // Busca offset da planta pelo índice de plantas
    __int64 offset = buscar_bplus(entidade, raiz_offset, id_alvo);
    if (offset == -1) {
        printf("Planta com ID %d não encontrada.\n", id_alvo);
        fclose(fp);
        return;
    }

    planta_t p;
    if (_fseeki64(fp, offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&p, sizeof(planta_t), 1, fp) != 1) { fclose(fp); return; }

    printf("\n--- Planta encontrada ---\n");

    editar_campo_string("Nome Científico", p.nome_cientifico, sizeof(p.nome_cientifico));
    editar_campo_string("Nome Popular", p.nome_popular, sizeof(p.nome_popular));
    editar_campo_string("Tipo", p.tipo, sizeof(p.tipo));
    editar_campo_string("Status de Conservação", p.status_conservacao, sizeof(p.status_conservacao));
    editar_campo_string("Descrição", p.descricao, sizeof(p.descricao));
    editar_campo_string("Data de Registro (AAAA-MM-DD)", p.data_registro, sizeof(p.data_registro));

    if (_fseeki64(fp, offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fwrite(&p, sizeof(planta_t), 1, fp) != 1) {
        perror("Erro ao atualizar planta");
    } else {
        printf("Planta atualizada com sucesso.\n");
    }

    fclose(fp);
}

int comparar_nome_popular(const void *a, const void *b) {
    const planta_t *p1 = (const planta_t *)a;
    const planta_t *p2 = (const planta_t *)b;
    return strcmp(p1->nome_popular, p2->nome_popular);
}

void exibir_nomes_ordenados(int pagina) {
    FILE *fp = fopen("data/plantas.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir plantas.dat");
        return;
    }

    // Carrega todas as plantas
    planta_t *plantas = NULL;
    int total = 0;
    planta_t temp;

    while (fread(&temp, sizeof(planta_t), 1, fp) == 1) {
        plantas = realloc(plantas, (total + 1) * sizeof(planta_t));
        plantas[total++] = temp;
    }
    fclose(fp);

    // Ordena por nome popular
    qsort(plantas, total, sizeof(planta_t), comparar_nome_popular);

    // Paginação: TAM_PAGINA por página
    int inicio = pagina * TAM_PAGINA;
    int fim = inicio + TAM_PAGINA;
    if (inicio >= total) {
        printf("Página %d está fora do intervalo. Total de plantas: %d\n", pagina, total);
        free(plantas);
        return;
    }
    if (fim > total) fim = total;

    printf("Página %d — nomes usuais das plantas:\n", pagina);
    for (int i = inicio; i < fim; i++) {
        printf("• %s\n", plantas[i].nome_popular);
    }

    free(plantas);
}

int comparar_nome_cientifico(const void *a, const void *b) {
    const planta_t *p1 = (const planta_t *)a;
    const planta_t *p2 = (const planta_t *)b;
    return strcmp(p1->nome_cientifico, p2->nome_cientifico);
}

void exibir_nomes_cientificos(int pagina) {
    FILE *fp = fopen("data/plantas.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir plantas.dat");
        return;
    }

    // Carrega todas as plantas
    planta_t *plantas = NULL;
    int total = 0;
    planta_t temp;

    while (fread(&temp, sizeof(planta_t), 1, fp) == 1) {
        plantas = realloc(plantas, (total + 1) * sizeof(planta_t));
        plantas[total++] = temp;
    }
    fclose(fp);

    // Ordena por nome científico
    qsort(plantas, total, sizeof(planta_t), comparar_nome_cientifico);

    // Paginação: TAM_PAGINA por página
    int inicio = pagina * TAM_PAGINA;
    int fim = inicio + TAM_PAGINA;
    if (inicio >= total) {
        printf("Página %d está fora do intervalo. Total de plantas: %d\n", pagina, total);
        free(plantas);
        return;
    }
    if (fim > total) fim = total;

    printf("Página %d — nomes científicos das plantas:\n", pagina);
    for (int i = inicio; i < fim; i++) {
        printf("• %s\n", plantas[i].nome_cientifico);
    }

    free(plantas);
}

int apagar_planta(const char* entidade, __int64 *raiz_offset, int id_alvo) {
    FILE *fp = fopen("data/plantas.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir o arquivo plantas.dat");
        return -1;
    }

    // Busca offset da planta pelo índice
    __int64 offset = buscar_bplus(entidade, *raiz_offset, id_alvo);
    if (offset < 0) {
        printf("Planta com ID %d não encontrada ou offset inválido.\n", id_alvo);
        fclose(fp);
        return -1;
    }

    planta_t p;
    if (_fseeki64(fp, offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&p, sizeof(planta_t), 1, fp) != 1) { fclose(fp); return -1; }

    // Marca como apagado
    p.id_planta = -1;
    if (_fseeki64(fp, offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fwrite(&p, sizeof(planta_t), 1, fp) != 1) {
        perror("Erro ao atualizar planta");
        fclose(fp);
        return -1;
    }

    // Remove do índice
    __int64 novo_raiz = remover_bplus(entidade, *raiz_offset, id_alvo);
    if (novo_raiz < -1) {
        printf("Erro ao remover do índice.\n");
        fclose(fp);
        return -1;
    }
    *raiz_offset = novo_raiz;
    salvar_raiz(*raiz_offset, entidade);

    if (*raiz_offset == -1) {
        printf("Arquivo e índice de plantas agora estão vazios.\n");
    } else {
        printf("Planta removida com sucesso.\n");
    }

    fclose(fp);
    return 0;
}


void buscar_planta(FILE *fp_bplus, __int64 raiz_offset, int id_alvo) {
    FILE *fp = fopen("data/plantas.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir o arquivo plantas.dat");
        return;
    }

    if (raiz_offset == -1) {
        printf("Nenhuma planta cadastrada.\n");
        fclose(fp);
        return;
    }

    __int64 offset = buscar_bplus("plantas", raiz_offset, id_alvo);
    if (offset == -1 || offset < 0) {
        printf("Planta com ID %d não encontrada.\n", id_alvo);
        fclose(fp);
        return;
    }

    planta_t p;
    if (_fseeki64(fp, offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&p, sizeof(planta_t), 1, fp) != 1) {
        printf("Erro ao ler registro da planta no arquivo.\n");
        fclose(fp);
        return;
    }

    if (p.id_planta == -1) {
        printf("Planta com ID %d foi removida.\n", id_alvo);
        fclose(fp);
        return;
    }

    printf("\n--- Planta encontrada ---\n");
    printf("ID: %d\n", p.id_planta);
    printf("Nome Científico: %s\n", p.nome_cientifico);
    printf("Nome Popular: %s\n", p.nome_popular);
    printf("Tipo: %s\n", p.tipo);
    printf("Status de Conservação: %s\n", p.status_conservacao);
    printf("Descrição: %s\n", p.descricao);
    printf("Data de Registro: %s\n", p.data_registro);
    printf("------------------------\n");

    fclose(fp);
}


int comparar_data_registro(const void *a, const void *b) {
    const planta_t *p1 = (const planta_t *)a;
    const planta_t *p2 = (const planta_t *)b;
    return strcmp(p1->data_registro, p2->data_registro);
}

void exibir_por_data(int pagina) {
    FILE *fp = fopen("data/plantas.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir plantas.dat");
        return;
    }

    planta_t *plantas = NULL;
    int total = 0;
    planta_t temp;

    while (fread(&temp, sizeof(planta_t), 1, fp) == 1) {
        plantas = realloc(plantas, (total + 1) * sizeof(planta_t));
        plantas[total++] = temp;
    }
    fclose(fp);

    qsort(plantas, total, sizeof(planta_t), comparar_data_registro);

    int inicio = pagina * TAM_PAGINA;
    int fim = inicio + TAM_PAGINA;
    if (inicio >= total) {
        printf("Página %d está fora do intervalo. Total de plantas: %d\n", pagina, total);
        free(plantas);
        return;
    }
    if (fim > total) fim = total;

    printf("Página %d — plantas ordenadas por data de registro:\n", pagina);
    for (int i = inicio; i < fim; i++) {
        printf("• %s — %s\n", plantas[i].nome_popular, plantas[i].data_registro);
    }

    free(plantas);
}

void navegar_nomes_populares() {
    FILE *fp = fopen("data/plantas.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir plantas.dat");
        return;
    }

    planta_t *plantas = NULL;
    int total = 0;
    planta_t temp;

    while (fread(&temp, sizeof(planta_t), 1, fp) == 1) {
        plantas = realloc(plantas, (total + 1) * sizeof(planta_t));
        plantas[total++] = temp;
    }
    fclose(fp);

    qsort(plantas, total, sizeof(planta_t), comparar_nome_popular);

    int pagina = 0;
    char entrada[10];

    while (1) {
        int inicio = pagina * TAM_PAGINA;
        int fim = inicio + TAM_PAGINA;
        if (inicio >= total) {
            printf("Fim da lista.\n");
            break;
        }
        if (fim > total) fim = total;

        printf("\nPágina %d:\n", pagina);
        for (int i = inicio; i < fim; i++) {
            printf("- %s\n", plantas[i].nome_popular);
        }

        printf("\n[Enter] próxima | [número] ir para página | [s] sair: ");
        fgets(entrada, sizeof(entrada), stdin);
        entrada[strcspn(entrada, "\n")] = '\0'; // remove '\n'

        if (strlen(entrada) == 0) {
            pagina++;
        } else if (entrada[0] == 's' || entrada[0] == 'S') {
            break;
        } else {
            int nova_pagina = atoi(entrada);
            if (nova_pagina >= 0 && nova_pagina * TAM_PAGINA < total) {
                pagina = nova_pagina;
            } else {
                printf("Página inválida.\n");
            }
        }
    }

    free(plantas);
}

void salvar_indice_nome_popular(const char *nome, long offset) {
    FILE *fp = fopen("data/index_nome_popular.idx", "ab");
    if (fp == NULL) {
        perror("Erro ao abrir index_nome_popular.idx");
        return;
    }

    indice_nome_t entrada;
    strncpy(entrada.nome_popular, nome, sizeof(entrada.nome_popular));
    entrada.nome_popular[sizeof(entrada.nome_popular) - 1] = '\0';
    entrada.offset = offset;

    fwrite(&entrada, sizeof(indice_nome_t), 1, fp);
    fclose(fp);
}

int comparar_indice_nome(const void *a, const void *b) {
    const indice_nome_t *i1 = (const indice_nome_t *)a;
    const indice_nome_t *i2 = (const indice_nome_t *)b;
    return strcmp(i1->nome_popular, i2->nome_popular);
}

void navegar_por_nome_popular_indexado() {
    FILE *fp_idx = fopen("data/index_nome_popular.idx", "rb");
    if (fp_idx == NULL) {
        perror("Erro ao abrir index_nome_popular.idx");
        return;
    }

    indice_nome_t *indices = NULL;
    int total = 0;
    indice_nome_t temp;

    while (fread(&temp, sizeof(indice_nome_t), 1, fp_idx) == 1) {
        indices = realloc(indices, (total + 1) * sizeof(indice_nome_t));
        indices[total++] = temp;
    }
    fclose(fp_idx);

    qsort(indices, total, sizeof(indice_nome_t), comparar_indice_nome);

    FILE *fp_data = fopen("data/plantas.dat", "rb");
    if (fp_data == NULL) {
        perror("Erro ao abrir plantas.dat");
        free(indices);
        return;
    }

    int pagina = 0;
    char entrada[10];
    planta_t planta;

    while (1) {
        int inicio = pagina * TAM_PAGINA;
        int fim = inicio + TAM_PAGINA;
        if (inicio >= total) {
            printf("Fim da lista.\n");
            break;
        }
        if (fim > total) fim = total;

        printf("\n Página %d:\n", pagina);
        for (int i = inicio; i < fim; i++) {
            fseek(fp_data, indices[i].offset, SEEK_SET);
            fread(&planta, sizeof(planta_t), 1, fp_data);

            printf("\n%s\n", planta.nome_popular);
            printf("ID: %05d\n", planta.id_planta);
            printf("Nome científico: %s\n", planta.nome_cientifico);
            printf("Tipo: %s\n", planta.tipo);
            printf("Status de conservação: %s\n", planta.status_conservacao);
            printf("Descrição: %s\n", planta.descricao);
            printf("Data de registro: %s\n", planta.data_registro);
        }

        printf("\n[Enter] próxima | [número] ir para página | [s] sair: ");
        fgets(entrada, sizeof(entrada), stdin);
        entrada[strcspn(entrada, "\n")] = '\0';

        if (strlen(entrada) == 0) {
            pagina++;
        } else if (entrada[0] == 's' || entrada[0] == 'S') {
            break;
        } else {
            int nova_pagina = atoi(entrada);
            if (nova_pagina >= 0 && nova_pagina * TAM_PAGINA < total) {
                pagina = nova_pagina;
            } else {
                printf("Página inválida.\n");
            }
        }
    }

    fclose(fp_data);
    free(indices);
}








//funcao inutilizada e errada!!!!!
/**long localizar_folha(int id) {
    FILE *fp = fopen("data/index_id.idx", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir index_id.idx");
        exit(1);
    }

    long offset = 0;
    bplus_no_t no;
    fread(&no, sizeof(bplus_no_t), 1, fp);

    while (!no.eh_folha) {
        int i = 0;
        while (i < no.num_chaves && id >= no.chaves[i]) {
            i++;
        }
        offset = no.filhos[i];
        fseek(fp, offset, SEEK_SET);
        fread(&no, sizeof(bplus_no_t), 1, fp);
    }

    fclose(fp);
    return offset;
}**/







