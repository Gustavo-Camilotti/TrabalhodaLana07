#include "inv_bioma.h"
#include "plantas.h" // Para poder buscar os detalhes da planta depois

int adicionar_indice_bioma(int id_bioma, int id_planta) {
    FILE *fp = fopen("data/inv_bioma.dat", "ab");
    if (fp == NULL) {
        // Tenta criar pasta data se não existir (embora já deva existir)
        fp = fopen("data/inv_bioma.dat", "wb");
        if (fp == NULL) return 0;
    }

    inv_bioma_t registro;
    registro.id_bioma = id_bioma;
    registro.id_planta = id_planta;

    fwrite(&registro, sizeof(inv_bioma_t), 1, fp);
    fclose(fp);
    return 1;
}

int buscar_plantas_por_bioma(int id_bioma_alvo) {
    FILE *fp_inv = fopen("data/inv_bioma.dat", "rb");
    if (fp_inv == NULL) {
        printf("Nenhum índice de biomas encontrado.\n");
        return 0;
    }

    // Abre também o arquivo de plantas para mostrar os nomes
    // (Isso evita carregar tudo na memória: lemos o ID no índice e buscamos o nome no arquivo de dados)
    FILE *fp_plantas = fopen("data/plantas.dat", "rb");
    
    // Carregamos a raiz da árvore de plantas para busca rápida
    __int64 raiz_plantas = carregar_raiz("plantas");

    inv_bioma_t reg;
    int count = 0;

    printf("\n--- Plantas encontradas no Bioma ID %d ---\n", id_bioma_alvo);

    while (fread(&reg, sizeof(inv_bioma_t), 1, fp_inv) == 1) {
        if (reg.id_bioma == id_bioma_alvo) {
            // Encontramos uma planta neste bioma!
            // Agora vamos buscar o nome dela usando a árvore B+
            
            if (raiz_plantas != -1 && fp_plantas != NULL) {
                __int64 offset = buscar_bplus("plantas", raiz_plantas, reg.id_planta);
                
                if (offset != -1) {
                    planta_t p;
                    if (FSEEK(fp_plantas, offset, SEEK_SET) == 0) {
                        fread(&p, sizeof(planta_t), 1, fp_plantas);
                        printf("-> [ID %d] %s (Nome Popular: %s)\n", 
                               p.id_planta, p.nome_cientifico, p.nome_popular);
                    }
                } else {
                    printf("-> [ID %d] (Dados da planta não encontrados)\n", reg.id_planta);
                }
            } else {
                printf("-> [ID %d]\n", reg.id_planta);
            }
            count++;
        }
    }

    if (fp_plantas) fclose(fp_plantas);
    fclose(fp_inv);

    if (count == 0) {
        printf("Nenhuma planta vinculada a este bioma.\n");
    } else {
        printf("Total encontrado: %d plantas.\n", count);
    }

    return count;
}