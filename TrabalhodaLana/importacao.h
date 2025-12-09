#ifndef IMPORTACAO_H
#define IMPORTACAO_H

#include "trie.h"

// Funcao para importar Plantas de um CSV
void importar_plantas_csv(const char *nome_arquivo, TrieNo *raiz_trie);

// Funcao para importar Biomas de um CSV
void importar_biomas_csv(const char *nome_arquivo);

#endif