#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plantas.h"
#include "ocorrencias.h"
#include "bplus.h"

// Função auxiliar para ler string com segurança
void ler_string(char *dest, int tamanho) {
    if (dest == NULL || tamanho <= 0) return;

    // Lê a linha do stdin
    if (fgets(dest, tamanho, stdin) != NULL) {
        // Remove o '\n' caso tenha sido lido
        dest[strcspn(dest, "\n")] = '\0';
    } else {
        // Em caso de erro, garante string vazia
        dest[0] = '\0';
    }
}


int main() {
    // Índice de plantas
    FILE *fp_idx_plantas = fp_bplus("plantas");
    __int64 raiz_offset_plantas = carregar_raiz("plantas");

    // Índice de ocorrências
    FILE *fp_idx_ocorrencias = fp_bplus("ocorrencias");
    __int64 raiz_offset_ocorrencias = carregar_raiz("ocorrencias");

    int opcao;
    do {
        printf("\n===== MENU PRINCIPAL =====\n");
        printf("1. Adicionar planta\n");
        printf("2. Listar plantas\n");
        printf("3. Editar planta\n");
        printf("4. Apagar planta\n");
        printf("5. Adicionar ocorrência\n");
        printf("6. Listar ocorrências\n");
        printf("7. Editar ocorrência\n");
        printf("8. Apagar ocorrência\n");
        printf("0. Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);
        getchar(); // consome \n

        switch (opcao) {
            case 1: {
                planta_t p;
                printf("Digite o ID da planta: ");
                scanf("%d", &p.id_planta);
                getchar();

                printf("Nome científico: ");
                ler_string(p.nome_cientifico, sizeof(p.nome_cientifico));
                printf("Nome popular: ");
                ler_string(p.nome_popular, sizeof(p.nome_popular));
                printf("Tipo: ");
                ler_string(p.tipo, sizeof(p.tipo));
                printf("Status de conservação: ");
                ler_string(p.status_conservacao, sizeof(p.status_conservacao));
                printf("Descrição: ");
                ler_string(p.descricao, sizeof(p.descricao));
                printf("Data de registro (AAAA-MM-DD): ");
                ler_string(p.data_registro, sizeof(p.data_registro));

                __int64 offset = salvar_planta(&p);
                if (offset != -1) {
                    printf("Planta salva com sucesso no offset %" PRId64 "\n", offset);
                    raiz_offset_plantas = carregar_raiz("plantas");
                }
                break;
            }

            case 2:
                listar_plantas("plantas", raiz_offset_plantas);
                break;

            case 3: {
                int id;
                printf("Digite o ID da planta que deseja editar: ");
                scanf("%d", &id);

                getchar();
                editar_planta("plantas", raiz_offset_plantas, id);
                break;
            }

            case 4: {
                int id;
                printf("Digite o ID da planta que deseja apagar: ");
                scanf("%d", &id);
                getchar();
                apagar_planta("plantas", &raiz_offset_plantas, id);
                break;
            }

            case 5: {
                ocorrencia_t oc;

                char buffer[32];

                printf("Digite o ID da ocorrência: ");
                ler_string(buffer, sizeof(buffer));
                oc.id_ocorrencia = atoi(buffer);

                printf("Data da ocorrência (AAAA-MM-DD): ");
                ler_string(oc.data_ocorrencia, sizeof(oc.data_ocorrencia));

                printf("Fonte dos dados: ");
                ler_string(oc.fonte_dados, sizeof(oc.fonte_dados));


                printf("Observador: ");
                ler_string(oc.observador, sizeof(oc.observador));


                printf("ID da planta associada: ");
                ler_string(buffer, sizeof(buffer));
                oc.id_planta = atoi(buffer);


                __int64 offset = salvar_ocorrencia(&oc);
                if (offset != -1) {
                    printf("Ocorrência salva com sucesso no offset %" PRId64 "\n", offset);
                    raiz_offset_ocorrencias = carregar_raiz("ocorrencias");
                }
                break;
            }

            case 6:
                listar_ocorrencias(raiz_offset_ocorrencias);
                break;

            case 7: {
                int id;
                printf("Digite o ID da ocorrência que deseja editar: ");
                scanf("%d", &id);
                getchar();
                editar_ocorrencia(raiz_offset_ocorrencias, id);
                break;
            }

            case 8: {
                int id;
                printf("Digite o ID da ocorrência que deseja apagar: ");
                scanf("%d", &id);
                getchar();
                apagar_ocorrencia(&raiz_offset_ocorrencias, id);
                break;
            }

            case 0:
                printf("Encerrando programa...\n");
                break;

            default:
                printf("Opção inválida. Tente novamente.\n");
                break;
        }

    } while (opcao != 0);

    fclose(fp_idx_plantas);
    fclose(fp_idx_ocorrencias);
    return 0;
}
