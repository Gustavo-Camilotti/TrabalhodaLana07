#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <locale.h>

// Módulos do projeto
#include "bplus.h"
#include "plantas.h"
#include "ocorrencias.h"
#include "bioma.h"
#include "classificacao.h"
#include "distribuicao.h"
#include "inv_bioma.h"
#include "trie.h"
#include "importacao.h"

// Função auxiliar para ler string com segurança
void ler_string(char *dest, int tamanho) {


    
    if (dest == NULL || tamanho <= 0) return;
    if (fgets(dest, tamanho, stdin) != NULL) {
        dest[strcspn(dest, "\n")] = '\0';
    } else {
        dest[0] = '\0';
    }
}

int main() {
    setlocale(LC_ALL, "Portuguese");

    // Inicializa as raízes dos índices B+ (Carrega da memória se existir)
    __int64 raiz_offset_plantas = carregar_raiz("plantas");
    __int64 raiz_offset_ocorrencias = carregar_raiz("ocorrencias");
   //carrega a trie na memoria
    TrieNo *raiz_nomes = carregar_trie_plantas();   
    // CORREÇÃO DE SEGURANÇA: Abre e fecha apenas para garantir que os arquivos existem
    // Isso evita travar o arquivo no Windows durante a execução
    FILE *fp_check = fp_bplus("plantas"); if (fp_check) fclose(fp_check);
    fp_check = fp_bplus("ocorrencias"); if (fp_check) fclose(fp_check);

    int opcao;
    do {
        printf("\n===== MENU PRINCIPAL =====\n");
        printf("--- PLANTAS ---\n");
        printf("1. Adicionar planta\n");
        printf("2. Listar plantas\n");
        printf("3. Editar planta\n");
        printf("4. Apagar planta\n");
        printf("--- OCORRENCIAS ---\n");
        printf("5. Adicionar ocorrencia\n");
        printf("6. Listar ocorrencias\n");
        printf("7. Editar ocorrencia\n");
        printf("8. Apagar ocorrencia\n");
        printf("--- OUTROS CADASTROS ---\n");
        printf("11. Adicionar Bioma\n");
        printf("12. Listar Biomas\n");
        printf("13. Apagar Bioma\n");
        printf("14. Adicionar Classificacao\n");
        printf("15. Listar Classificacoes\n");
        printf("16. Apagar Classificacao\n");
        printf("17. Adicionar Distribuicao (Geografica)\n");
        printf("18. Listar Distribuicoes\n");
        printf("19. Apagar Distribuicao\n");
        printf("--- BUSCAS ESPECIAIS ---\n");
        printf("9. Vincular Planta a Bioma (Manual)\n");
        printf("10. Buscar Plantas por Bioma (Indice Invertido)\n");
        printf("20. Buscar Planta por Nome (Trie)\n");
        printf("21. Importar Plantas de CSV (Lote)\n");
        printf("22. Importar Biomas de CSV (Lote)\n");
        printf("0. Sair\n");
        printf("Escolha uma opcao: ");
        
        // Tratamento para entrada inválida (letras no lugar de números)
        if (scanf("%d", &opcao) != 1) {
            while(getchar() != '\n'); 
            opcao = -1;
        }
        getchar(); // consome \n

        switch (opcao) {
            case 1: {
                planta_t p;
                int id_temp;
                printf("\n--- Cadastro de Planta ---\n");
                printf("Digite o ID da planta: ");
                scanf("%d", &id_temp);
                getchar();

                // Verifica duplicidade
                if (buscar_bplus("plantas", raiz_offset_plantas, id_temp) != -1) {
                    printf("\n[ERRO] O ID %d ja existe cadastrado! Tente outro.\n", id_temp);
                    break;
                }
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
                    raiz_offset_plantas = carregar_raiz("plantas");
                    // ATUALIZA A TRIE NA MEMÓRIA
                    inserir_trie(raiz_nomes, p.nome_popular, offset);
                    inserir_trie(raiz_nomes, p.nome_cientifico, offset);
               
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

                if (buscar_bplus("ocorrencias", raiz_offset_ocorrencias, id_temp) != -1) {
                    printf("\n[ERRO] O ID %d ja existe para outra ocorrencia! Tente outro.\n", id_temp);
                    break;
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
                oc.id_planta = atoi(buffer);

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
                printf("Digite o ID da ocorrencia que deseja editar: ");
                scanf("%d", &id);
                getchar();
                editar_ocorrencia(raiz_offset_ocorrencias, id);
                break;
            }
            case 8: {
                int id;
                printf("Digite o ID da ocorrencia que deseja apagar: ");
                scanf("%d", &id);
                getchar();
                apagar_ocorrencia(&raiz_offset_ocorrencias, id);
                break;
            }
            case 9: {
                int id_planta, id_bioma;
                printf("\n--- Vincular Planta a Bioma (Manual) ---\n");
                printf("Digite o ID da Planta existente: ");
                scanf("%d", &id_planta);
                if (buscar_bplus("plantas", raiz_offset_plantas, id_planta) == -1) {
                    printf("[ERRO] Planta nao encontrada! Cadastre a planta primeiro.\n");
                    break;
                }
                printf("Digite o ID do Bioma: ");
                scanf("%d", &id_bioma);
                getchar();
                if (adicionar_indice_bioma(id_bioma, id_planta)) {
                    printf("Vinculo criado com sucesso! (Bioma %d -> Planta %d)\n", id_bioma, id_planta);
                } else {
                    printf("Erro ao salvar vinculo.\n");
                }
                break;
            }
            case 10: {
                int id_bioma;
                printf("\n--- Buscar Plantas por Bioma ---\n");
                printf("Digite o ID do Bioma para pesquisar: ");
                scanf("%d", &id_bioma);
                getchar();
                buscar_plantas_por_bioma(id_bioma);
                break;
            }
            case 11: {
                bioma_t b;
                printf("\n--- Cadastro de Bioma ---\n");
                printf("ID do Bioma: ");
                scanf("%d", &b.id_bioma); getchar();
                printf("Nome do Bioma: "); ler_string(b.nome_bioma, sizeof(b.nome_bioma));
                printf("Tipo de Vegetacao: "); ler_string(b.tipo_vegetacao, sizeof(b.tipo_vegetacao));
                printf("Clima: "); ler_string(b.clima, sizeof(b.clima));
                printf("Descricao: "); ler_string(b.descricao, sizeof(b.descricao));
                salvar_bioma(&b);
                break;
            }
            case 12: listar_biomas(); break;
            case 13: {
                int id; printf("ID do Bioma para apagar: "); scanf("%d", &id); getchar();
                apagar_bioma(id); break;
            }
            case 14: {
                classificacao_t c;
                printf("\n--- Cadastro de Classificacao ---\n");
                printf("ID da Classificacao: "); scanf("%d", &c.id_classificacao); getchar();
                printf("Reino: "); ler_string(c.reino, sizeof(c.reino));
                printf("Filo: "); ler_string(c.filo, sizeof(c.filo));
                printf("Classe: "); ler_string(c.classe, sizeof(c.classe));
                printf("Ordem: "); ler_string(c.ordem, sizeof(c.ordem));
                printf("Familia: "); ler_string(c.familia, sizeof(c.familia));
                printf("Genero: "); ler_string(c.genero, sizeof(c.genero));
                printf("ID da Planta Associada: "); scanf("%d", &c.id_planta); getchar();
                salvar_classificacao(&c);
                break;
            }
            case 15: listar_classificacoes(); break;
            case 16: {
                int id; printf("ID da Classificacao para apagar: "); scanf("%d", &id); getchar();
                apagar_classificacao(id); break;
            }
            case 17: {
                distribuicao_t d;
                printf("\n--- Cadastro de Distribuicao Geografica ---\n");
                printf("ID da Distribuicao: "); scanf("%d", &d.id_distribuicao); getchar();
                printf("Continente: "); ler_string(d.continente, sizeof(d.continente));
                printf("Pais: "); ler_string(d.pais, sizeof(d.pais));
                printf("ID do Bioma: "); scanf("%d", &d.id_bioma);
                printf("ID da Planta: "); scanf("%d", &d.id_planta); getchar();
                salvar_distribuicao(&d);
                // Automacao do Índice Invertido
                adicionar_indice_bioma(d.id_bioma, d.id_planta);
                printf("[SISTEMA] Indice invertido atualizado (Bioma %d -> Planta %d).\n", d.id_bioma, d.id_planta);
                break;
            }
            case 18: listar_distribuicoes(); break;
            case 19: {
                int id; printf("ID da Distribuicao para apagar: "); scanf("%d", &id); getchar();
                apagar_distribuicao(id); break;
            }
            case 20: {
                char termo[100];
                printf("\n--- Busca por Nome (Popular ou Cientifico) ---\n");
                printf("Digite o nome: ");
                ler_string(termo, sizeof(termo));

                long offset = buscar_trie(raiz_nomes, termo);
                
                if (offset != -1) {
                    FILE *fp = fopen("data/plantas.dat", "rb");
                    if (fp) {
                        planta_t p;
                        fseek(fp, offset, SEEK_SET);
                        fread(&p, sizeof(planta_t), 1, fp);
                        
                        printf("\n--- Planta Encontrada ---\n");
                        printf("ID: %d\n", p.id_planta);
                        printf("Nome Popular: %s\n", p.nome_popular);
                        printf("Nome Cientifico: %s\n", p.nome_cientifico);
                        printf("Descricao: %s\n", p.descricao);
                        fclose(fp);
                    }
                } else {
                    printf("Nenhuma planta encontrada com o nome '%s'.\n", termo);
                }
                break;
            }
        case 21: {
                char nome_arq[100];
                printf("\n--- Importacao de Plantas (CSV) ---\n");
                printf("Digite o nome do arquivo (ex: tabelaplanta.csv): ");
                scanf("%s", nome_arq);
                getchar(); // limpar buffer

                importar_plantas_csv(nome_arq, raiz_nomes);
                
                // Atualiza a raiz da B+ no main
                raiz_offset_plantas = carregar_raiz("plantas");
                break;
            }
        case 22: {
                char nome_arq[100];
                printf("\n--- Importacao de Biomas (CSV) ---\n");
                printf("Digite o nome do arquivo (ex: biomas.csv): ");
                scanf("%s", nome_arq);
                getchar();

                importar_biomas_csv(nome_arq);
                break;
            }
        case 0: printf("Encerrando programa...\n"); break;
            default: printf("Opcao invalida. Tente novamente.\n"); break;
        }
    if (opcao != 0) {
            printf("\n\n>>> Pressione ENTER para voltar ao menu... <<<");
            
            // O primeiro getchar pode consumir um \n sobrando, o segundo espera voce apertar
            // Se o programa passar direto, adicione mais um getchar();
            getchar(); 
            
            // Comando para limpar a tela
            // No Windows usa "cls", no Linux usa "clear"
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
        }



    } while (opcao != 0);

    return 0;
}