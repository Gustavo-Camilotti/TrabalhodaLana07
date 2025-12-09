#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "importacao.h"
#include "plantas.h"
#include "bioma.h"
#include "bplus.h"
#include "trie.h"

void importar_plantas_csv(const char *nome_arquivo, TrieNo *raiz_trie) {
    FILE *fp = fopen(nome_arquivo, "r");
    if (fp == NULL) {
        printf("[ERRO] Nao foi possivel abrir '%s'. Verifique se o arquivo esta na pasta.\n", nome_arquivo);
        return;
    }

    char linha[1024];
    int cadastrados = 0;
    int pulados = 0;

    // Carrega a raiz para verificar duplicatas
    __int64 raiz_plantas = carregar_raiz("plantas");

    printf("\n--- Iniciando Importacao de Plantas ---\n");

    while (fgets(linha, sizeof(linha), fp)) {
        // Remove o "enter" do final da linha
        linha[strcspn(linha, "\n")] = 0;

        // Pula linhas vazias
        if (strlen(linha) < 5) continue;

        planta_t p;
        
        // --- QUEBRANDO A LINHA NOS ';' ---
        
        // 1. ID
        char *token = strtok(linha, ";");
        if (!token) continue;
        p.id_planta = atoi(token);

        // Verifica se ja existe
        if (buscar_bplus("plantas", raiz_plantas, p.id_planta) != -1) {
            pulados++;
            continue;
        }

        // 2. Nome Cientifico
        token = strtok(NULL, ";");
        if (token) strncpy(p.nome_cientifico, token, sizeof(p.nome_cientifico)-1);
        else p.nome_cientifico[0] = '\0';

        // 3. Nome Popular
        token = strtok(NULL, ";");
        if (token) strncpy(p.nome_popular, token, sizeof(p.nome_popular)-1);
        else p.nome_popular[0] = '\0';

        // 4. Tipo
        token = strtok(NULL, ";");
        if (token) strncpy(p.tipo, token, sizeof(p.tipo)-1);
        else p.tipo[0] = '\0';

        // 5. Status
        token = strtok(NULL, ";");
        if (token) strncpy(p.status_conservacao, token, sizeof(p.status_conservacao)-1);
        else p.status_conservacao[0] = '\0';

        // 6. Descricao
        token = strtok(NULL, ";");
        if (token) strncpy(p.descricao, token, sizeof(p.descricao)-1);
        else p.descricao[0] = '\0';

        // 7. Data
        token = strtok(NULL, ";");
        if (token) strncpy(p.data_registro, token, sizeof(p.data_registro)-1);
        else p.data_registro[0] = '\0';

        // --- SALVANDO NO SISTEMA ---
        __int64 offset = salvar_planta(&p);

        if (offset != -1) {
            // Atualiza a Trie (Busca por nome)
            inserir_trie(raiz_trie, p.nome_popular, offset);
            inserir_trie(raiz_trie, p.nome_cientifico, offset);
            cadastrados++;
            
            // Recarrega raiz caso tenha mudado
            raiz_plantas = carregar_raiz("plantas");
        }
    }

    fclose(fp);
    printf("Sucesso: %d novas plantas importadas.\n", cadastrados);
    printf("Ignorados: %d (IDs ja existentes).\n", pulados);
}

void importar_biomas_csv(const char *nome_arquivo) {
    FILE *fp = fopen(nome_arquivo, "r");
    if (fp == NULL) {
        printf("[ERRO] Nao abriu '%s'.\n", nome_arquivo);
        return;
    }

    char linha[1024];
    int cadastrados = 0;

    printf("\n--- Importando Biomas ---\n");

    while (fgets(linha, sizeof(linha), fp)) {
        linha[strcspn(linha, "\n")] = 0;
        if (strlen(linha) < 2) continue;

        bioma_t b;
        char *token = strtok(linha, ";");
        if (!token) continue;
        b.id_bioma = atoi(token);

        token = strtok(NULL, ";");
        if (token) strncpy(b.nome_bioma, token, sizeof(b.nome_bioma)-1);
        
        token = strtok(NULL, ";");
        if (token) strncpy(b.tipo_vegetacao, token, sizeof(b.tipo_vegetacao)-1);

        token = strtok(NULL, ";");
        if (token) strncpy(b.clima, token, sizeof(b.clima)-1);

        token = strtok(NULL, ";");
        if (token) strncpy(b.descricao, token, sizeof(b.descricao)-1);

        if (salvar_bioma(&b) != -1) cadastrados++;
    }
    fclose(fp);
    printf("Biomas importados: %d\n", cadastrados);
}