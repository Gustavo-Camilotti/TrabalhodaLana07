#include "classificacao.h"

__int64 salvar_classificacao(const classificacao_t *c) {
    FILE *fp = fopen("data/classificacoes.dat", "ab");
    if (fp == NULL) {
        perror("Erro ao abrir classificacoes.dat");
        return -1;
    }

    if (FSEEK(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    __int64 offset = FTELL(fp);

    if (fwrite(c, sizeof(classificacao_t), 1, fp) != 1) {
        perror("Erro ao salvar classifica��o");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    printf("Classifica��o salva com sucesso (ID %d, offset %" PRId64 ").\n",
           c->id_classificacao, offset);
    return offset;
}

void listar_classificacoes() {
    FILE *fp = fopen("data/classificacoes.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir classificacoes.dat");
        return;
    }

    classificacao_t c;
    int count = 0;

    printf("\n--- Lista de Classifica��es Bot�nicas ---\n");

    while (fread(&c, sizeof(classificacao_t), 1, fp) == 1) {
        if (c.id_classificacao == -1) continue; // ignorar apagados

        printf("\n===== Classifica��o %d =====\n", c.id_classificacao);
        printf("Reino: %s\n", c.reino);
        printf("Filo: %s\n", c.filo);
        printf("Classe: %s\n", c.classe);
        printf("Ordem: %s\n", c.ordem);
        printf("Fam�lia: %s\n", c.familia);
        printf("G�nero: %s\n", c.genero);
        printf("ID Planta: %d\n", c.id_planta);
        count++;
    }

    fclose(fp);
    printf("\nTotal de classifica��es exibidas: %d\n", count);
}

int apagar_classificacao(int id_alvo) {
    FILE *fp = fopen("data/classificacoes.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir classificacoes.dat");
        return -1;
    }

    classificacao_t c;
    __int64 offset = 0;

    while (fread(&c, sizeof(classificacao_t), 1, fp) == 1) {
        if (c.id_classificacao == id_alvo) {
            c.id_classificacao = -1;
            FSEEK(fp, offset, SEEK_SET);
            fwrite(&c, sizeof(classificacao_t), 1, fp);
            fclose(fp);
            printf("Classifica��o %d apagada com sucesso.\n", id_alvo);
            return 0;
        }
        offset += sizeof(classificacao_t);
    }

    fclose(fp);
    printf("Classifica��o com ID %d n�o encontrada.\n", id_alvo);
    return -1;
}
