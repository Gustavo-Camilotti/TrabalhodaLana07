Sistema de Gerenciamento Botânico - CPD 2025/2

**Trabalho Final da disciplina de Classificação e Pesquisa de Dados**

Este projeto consiste em um sistema completo para catalogação de espécies de plantas e ocorrências botânicas, com foco em eficiência de armazenamento e busca utilizando estruturas de dados avançadas em memória secundária (disco).

---

## 👨‍💻 Equipe
* Erick
* Gustavo
* Icaro

**Professora:** Lana Rossato  
**Turma:** B-2025/2

---

## Funcionalidades Implementadas

O sistema atende a todos os requisitos propostos, incluindo CRUD completo e múltiplos métodos de indexação:

 1. Gerenciamento de Entidades (CRUD)
* **Plantas:** Cadastro completo (Nome Científico, Popular, Conservação, etc.).
* **Ocorrências:** Registro de avistamentos vinculados às plantas.
* **Biomas, Classificação e Distribuição Geográfica:** Entidades complementares totalmente funcionais.
* **Integridade:** Sistema de proteção que impede o cadastro de IDs duplicados.

 2. Estruturas de Dados e Indexação
* **Árvore B+ (Em Disco):** Utilizada como índice primário para **Plantas** e **Ocorrências**. Permite inserção, remoção e busca eficiente por ID diretamente no arquivo binário, minimizando acessos a disco.
* **Arquivo Invertido (Em Disco):** Implementado para relacionar **Biomas -> Plantas**. Permite listar todas as espécies de um bioma sem percorrer o arquivo de dados inteiro.
* **Árvore Trie (Em Memória):** Utilizada para busca rápida por **Nome** (Popular ou Científico). A estrutura é carregada a partir do arquivo binário na inicialização para garantir performance instantânea.

 3. Persistência de Dados
* Todos os dados são salvos em arquivos binários (`.dat`) na pasta `data/`.
* Implementação de serialização de nós da Árvore B+ (convertendo ponteiros de memória em *offsets* de arquivo).

---

  Tecnologias e Compilação

O projeto foi desenvolvido em C e configurado para ser multiplataforma (compatível com Windows/MinGW e Linux/GCC).

Pré-requisitos
* Compilador GCC (MinGW no Windows ou build-essential no Linux).
* Git (para versionamento).

### Como Compilar e Rodar

Abra o terminal na pasta raiz do projeto (`TrabalhodaLana`) e execute:

```bash
# Comando de Compilação
cd trabalhodalana
gcc main.c bplus.c plantas.c ocorrencias.c bioma.c distribuicao.c classificacao.c inv_bioma.c trie.c -o projeto_final.exe

# Executar
.\projeto_final.exe
