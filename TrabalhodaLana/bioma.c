#include "bioma.h"

__int64 salvar_bioma(const bioma_t *b) {
    FILE *fp = fopen("data/biomas.dat", "ab");
    if (fp == NULL) {
        perror("Erro ao abrir biomas.dat");
        return -1;
    }

    if (FSEEK(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    __int64 offset = FTELL(fp);

    if (fwrite(b, sizeof(bioma_t), 1, fp) != 1) {
        perror("Erro ao salvar bioma");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    printf("Bioma salvo com sucesso (ID %d, offset %" PRId64 ").\n", b->id_bioma, offset);
    return offset;
}

void listar_biomas() {
    FILE *fp = fopen("data/biomas.dat", "rb");
    if (fp == NULL) {
        perror("Erro ao abrir biomas.dat");
        return;
    }

    bioma_t b;
    int count = 0;

    printf("\n--- Lista de Biomas ---\n");

    while (fread(&b, sizeof(bioma_t), 1, fp) == 1) {
        if (b.id_bioma == -1) continue; // ignorar apagados

        printf("\n===== Bioma %d =====\n", b.id_bioma);
        printf("Nome: %s\n", b.nome_bioma);
        printf("Vegeta��o: %s\n", b.tipo_vegetacao);
        printf("Clima: %s\n", b.clima);
        printf("Descri��o: %s\n", b.descricao);
        count++;
    }

    fclose(fp);
    printf("\nTotal de biomas exibidos: %d\n", count);
}

int apagar_bioma(int id_alvo) {
    FILE *fp = fopen("data/biomas.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir biomas.dat");
        return -1;
    }

    bioma_t b;
    __int64 offset = 0;

    while (fread(&b, sizeof(bioma_t), 1, fp) == 1) {
        if (b.id_bioma == id_alvo) {
            b.id_bioma = -1;
            FSEEK(fp, offset, SEEK_SET);
            fwrite(&b, sizeof(bioma_t), 1, fp);
            fclose(fp);
            printf("Bioma %d apagado com sucesso.\n", id_alvo);
            return 0;
        }
        offset += sizeof(bioma_t);
    }

    fclose(fp);
    printf("Bioma com ID %d n�o encontrado.\n", id_alvo);
    return -1;
}

