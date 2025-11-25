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
                int id_temp;
                
                printf("\n--- Cadastro de Planta ---\n");
                printf("Digite o ID da planta: ");
                scanf("%d", &id_temp);
                getchar(); // consome \n

                // BLINDAGEM: Verifica se o ID já existe na árvore B+ de plantas
                if (buscar_bplus("plantas", raiz_offset_plantas, id_temp) != -1) {
                    printf("\n[ERRO] O ID %d ja existe cadastrado! Tente outro.\n", id_temp);
                    break; // Sai do case e volta para o menu
                }

                // Se chegou aqui, o ID é válido
                p.id_planta = id_temp;

                printf("Nome cientifico: ");
                ler_string(p.nome_cientifico, sizeof(p.nome_cientifico));
                printf("Nome popular: ");
                ler_string(p.nome_popular, sizeof(p.nome_popular));
                printf("Tipo: ");
                ler_string(p.tipo, sizeof(p.tipo));
                printf("Status de conservacao: ");
                ler_string(p.status_conservacao, sizeof(p.status_conservacao));
                printf("Descricao: ");
                ler_string(p.descricao, sizeof(p.descricao));
                printf("Data de registro (AAAA-MM-DD): ");
                ler_string(p.data_registro, sizeof(p.data_registro));

                __int64 offset = salvar_planta(&p);
                if (offset != -1) {
                    printf("Planta salva com sucesso no offset %" PRId64 "\n", offset);
                    // Recarrega a raiz pois a inserção pode ter alterado a árvore (split)
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
                int id_temp;

                printf("\n--- Cadastro de Ocorrencia ---\n");
                printf("Digite o ID da ocorrencia: ");
                ler_string(buffer, sizeof(buffer));
                id_temp = atoi(buffer);

                // BLINDAGEM: Verifica se o ID já existe na árvore B+ de ocorrências
                if (buscar_bplus("ocorrencias", raiz_offset_ocorrencias, id_temp) != -1) {
                    printf("\n[ERRO] O ID %d ja existe para outra ocorrencia! Tente outro.\n", id_temp);
                    break; // Sai do case e volta para o menu
                }

                oc.id_ocorrencia = id_temp;

                printf("Data da ocorrencia (AAAA-MM-DD): ");
                ler_string(oc.data_ocorrencia, sizeof(oc.data_ocorrencia));

                printf("Fonte dos dados: ");
                ler_string(oc.fonte_dados, sizeof(oc.fonte_dados));

                printf("Observador: ");
                ler_string(oc.observador, sizeof(oc.observador));

                printf("ID da planta associada: ");
                ler_string(buffer, sizeof(buffer));
                int id_planta_assoc = atoi(buffer);

                // VERIFICAÇÃO EXTRA (Opcional mas recomendada): A planta existe?
                if (buscar_bplus("plantas", raiz_offset_plantas, id_planta_assoc) == -1) {
                    printf("\n[AVISO] Nenhuma planta encontrada com ID %d. Cadastrando mesmo assim (sem vinculo valido).\n", id_planta_assoc);
                }
                oc.id_planta = id_planta_assoc;

                __int64 offset = salvar_ocorrencia(&oc);
                if (offset != -1) {
                    printf("Ocorrencia salva com sucesso no offset %" PRId64 "\n", offset);
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
