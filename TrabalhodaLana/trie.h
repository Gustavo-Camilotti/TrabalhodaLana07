#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bplus.h" // Para as macros de arquivo

// Tamanho do alfabeto (ASCII estendido para garantir espaços e símbolos)
#define TAM_ALFABETO 256

// Nó da Trie
typedef struct TrieNo {
    struct TrieNo *filhos[TAM_ALFABETO];
    long offset; // Offset no arquivo .dat (-1 se não for fim de palavra)
    int fim_de_palavra;
} TrieNo;

// Inicializa um novo nó
TrieNo *criar_no_trie();

// Insere uma palavra e seu offset na Trie
void inserir_trie(TrieNo *raiz, const char *palavra, long offset);

// Busca uma palavra exata e retorna o offset (ou -1 se não achar)
long buscar_trie(TrieNo *raiz, const char *palavra);

// Carrega todos os nomes do arquivo plantas.dat para a Trie na memória
TrieNo* carregar_trie_plantas();

// Libera memória
void liberar_trie(TrieNo *no);

#endif