#include "ocorrencias.h"

#define OCORRENCIAS_POR_PAGINA 5

__int64 salvar_ocorrencia(const ocorrencia_t *oc) {
    FILE *fp_dados = fopen("data/ocorrencias.dat", "ab");
    if (fp_dados == NULL) {
        perror("Erro ao abrir ocorrencias.dat");
        return -1;
    }

    if (FSEEK(fp_dados, 0, SEEK_END) != 0) {
        fclose(fp_dados);
        return -1;
    }
    __int64 offset = FTELL(fp_dados);

    if (fwrite(oc, sizeof(ocorrencia_t), 1, fp_dados) != 1) {
        perror("Erro ao salvar ocorrencia");
        fclose(fp_dados);
        return -1;
    }
    fclose(fp_dados);

    // Atualiza andice B+ de ocorrencias
    __int64 raiz_offset = carregar_raiz("ocorrencias");
    raiz_offset = salvar_na_arvore_bplus("ocorrencias", oc->id_ocorrencia, offset, raiz_offset);
    salvar_raiz(raiz_offset, "ocorrencias");

    printf("Ocorrencia salva com sucesso (ID %d, offset %" PRId64 ").\n",
           oc->id_ocorrencia, offset);

    return offset;
}

void listar_ocorrencias(__int64 raiz_offset) {
    if (raiz_offset == -1) {
        printf("Nenhuma ocorrencia cadastrada.\n");
        return;
    }

    bplus_no_t no;
    __int64 offset = raiz_offset;

    // Vai ata a folha mais a esquerda
    while (1) {
        if (!carregar_no("ocorrencias", offset, &no)) {
            printf("Erro ao carregar no.\n");
            return;
        }
        if (no.eh_folha) break;
        offset = no.filhos[0];
    }

    FILE *fp_dados = fopen("data/ocorrencias.dat", "rb");
    if (fp_dados == NULL) {
        perror("Erro ao abrir ocorrencias.dat");
        return;
    }

    int exibidos = 0;
    int pagina = 1;
    char comando[8];

    printf("\n--- Lista de ocorrencias (ordenadas por ID) ---\n");

    // Percorre folhas encadeadas
    while (offset != -1) {
        if (!carregar_no("ocorrencias", offset, &no)) {
            printf("Erro ao carregar no.\n");
            break;
        }

        for (int i = 0; i < no.num_chaves; i++) {
            if (no.offsets[i] < 0) continue; // proteaao

            ocorrencia_t oc;
            if (FSEEK(fp_dados, no.offsets[i], SEEK_SET) != 0) continue;
            if (fread(&oc, sizeof(ocorrencia_t), 1, fp_dados) != 1) continue;

            // Ignora registros apagados
            if (oc.id_ocorrencia == -1) continue;

            printf("\n===== ocorrencia %d =====\n", oc.id_ocorrencia);
            printf("Data: %s\n", oc.data_ocorrencia);
            printf("Fonte: %s\n", oc.fonte_dados);
            printf("Observador: %s\n", oc.observador);
            printf("ID Planta: %d\n", oc.id_planta);

            exibidos++;

            // Controle de paginaaao
            if (exibidos % OCORRENCIAS_POR_PAGINA == 0) {
                printf("---- Pagina %d concluida ----\n", pagina);
                printf("Pressione ENTER para continuar ou Q para sair...\n");

                fgets(comando, sizeof(comando), stdin);
                if (comando[0] == 'q' || comando[0] == 'Q') {
                    fclose(fp_dados);
                    printf("\nTotal de registros exibidos: %d\n", exibidos);
                    return;
                }
                pagina++;
            }
        }

        offset = no.proximo; // vai para praxima folha
    }

    fclose(fp_dados);

    printf("\nTotal de registros exibidos: %d\n", exibidos);
}

int apagar_ocorrencia(__int64 *raiz_offset, int id_alvo) {
    FILE *fp = fopen("data/ocorrencias.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir o arquivo ocorrencias.dat");
        return -1;
    }

    // Busca no andice
    __int64 offset = buscar_bplus("ocorrencias", *raiz_offset, id_alvo);
    if (offset == -1) {
        printf("ocorrencia com ID %d nao encontrada.\n", id_alvo);
        fclose(fp);
        return -1;
    }

    ocorrencia_t oc;
    if (FSEEK(fp, offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&oc, sizeof(ocorrencia_t), 1, fp) != 1) { fclose(fp); return -1; }

    // Marca como apagada
    oc.id_ocorrencia = -1;
    if (FSEEK(fp, offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fwrite(&oc, sizeof(ocorrencia_t), 1, fp) != 1) {
        perror("Erro ao atualizar ocorrencia");
        fclose(fp);
        return -1;
    }

    // Remove do andice
    *raiz_offset = remover_bplus("ocorrencias", *raiz_offset, id_alvo);
    salvar_raiz(*raiz_offset, "ocorrencias");

    if (*raiz_offset == -1) {
        printf("Arquivo e indice de ocorrencias agora estao vazios.\n");
    } else {
        printf("ocorrencia removida com sucesso.\n");
    }

    fclose(fp);
    return 0;
}


void editar_ocorrencia(__int64 raiz_offset, int id_alvo) {
    FILE *fp = fopen("data/ocorrencias.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir o arquivo ocorrencias.dat");
        return;
    }

    // Busca offset da ocorrencia pelo andice de ocorrencias
    __int64 offset = buscar_bplus("ocorrencias", raiz_offset, id_alvo);
    if (offset == -1) {
        printf("ocorrencia com ID %d nao encontrada.\n", id_alvo);
        fclose(fp);
        return;
    }

    ocorrencia_t oc;
    if (FSEEK(fp, offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&oc, sizeof(ocorrencia_t), 1, fp) != 1) { fclose(fp); return; }

    printf("\n--- ocorrencia encontrada ---\n");

    editar_campo_string("Data da ocorrencia (AAAA-MM-DD)", oc.data_ocorrencia, sizeof(oc.data_ocorrencia));
    editar_campo_string("Fonte dos Dados", oc.fonte_dados, sizeof(oc.fonte_dados));
    editar_campo_string("Observador", oc.observador, sizeof(oc.observador));
    // Se quiser, pode tambam pedir novo ID da planta associada (int)

    if (FSEEK(fp, offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fwrite(&oc, sizeof(ocorrencia_t), 1, fp) != 1) {
        perror("Erro ao atualizar ocorrencia");
    } else {
        printf("ocorrencia atualizada com sucesso.\n");
    }

    fclose(fp);
}
