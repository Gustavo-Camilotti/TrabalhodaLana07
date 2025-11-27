#include "trie.h"
#include "plantas.h" // Para ler o arquivo de plantas

TrieNo *criar_no_trie() {
    TrieNo *no = (TrieNo *)malloc(sizeof(TrieNo));
    if (no) {
        no->offset = -1;
        no->fim_de_palavra = 0;
        for (int i = 0; i < TAM_ALFABETO; i++) {
            no->filhos[i] = NULL;
        }
    }
    return no;
}

// Insere ignorando maiúsculas/minúsculas
void inserir_trie(TrieNo *raiz, const char *palavra, long offset) {
    TrieNo *atual = raiz;
    for (int i = 0; palavra[i] != '\0'; i++) {
        // Converte para unsigned char para usar como índice
        unsigned char index = (unsigned char)tolower(palavra[i]);
        
        if (atual->filhos[index] == NULL) {
            atual->filhos[index] = criar_no_trie();
        }
        atual = atual->filhos[index];
    }
    atual->fim_de_palavra = 1;
    atual->offset = offset;
}

long buscar_trie(TrieNo *raiz, const char *palavra) {
    if (raiz == NULL) return -1;
    
    TrieNo *atual = raiz;
    for (int i = 0; palavra[i] != '\0'; i++) {
        unsigned char index = (unsigned char)tolower(palavra[i]);
        
        if (atual->filhos[index] == NULL) {
            return -1; // Não encontrado
        }
        atual = atual->filhos[index];
    }
    
    if (atual != NULL && atual->fim_de_palavra) {
        return atual->offset;
    }
    return -1;
}

TrieNo* carregar_trie_plantas() {
    TrieNo *raiz = criar_no_trie();
    FILE *fp = fopen("data/plantas.dat", "rb");
    
    if (fp == NULL) return raiz; // Retorna trie vazia se não houver arquivo

    planta_t p;
    long offset_atual;

    while (1) {
        offset_atual = FTELL(fp); // Pega a posição antes de ler
        if (fread(&p, sizeof(planta_t), 1, fp) != 1) break;

        if (p.id_planta != -1) {
            // Insere Nome Popular
            inserir_trie(raiz, p.nome_popular, offset_atual);
            // Insere Nome Científico também (opcional, permite buscar pelos dois)
            inserir_trie(raiz, p.nome_cientifico, offset_atual);
        }
    }
    
    fclose(fp);
    printf("[SISTEMA] Indice Trie carregado na memoria.\n");
    return raiz;
}

void liberar_trie(TrieNo *no) {
    if (no == NULL) return;
    for (int i = 0; i < TAM_ALFABETO; i++) {
        liberar_trie(no->filhos[i]);
    }
    free(no);
}