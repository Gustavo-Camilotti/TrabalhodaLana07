#include "distribuicao.h"

__int64 salvar_distribuicao(const distribuicao_t *d) {
    FILE *fp = fopen("data/distribuicoes.dat", "ab");
    if (fp == NULL) {
        perror("Erro ao abrir distribuicoes.dat");
        return -1;
    }

    if (FSEEK(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    __int64 offset = FTELL(fp);

    if (fwrite(d, sizeof(distribuicao_t), 1, fp) != 1) {
        perror("Erro ao salvar distribuicao");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    printf("Distribuicao salva com sucesso (ID %d, offset %" PRId64 ").\n",
           d->id_distribuicao, offset);
    return offset;
}

void listar_distribuicoes() {
    FILE *fp = fopen("data/distribuicoes.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir distribuicoes.dat");
        return;
    }

    distribuicao_t d;
    int count = 0;

    printf("\n--- Lista de Distribuicoes Geograficas ---\n");

    while (fread(&d, sizeof(distribuicao_t), 1, fp) == 1) {
        if (d.id_distribuicao == -1) continue; // ignorar apagados

        printf("\n===== Distribuicao %d =====\n", d.id_distribuicao);
        printf("Continente: %s\n", d.continente);
        printf("Pais: %s\n", d.pais);
        printf("ID Bioma: %d\n", d.id_bioma);
        printf("ID Planta: %d\n", d.id_planta);
        count++;
    }

    fclose(fp);
    printf("\nTotal de distribuicoes exibidas: %d\n", count);
}

int apagar_distribuicao(int id_alvo) {
    FILE *fp = fopen("data/distribuicoes.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir distribuicoes.dat");
        return -1;
    }

    distribuicao_t d;
    __int64 offset = 0;

    while (fread(&d, sizeof(distribuicao_t), 1, fp) == 1) {
        if (d.id_distribuicao == id_alvo) {
            d.id_distribuicao = -1;
            FSEEK(fp, offset, SEEK_SET);
            fwrite(&d, sizeof(distribuicao_t), 1, fp);
            fclose(fp);
            printf("Distribuicao %d apagada com sucesso.\n", id_alvo);
            return 0;
        }
        offset += sizeof(distribuicao_t);
    }

    fclose(fp);
    printf("Distribuicao com ID %d nao encontrada.\n", id_alvo);
    return -1;
}
