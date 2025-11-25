ALTERACOES NA PRIMEIRA VERSAO (GUSTAVO CAMILOTTI)
# Trabalho Final de CPD - Indexação de Plantas e Ocorrências

## funcionalidades Implementadas
- **Persistência em Disco:** Dados salvos em arquivos `.dat` (binários) para acesso rápido.
- **Índice Árvore B+:** Implementação completa de inserção e busca indexada para Plantas e Ocorrências.
- **Compatibilidade:** Código ajustado para compilar corretamente em Windows (MinGW) e Linux (GCC).
- **CRUD:** Inserção, Listagem, Edição e Remoção de registros.

## Como Compilar e Rodar
### Windows (MinGW)
Abra o terminal na pasta do projeto e execute:
```bash
gcc main.c bplus.c plantas.c ocorrencias.c bioma.c distribuicao.c classificacao.c -o projeto_final.exe
.\projeto_final.exe
