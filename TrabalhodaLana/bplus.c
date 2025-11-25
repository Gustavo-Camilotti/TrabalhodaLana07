#include "bplus.h"

/**********************************************************************************************************************
    Funcoes utilitarias

**********************************************************************************************************************/

int carregar_no(const char* entidade, __int64 offset, bplus_no_t *no_out) {
    if (entidade == NULL || no_out == NULL) return 0;
    if (offset < 0) return 0;

    FILE *fp_idx = fp_bplus(entidade);
    if (fp_idx == NULL) return 0;

    // Verifica tamanho do arquivo
    if (FSEEK(fp_idx, 0, SEEK_END) != 0) { fclose(fp_idx); return 0; }
    __int64 tamanho = FTELL(fp_idx);
    if (tamanho < 0 || offset + (__int64)sizeof(bplus_no_t) > tamanho) { fclose(fp_idx); return 0; }

    if (FSEEK(fp_idx, offset, SEEK_SET) != 0) { fclose(fp_idx); return 0; }

    size_t lidos = fread(no_out, sizeof(bplus_no_t), 1, fp_idx);
    fclose(fp_idx);

    if (lidos != 1) return 0;

    return 1;
}



// Retorna 1 em sucesso, 0 em erro
int salvar_no(const char* entidade, __int64 offset, const bplus_no_t *no) {
    if (entidade == NULL || no == NULL) return 0;
    if (offset < 0) return 0;

    FILE *fp_idx = fp_bplus(entidade);
    if (fp_idx == NULL) return 0;

    // Verifica tamanho do arquivo
    if (FSEEK(fp_idx, 0, SEEK_END) != 0) { fclose(fp_idx); return 0; }
    __int64 tamanho = FTELL(fp_idx);
    if (tamanho < 0) { fclose(fp_idx); return 0; }

    // Permite escrever no fim ou sobrescrever dentro do arquivo
    if (offset > tamanho) { fclose(fp_idx); return 0; }

    if (FSEEK(fp_idx, offset, SEEK_SET) != 0) { fclose(fp_idx); return 0; }

    size_t escritos = fwrite(no, sizeof(bplus_no_t), 1, fp_idx);
    if (escritos != 1) { fclose(fp_idx); return 0; }

    if (fflush(fp_idx) != 0) { fclose(fp_idx); return 0; }

    fclose(fp_idx);
    return 1;
}

// Retorna offset do novo n em sucesso, -1 em erro.
__int64 criar_no_folha(const char* entidade) {
    if (entidade == NULL) return -1;

    FILE *fp_idx = fp_bplus(entidade);
    if (fp_idx == NULL) return -1;

    bplus_no_t novo;
    memset(&novo, 0, sizeof(bplus_no_t));
    novo.eh_folha = 1;
    novo.num_chaves = 0;
    novo.proximo = -1;

    if (FSEEK(fp_idx, 0, SEEK_END) != 0) { fclose(fp_idx); return -1; }
    __int64 offset = FTELL(fp_idx);
    if (offset < 0) { fclose(fp_idx); return -1; }

    if (fwrite(&novo, sizeof(bplus_no_t), 1, fp_idx) != 1) { fclose(fp_idx); return -1; }
    if (fflush(fp_idx) != 0) { fclose(fp_idx); return -1; }

    fclose(fp_idx);
    return offset;
}


// Retorna offset do novo n em sucesso, -1 em erro.
__int64 criar_no_interno(const char* entidade) {
    if (entidade == NULL) return -1;

    FILE *fp_idx = fp_bplus(entidade);
    if (fp_idx == NULL) return -1;

    bplus_no_t novo;
    memset(&novo, 0, sizeof(bplus_no_t));
    novo.eh_folha = 0;
    novo.num_chaves = 0;
    novo.proximo = -1;

    if (FSEEK(fp_idx, 0, SEEK_END) != 0) { fclose(fp_idx); return -1; }
    __int64 offset = FTELL(fp_idx);
    if (offset < 0) { fclose(fp_idx); return -1; }

    if (fwrite(&novo, sizeof(bplus_no_t), 1, fp_idx) != 1) { fclose(fp_idx); return -1; }
    if (fflush(fp_idx) != 0) { fclose(fp_idx); return -1; }

    fclose(fp_idx);
    return offset;
}
//---------------------------------------------------------------------------------------------------------------------

/**********************************************************************************************************************
    Funcoes de insercao

**********************************************************************************************************************/

// Retorna -1 se nao houve split (insercao local), ou o offset do novo no criado se houve split.
// chave_promovida e valido apenas quando ha split.
// entidade seleciona o indice correto ("plantas" ou "ocorrencias").
__int64 inserir_bplus_recursivo(
    const char* entidade,
    __int64 no_offset,
    int id,
    __int64 dado_offset,
    int *chave_promovida
) {
    if (entidade == NULL || no_offset < 0 || chave_promovida == NULL) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no;
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    // Caso 1: no folha
    if (no.eh_folha) {
        if (no.num_chaves < ORDEM) {
            // insercao simples ordenada no no folha
            int i = no.num_chaves - 1;
            while (i >= 0 && id < no.chaves[i]) {
                no.chaves[i+1]  = no.chaves[i];
                no.offsets[i+1] = no.offsets[i];
                i--;
            }
            no.chaves[i+1]  = id;
            no.offsets[i+1] = dado_offset;
            no.num_chaves++;

            if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
            if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
            if (fflush(fp) != 0) { fclose(fp); return -1; }
            fclose(fp);
            return -1; // sem split
        } else {
            // Split da folha
            bplus_no_t novo_folha;
            memset(&novo_folha, 0, sizeof(bplus_no_t));
            novo_folha.eh_folha = 1;
            novo_folha.proximo  = no.proximo;

            // Copia metade superior das chaves/offsets para a nova folha
            int meio = ORDEM / 2;
            for (int i = meio; i < ORDEM; i++) {
                novo_folha.chaves[novo_folha.num_chaves]  = no.chaves[i];
                novo_folha.offsets[novo_folha.num_chaves] = no.offsets[i];
                novo_folha.num_chaves++;
            }
            no.num_chaves = meio;

            // Inserir a nova chave no no correto (no original ou na nova folha)
            if (id < novo_folha.chaves[0]) {
                int i = no.num_chaves - 1;
                while (i >= 0 && id < no.chaves[i]) {
                    no.chaves[i+1]  = no.chaves[i];
                    no.offsets[i+1] = no.offsets[i];
                    i--;
                }
                no.chaves[i+1]  = id;
                no.offsets[i+1] = dado_offset;
                no.num_chaves++;
            } else {
                int i = novo_folha.num_chaves - 1;
                while (i >= 0 && id < novo_folha.chaves[i]) {
                    novo_folha.chaves[i+1]  = novo_folha.chaves[i];
                    novo_folha.offsets[i+1] = novo_folha.offsets[i];
                    i--;
                }
                novo_folha.chaves[i+1]  = id;
                novo_folha.offsets[i+1] = dado_offset;
                novo_folha.num_chaves++;
            }

            // Encadeia folhas
            if (FSEEK(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
            __int64 novo_offset = FTELL(fp);
            if (novo_offset < 0) { fclose(fp); return -1; }
            novo_folha.proximo = no.proximo;
            no.proximo         = novo_offset;

            // Salva no original e nova folha
            if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
            if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
            if (FSEEK(fp, novo_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
            if (fwrite(&novo_folha, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
            if (fflush(fp) != 0) { fclose(fp); return -1; }

            // Em B+ tree, a chave promovida para o pai o a primeira chave da nova folha
            *chave_promovida = novo_folha.chaves[0];
            fclose(fp);
            return novo_offset;
        }
    }

    // Caso 2: No interno
    int i = 0;
    while (i < no.num_chaves && id >= no.chaves[i]) {
        i++;
    }

    int chave_promovida_filho = -1;
    __int64 novo_filho_offset = inserir_bplus_recursivo(entidade, no.filhos[i], id, dado_offset, &chave_promovida_filho);

    if (novo_filho_offset != -1) {
        // O filho foi split e promoveu uma chave; inserir no no interno atual
        if (no.num_chaves < ORDEM) {
            // insercao simples no no interno (sem split)
            for (int j = no.num_chaves; j > i; j--) {
                no.chaves[j] = no.chaves[j-1];
                no.filhos[j+1] = no.filhos[j];
            }
            no.chaves[i]   = chave_promovida_filho;
            no.filhos[i+1] = novo_filho_offset;
            no.num_chaves++;

            if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
            if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
            if (fflush(fp) != 0) { fclose(fp); return -1; }
            fclose(fp);
            return -1; // sem split no no interno
        } else {
            // Split de no interno
            bplus_no_t novo_interno;
            memset(&novo_interno, 0, sizeof(bplus_no_t));
            novo_interno.eh_folha = 0;

            // Primeiro, inserimos a chave promovida do filho no no atual (em memAria)
            // para depois fazer o split com o arranjo ja atualizado.
            // Desloca para abrir espaco
            for (int j = no.num_chaves; j > i; j--) {
                no.chaves[j]   = no.chaves[j-1];
                no.filhos[j+1] = no.filhos[j];
            }
            no.chaves[i]   = chave_promovida_filho;
            no.filhos[i+1] = novo_filho_offset;
            no.num_chaves++; // agora no.num_chaves == ORDEM + 1 (cheio com overflow)

            int meio = no.num_chaves / 2; // mediana apos inserir
            // A chave mediana sobe para o pai
            *chave_promovida = no.chaves[meio];

            // Copiar metade direita para o novo no interno
            // Chaves: [meio+1 .. no.num_chaves-1]
            // Filhos: [meio+1 .. no.num_chaves] (lembrando filhos sao um a mais)
            for (int j = meio + 1; j < no.num_chaves; j++) {
                novo_interno.chaves[novo_interno.num_chaves] = no.chaves[j];
                novo_interno.filhos[novo_interno.num_chaves] = no.filhos[j];
                novo_interno.num_chaves++;
            }
            // ultimo filho da direita
            novo_interno.filhos[novo_interno.num_chaves] = no.filhos[no.num_chaves];

            // Ajustar o no original para ficar com a metade esquerda
            // apos a subida da mediana, o no original fica com [0 .. meio-1] chaves
            // Filhos [0 .. meio]
            // noo e necessario "limpar" os buffers; basta ajustar num_chaves
            no.num_chaves = meio;

            // Gravar novo no no fim do arquivo
            if (FSEEK(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
            __int64 novo_offset = FTELL(fp);
            if (novo_offset < 0) { fclose(fp); return -1; }
            if (FSEEK(fp, novo_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
            if (fwrite(&novo_interno, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

            // Atualizar no original
            if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
            if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
            if (fflush(fp) != 0) { fclose(fp); return -1; }

            fclose(fp);
            // Retorna o offset do novo no para o novel acima
            return novo_offset;
        }
    }

    fclose(fp);
    return -1; // insercao no filho noo causou split
}

// Cria raiz folha se raiz_offset == -1
// Evita duplicatas: noo insere se id ja existir
// Atualiza raiz_offset e persiste apos split
int inserir_bplus(const char* entidade, __int64* raiz_offset, int id, __int64 dado_offset) {
    if (entidade == NULL || raiz_offset == NULL) return 0;

    // Checa duplicata
    __int64 ja_existe = buscar_bplus(entidade, *raiz_offset, id);
    if (ja_existe >= 0) {
        // ja existe, noo insere
        return 1;
    }

    // Se arvore vazia, cria folha raiz
    if (*raiz_offset == -1) {
        __int64 folha = criar_no_folha(entidade);
        if (folha < 0) return 0;

        // insercao trivial na folha recAm-criada
        int dummy_prom;
        __int64 split = inserir_bplus_recursivo(entidade, folha, id, dado_offset, &dummy_prom);
        if (split == -1) {
            *raiz_offset = folha;
            salvar_raiz(*raiz_offset, entidade);
            return 1;
        } else {
            // Se a prApria folha raiz splitou, cria nova raiz interna
            bplus_no_t nova;
            memset(&nova, 0, sizeof(bplus_no_t));
            nova.eh_folha = 0;
            nova.num_chaves = 1;
            nova.chaves[0] = dummy_prom;
            nova.filhos[0] = folha;
            nova.filhos[1] = split;

            FILE* fp = fp_bplus(entidade);
            if (fp == NULL) return 0;
            // Nova raiz vai para o fim do arquivo
            if (FSEEK(fp, 0, SEEK_END) != 0) { fclose(fp); return 0; }
            __int64 nova_offset = FTELL(fp);
            if (nova_offset < 0) { fclose(fp); return 0; }
            if (FSEEK(fp, nova_offset, SEEK_SET) != 0) { fclose(fp); return 0; }
            if (fwrite(&nova, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return 0; }
            if (fflush(fp) != 0) { fclose(fp); return 0; }
            fclose(fp);

            *raiz_offset = nova_offset;
            salvar_raiz(*raiz_offset, entidade);
            return 1;
        }
    }

    // insercao normal a partir da raiz existente
    int chave_promovida = -1;
    __int64 novo_offset = inserir_bplus_recursivo(entidade, *raiz_offset, id, dado_offset, &chave_promovida);

    // Se houve split na raiz atual, criar nova raiz interna
    if (novo_offset != -1) {
        bplus_no_t nova_raiz;
        memset(&nova_raiz, 0, sizeof(bplus_no_t));
        nova_raiz.eh_folha = 0;
        nova_raiz.num_chaves = 1;
        nova_raiz.chaves[0] = chave_promovida;
        nova_raiz.filhos[0] = *raiz_offset; // raiz antiga
        nova_raiz.filhos[1] = novo_offset;  // novo no

        FILE* fp = fp_bplus(entidade);
        if (fp == NULL) return 0;

        // Grava nova raiz no fim do arquivo indice
        if (FSEEK(fp, 0, SEEK_END) != 0) { fclose(fp); return 0; }
        __int64 nova_offset = FTELL(fp);
        if (nova_offset < 0) { fclose(fp); return 0; }
        if (FSEEK(fp, nova_offset, SEEK_SET) != 0) { fclose(fp); return 0; }
        if (fwrite(&nova_raiz, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return 0; }
        if (fflush(fp) != 0) { fclose(fp); return 0; }
        fclose(fp);

        *raiz_offset = nova_offset;
        salvar_raiz(*raiz_offset, entidade);
    }

    return 1;
}

__int64 salvar_na_arvore_bplus(const char* entidade, int id, __int64 offset, __int64 raiz_offset) {
    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    // Caso inicial: arvore vazia
    if (raiz_offset == -1) {
        bplus_no_t raiz;
        memset(&raiz, 0, sizeof(bplus_no_t));
        raiz.eh_folha = 1;
        raiz.num_chaves = 1;
        raiz.chaves[0] = id;
        raiz.offsets[0] = offset;
        raiz.proximo = -1;

        if (FSEEK(fp, 0, SEEK_END) != 0) return -1;
        raiz_offset = FTELL(fp);
        if (raiz_offset < 0) return -1;

        if (fwrite(&raiz, sizeof(bplus_no_t), 1, fp) != 1) return -1;
        if (fflush(fp) != 0) return -1;
        return raiz_offset;
    }

    // insercao recursiva
    int chave_promovida;
    __int64 novo_filho_offset = inserir_bplus_recursivo(entidade, raiz_offset, id, offset, &chave_promovida);;

    // Se houve split na raiz, criar nova raiz
    if (novo_filho_offset != -1) {
        bplus_no_t nova_raiz;
        memset(&nova_raiz, 0, sizeof(bplus_no_t));
        nova_raiz.eh_folha = 0;
        nova_raiz.num_chaves = 1;
        nova_raiz.chaves[0] = chave_promovida;
        nova_raiz.filhos[0] = raiz_offset;
        nova_raiz.filhos[1] = novo_filho_offset;

        if (FSEEK(fp, 0, SEEK_END) != 0) return -1;
        raiz_offset = FTELL(fp);
        if (raiz_offset < 0) return -1;

        if (fwrite(&nova_raiz, sizeof(bplus_no_t), 1, fp) != 1) return -1;
        if (fflush(fp) != 0) return -1;
    }

    return raiz_offset;
}

//---------------------------------------------------------------------------------------------------------------------

/**********************************************************************************************************************
    Funoces de remocao

**********************************************************************************************************************/

// Funde duas folhas vizinhas e retorna o offset do no resultante.
// chave_removida_pai recebe a chave que deve ser removida do no pai.
__int64 fundir_folhas(const char* entidade, __int64 no_offset, __int64 irmao_offset, int *chave_removida_pai) {
    if (entidade == NULL || chave_removida_pai == NULL) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no, irmao;
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (FSEEK(fp, irmao_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&irmao, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (irmao.chaves[0] < no.chaves[0]) {
        // irmao a esquerda: funde 'no' dentro de 'irmao'
        for (int i = 0; i < no.num_chaves; i++) {
            if (irmao.num_chaves >= ORDEM) break; // protecao contra overflow
            irmao.chaves[irmao.num_chaves]  = no.chaves[i];
            irmao.offsets[irmao.num_chaves] = no.offsets[i];
            irmao.num_chaves++;
        }
        irmao.proximo = no.proximo;

        if (FSEEK(fp, irmao_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
        if (fwrite(&irmao, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
        if (fflush(fp) != 0) { fclose(fp); return -1; }

        *chave_removida_pai = no.chaves[0]; // chave separadora que apontava para 'no'
        fclose(fp);
        return irmao_offset;
    } else {
        // irmao a direita: funde 'irmao' dentro de 'no'
        for (int i = 0; i < irmao.num_chaves; i++) {
            if (no.num_chaves >= ORDEM) break; // protecao contra overflow
            no.chaves[no.num_chaves]  = irmao.chaves[i];
            no.offsets[no.num_chaves] = irmao.offsets[i];
            no.num_chaves++;
        }
        no.proximo = irmao.proximo;

        if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
        if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
        if (fflush(fp) != 0) { fclose(fp); return -1; }

        *chave_removida_pai = irmao.chaves[0]; // chave separadora que apontava para 'irmao'
        fclose(fp);
        return no_offset;
    }
}

// Funde dois nos internos e retorna o offset do no resultante.
// chave_removida_pai recebe a chave separadora que deve ser removida do pai.
__int64 fundir_internos(const char* entidade, __int64 no_offset, __int64 irmao_offset, int chave_pai, int *chave_removida_pai) {
    if (entidade == NULL || chave_removida_pai == NULL) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no, irmao;
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (FSEEK(fp, irmao_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&irmao, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (irmao.chaves[0] < no.chaves[0]) {
        // irmao a esquerda: funde 'no' dentro de 'irmao'
        if (irmao.num_chaves < ORDEM) {
            irmao.chaves[irmao.num_chaves] = chave_pai;
            irmao.num_chaves++;
        }

        for (int i = 0; i < no.num_chaves; i++) {
            if (irmao.num_chaves >= ORDEM) break; // protecao contra overflow
            irmao.chaves[irmao.num_chaves] = no.chaves[i];
            irmao.filhos[irmao.num_chaves] = no.filhos[i];
            irmao.num_chaves++;
        }
        irmao.filhos[irmao.num_chaves] = no.filhos[no.num_chaves];

        if (FSEEK(fp, irmao_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
        if (fwrite(&irmao, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
        if (fflush(fp) != 0) { fclose(fp); return -1; }

        *chave_removida_pai = chave_pai;
        fclose(fp);
        return irmao_offset;
    } else {
        // irmao a direita: funde 'irmao' dentro de 'no'
        if (no.num_chaves < ORDEM) {
            no.chaves[no.num_chaves] = chave_pai;
            no.num_chaves++;
        }

        for (int i = 0; i < irmao.num_chaves; i++) {
            if (no.num_chaves >= ORDEM) break; // protecao contra overflow
            no.chaves[no.num_chaves] = irmao.chaves[i];
            no.filhos[no.num_chaves] = irmao.filhos[i];
            no.num_chaves++;
        }
        no.filhos[no.num_chaves] = irmao.filhos[irmao.num_chaves];

        if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
        if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
        if (fflush(fp) != 0) { fclose(fp); return -1; }

        *chave_removida_pai = chave_pai;
        fclose(fp);
        return no_offset;
    }
}

// Redistribui chaves entre folhas vizinhas.
// atualizar_pai recebe a nova chave separadora para o no pai.
void redistribuir_folha(const char* entidade, __int64 no_offset, __int64 irmao_offset, int *atualizar_pai) {
    if (entidade == NULL || atualizar_pai == NULL) return;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return;

    bplus_no_t no, irmao;
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }

    if (FSEEK(fp, irmao_offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&irmao, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }

    int minimo = (ORDEM + 1) / 2;

    if (irmao.num_chaves > minimo) {
        if (irmao.chaves[0] < no.chaves[0]) {
            // redistribuicao do irmao a esquerda
            if (no.num_chaves >= ORDEM) { fclose(fp); return; } // protecao
            for (int i = no.num_chaves; i > 0; i--) {
                no.chaves[i]  = no.chaves[i-1];
                no.offsets[i] = no.offsets[i-1];
            }
            no.chaves[0]  = irmao.chaves[irmao.num_chaves - 1];
            no.offsets[0] = irmao.offsets[irmao.num_chaves - 1];
            no.num_chaves++;
            irmao.num_chaves--;

            *atualizar_pai = no.chaves[0]; // nova separadora
        } else {
            // redistribuicao do irmao a direita
            if (no.num_chaves >= ORDEM) { fclose(fp); return; } // protecao
            no.chaves[no.num_chaves]  = irmao.chaves[0];
            no.offsets[no.num_chaves] = irmao.offsets[0];
            no.num_chaves++;

            for (int i = 0; i < irmao.num_chaves - 1; i++) {
                irmao.chaves[i]  = irmao.chaves[i+1];
                irmao.offsets[i] = irmao.offsets[i+1];
            }
            irmao.num_chaves--;

            *atualizar_pai = irmao.chaves[0]; // nova separadora
        }

        // Grava os dois nos atualizados
        if (FSEEK(fp, no_offset, SEEK_SET) == 0) fwrite(&no, sizeof(bplus_no_t), 1, fp);
        if (FSEEK(fp, irmao_offset, SEEK_SET) == 0) fwrite(&irmao, sizeof(bplus_no_t), 1, fp);
        fflush(fp);
    }

    fclose(fp);
}

// Remove uma chave de uma folha. Retorna -1 se remocao ok ou redistribuicao,
// ou o offset do no sobrevivente se houve fusao.
__int64 remover_bplus_folha(const char* entidade, __int64 no_offset, int id,
                            __int64 irmao_offset, int chave_pai,
                            int *chave_promovida) {
    if (entidade == NULL || chave_promovida == NULL) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no, irmao;

    // Carregar no alvo
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    // Procurar chave
    int pos = -1;
    for (int i = 0; i < no.num_chaves; i++) {
        if (no.chaves[i] == id) { pos = i; break; }
    }
    if (pos == -1) { fclose(fp); return -1; } // noo encontrada

    // Remover deslocando
    for (int i = pos; i < no.num_chaves - 1; i++) {
        no.chaves[i]  = no.chaves[i+1];
        no.offsets[i] = no.offsets[i+1];
    }
    no.num_chaves--;

    // Salvar no atualizado
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
    if (fflush(fp) != 0) { fclose(fp); return -1; }

    // Verificar underflow
    int minimo = (ORDEM + 1) / 2;
    if (no.num_chaves < minimo) {
        // Carregar irmao
        if (FSEEK(fp, irmao_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
        if (fread(&irmao, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

        if (irmao.num_chaves > minimo) {
            // redistribuicao
            int nova_chave_pai;
            redistribuir_folha(entidade, no_offset, irmao_offset, &nova_chave_pai);
            *chave_promovida = nova_chave_pai;
            fclose(fp);
            return -1; // redistribuicao resolveu
        } else {
            // Fusao
            __int64 sobrevivente = fundir_folhas(entidade, no_offset, irmao_offset, chave_promovida);
            fclose(fp);
            return sobrevivente; // sinaliza fusao para o pai
        }
    }

    fclose(fp);
    return -1; // remocao ok, sem underflow
}



__int64 remover_bplus_recursivo(const char* entidade, __int64 no_offset, int id, int *chave_promovida) {
    if (entidade == NULL || no_offset < 0) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no;
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    int minimo = (ORDEM + 1) / 2;

    // Caso 1: no folha
    if (no.eh_folha) {
        fclose(fp);
        return remover_bplus_folha(entidade, no_offset, id, -1, -1, chave_promovida);
    }

    // Caso 2: no interno
    int i = 0;
    while (i < no.num_chaves && id >= no.chaves[i]) i++;

    int chave_promovida_filho;
    __int64 resultado = remover_bplus_recursivo(entidade, no.filhos[i], id, &chave_promovida_filho);

    if (resultado != -1) { // filho sofreu underflow
        __int64 irmao_offset;
        int chave_removida_pai;

        if (i > 0) { // irmao A esquerda
            irmao_offset = no.filhos[i-1];

            bplus_no_t filho;
            FSEEK(fp, resultado, SEEK_SET);
            fread(&filho, sizeof(bplus_no_t), 1, fp);

            if (filho.eh_folha) {
                __int64 sobrevivente = remover_bplus_folha(entidade, resultado, id, irmao_offset, no.chaves[i-1], chave_promovida);
                if (sobrevivente != -1) {
                    for (int j = i-1; j < no.num_chaves - 1; j++) {
                        no.chaves[j] = no.chaves[j+1];
                        no.filhos[j+1] = no.filhos[j+2];
                    }
                    no.num_chaves--;
                }
            } else {
                bplus_no_t irmao;
                FSEEK(fp, irmao_offset, SEEK_SET);
                fread(&irmao, sizeof(bplus_no_t), 1, fp);

                if (irmao.num_chaves > minimo) {
                    no.chaves[i-1] = chave_promovida_filho;
                } else {
                    fundir_internos(entidade, resultado, irmao_offset, no.chaves[i-1], &chave_removida_pai);
                    for (int j = i-1; j < no.num_chaves - 1; j++) {
                        no.chaves[j] = no.chaves[j+1];
                        no.filhos[j+1] = no.filhos[j+2];
                    }
                    no.num_chaves--;
                }
            }
        } else if (i < no.num_chaves) { // irmao A direita
            irmao_offset = no.filhos[i+1];

            bplus_no_t filho;
            FSEEK(fp, resultado, SEEK_SET);
            fread(&filho, sizeof(bplus_no_t), 1, fp);

            if (filho.eh_folha) {
                __int64 sobrevivente = remover_bplus_folha(entidade, resultado, id, irmao_offset, no.chaves[i], chave_promovida);
                if (sobrevivente != -1) {
                    for (int j = i; j < no.num_chaves - 1; j++) {
                        no.chaves[j] = no.chaves[j+1];
                        no.filhos[j+1] = no.filhos[j+2];
                    }
                    no.num_chaves--;
                }
            } else {
                bplus_no_t irmao;
                FSEEK(fp, irmao_offset, SEEK_SET);
                fread(&irmao, sizeof(bplus_no_t), 1, fp);

                if (irmao.num_chaves > minimo) {
                    no.chaves[i] = chave_promovida_filho;
                } else {
                    fundir_internos(entidade, resultado, irmao_offset, no.chaves[i], &chave_removida_pai);
                    for (int j = i; j < no.num_chaves - 1; j++) {
                        no.chaves[j] = no.chaves[j+1];
                        no.filhos[j+1] = no.filhos[j+2];
                    }
                    no.num_chaves--;
                }
            }
        }

        // Salvar pai atualizado
        if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
        if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
        if (fflush(fp) != 0) { fclose(fp); return -1; }

        if (no.num_chaves < minimo) {
            fclose(fp);
            return no_offset; // sinaliza underflow para novel acima
        }
    }

    fclose(fp);
    return -1;
}

__int64 remover_bplus(const char* entidade, __int64 raiz_offset, int id) {
    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    int chave_promovida;
    remover_bplus_recursivo(entidade, raiz_offset, id, &chave_promovida);;

    // Carrega a raiz para verificar se houve alteracao
    bplus_no_t raiz;
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) return -1;
    if (fread(&raiz, sizeof(bplus_no_t), 1, fp) != 1) return -1;

    // Caso especial: raiz sem chaves
    if (raiz.num_chaves == 0) {
        if (raiz.eh_folha) {
            // arvore ficou vazia
            return -1;
        } else {
            // Nova raiz passa a ser o unico filho
            return raiz.filhos[0];
        }
    }

    return raiz_offset;
}

//---------------------------------------------------------------------------------------------------------------------

/**********************************************************************************************************************
    Funoces de Impressao e Navegacao

**********************************************************************************************************************/

void imprimir_bplus(const char* entidade, __int64 raiz_offset, int nivel) {
    if (raiz_offset == -1) {
        printf("arvore B+ vazia.\n");
        return;
    }

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) {
        perror("Erro ao abrir indice");
        return;
    }

    bplus_no_t no;
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }

    // indentacao por novel
    for (int i = 0; i < nivel; i++) printf("  ");

    // imprime o no atual
    printf("no em offset %" PRId64 " | ", raiz_offset);
    printf("%s | ", no.eh_folha ? "Folha" : "Interno");
    printf("Chaves: ");
    for (int i = 0; i < no.num_chaves; i++) {
        printf("%d ", no.chaves[i]);
    }
    printf("\n");

    fclose(fp);

    // se noo for folha, imprime recursivamente os filhos
    if (!no.eh_folha) {
        for (int i = 0; i <= no.num_chaves; i++) {
            imprimir_bplus(entidade, no.filhos[i], nivel + 1);
        }
    }
}


int contar_registros_bplus(const char* entidade, __int64 raiz_offset) {
    if (raiz_offset == -1) {
        return 0; // arvore vazia
    }

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no;
    // Posiciona na raiz
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    // Navega ata a folha mais a esquerda
    while (!no.eh_folha) {
        if (FSEEK(fp, no.filhos[0], SEEK_SET) != 0) { fclose(fp); return -1; }
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }
    }

    int contador = 0;

    // Percorre folhas encadeadas
    while (1) {
        contador += no.num_chaves;

        if (no.proximo == -1) break;

        if (FSEEK(fp, no.proximo, SEEK_SET) != 0) break;
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) break;
    }

    fclose(fp);
    return contador;
}


void listar_bplus_pagina_total(const char* entidade, __int64 raiz_offset, int tamanho_pagina, int pagina_desejada) {
    if (raiz_offset == -1) {
        printf("arvore B+ vazia.\n");
        return;
    }

    int total_registros = contar_registros_bplus(entidade, raiz_offset);
    if (total_registros <= 0) {
        printf("Nenhum registro encontrado.\n");
        return;
    }

    int total_paginas = (total_registros + tamanho_pagina - 1) / tamanho_pagina;

    if (pagina_desejada < 1 || pagina_desejada > total_paginas) {
        printf("pagina invAlida. Existem %d paginas disponoveis.\n", total_paginas);
        return;
    }

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) {
        perror("Erro ao abrir indice");
        return;
    }

    bplus_no_t no;
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }

    // Navega ate a folha mais a esquerda
    while (!no.eh_folha) {
        if (FSEEK(fp, no.filhos[0], SEEK_SET) != 0) { fclose(fp); return; }
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }
    }

    int contador = 0;
    int inicio_pagina = (pagina_desejada - 1) * tamanho_pagina + 1;
    int fim_pagina = pagina_desejada * tamanho_pagina;

    // Percorre folhas encadeadas
    while (1) {
        for (int i = 0; i < no.num_chaves; i++) {
            contador++;
            if (contador >= inicio_pagina && contador <= fim_pagina) {
                printf("ID: %d -> Offset: %" PRId64 "\n", no.chaves[i], no.offsets[i]);
            }
            if (contador > fim_pagina) break; // ja exibiu a pagina completa
        }

        if (contador > fim_pagina || no.proximo == -1) break;

        if (FSEEK(fp, no.proximo, SEEK_SET) != 0) break;
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) break;
    }

    fclose(fp);

    printf("---- pagina %d de %d exibida ----\n", pagina_desejada, total_paginas);
}



//---------------------------------------------------------------------------------------------------------------------

/**********************************************************************************************************************
    Funoces de Persistencia

**********************************************************************************************************************/

FILE* fp_bplus(const char* entidade) {
    char path[256];
    snprintf(path, sizeof(path), "data/bplus_%s.dat", entidade);

    FILE *fp = fopen(path, "r+b");
    if (fp == NULL) {
        fp = fopen(path, "w+b"); // cria se noo existir
        if (fp == NULL) {
            perror("Erro ao abrir/criar indice B+");
            return NULL;
        }
    }
    return fp;
}

void fechar_bplus(const char* entidade) {
    FILE *fp = fp_bplus(entidade);
    if (fp != NULL) {
        fclose(fp);
    }
}

void salvar_raiz(__int64 raiz_offset, const char* entidade) {
    char path[256];
    snprintf(path, sizeof(path), "data/bplus_%s_meta.dat", entidade);

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("Erro ao abrir meta do indice B+");
        return;
    }

    if (fwrite(&raiz_offset, sizeof(__int64), 1, fp) != 1) {
        perror("Erro ao salvar raiz");
    }
    fclose(fp);
}

__int64 carregar_raiz(const char* entidade) {
    char path[256];
    snprintf(path, sizeof(path), "data/bplus_%s_meta.dat", entidade);

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return -1;

    __int64 raiz_offset;
    if (fread(&raiz_offset, sizeof(__int64), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return raiz_offset;
}


//-------------------------------------------------------------------------------------------------------------------------------

__int64 buscar_bplus(const char* entidade, __int64 raiz_offset, int id) {
    if (entidade == NULL || raiz_offset == -1) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no;
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    // Caso 1: no folha
    if (no.eh_folha) {
        for (int i = 0; i < no.num_chaves; i++) {
            if (no.chaves[i] == id) {
                __int64 resultado = no.offsets[i];
                fclose(fp);
                return resultado; // encontrado
            }
        }
        fclose(fp);
        return -1; // noo encontrado
    }

    // Caso 2: no interno
    int i = 0;
    while (i < no.num_chaves && id >= no.chaves[i]) {
        i++;
    }

    fclose(fp);
    return buscar_bplus(entidade, no.filhos[i], id);
}


#include <inttypes.h> // para PRId64

void listar_bplus_paginado(const char* entidade, __int64 raiz_offset, int tamanho_pagina) {
    if (raiz_offset == -1) {
        printf("arvore B+ vazia.\n");
        return;
    }

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) {
        perror("Erro ao abrir indice");
        return;
    }

    bplus_no_t no;
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }

    // Navega atA a folha mais A esquerda
    while (!no.eh_folha) {
        if (FSEEK(fp, no.filhos[0], SEEK_SET) != 0) { fclose(fp); return; }
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }
    }

    int contador = 0;
    int pagina = 1;

    // Percorre folhas encadeadas
    while (1) {
        for (int i = 0; i < no.num_chaves; i++) {
            printf("ID: %d -> Offset: %" PRId64 "\n", no.chaves[i], no.offsets[i]);
            contador++;

            if (contador % tamanho_pagina == 0) {
                printf("---- pagina %d concluida ----\n", pagina);
                pagina++;
                printf("Pressione ENTER para continuar...\n");
                getchar();
            }
        }

        if (no.proximo == -1) break;

        if (FSEEK(fp, no.proximo, SEEK_SET) != 0) break;
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) break;
    }

    fclose(fp);

    // Se terminar sem completar a Altima pagina
    if (contador % tamanho_pagina != 0) {
        printf("---- pagina %d concluida ----\n", pagina);
    }
}

/*
void listar_bplus_pagina(FILE *fp, __int64 raiz_offset, int tamanho_pagina, int pagina_desejada) {
    bplus_no_t no;
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) return;
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) return;

    // Navega atA a folha mais A esquerda
    while (!no.eh_folha) {
        if (FSEEK(fp, no.filhos[0], SEEK_SET) != 0) return;
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) return;
    }

    int contador = 0;
    int inicio_pagina = (pagina_desejada - 1) * tamanho_pagina + 1;
    int fim_pagina    = pagina_desejada * tamanho_pagina;

    // Percorre folhas encadeadas
    while (1) {
        for (int i = 0; i < no.num_chaves; i++) {
            contador++;
            if (contador >= inicio_pagina && contador <= fim_pagina) {
                printf("ID: %d -> Offset: %" PRId64 "\n", no.chaves[i], no.offsets[i]);
            }
            if (contador > fim_pagina) break; // ja exibiu a pagina completa
        }

        if (contador > fim_pagina || no.proximo == -1) break;

        if (FSEEK(fp, no.proximo, SEEK_SET) != 0) break;
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) break;
    }

    printf("---- pagina %d exibida ----\n", pagina_desejada);
}
*/

void listar_bplus_pagina(const char* entidade, __int64 raiz_offset, int tamanho_pagina, int pagina_desejada) {
    if (raiz_offset == -1) {
        printf("arvore B+ vazia.\n");
        return;
    }

    int total_registros = contar_registros_bplus(entidade, raiz_offset);
    if (total_registros <= 0) {
        printf("Nenhum registro encontrado.\n");
        return;
    }

    int total_paginas = (total_registros + tamanho_pagina - 1) / tamanho_pagina;

    if (pagina_desejada < 1 || pagina_desejada > total_paginas) {
        printf("pagina invalida. Existem %d paginas disponoveis.\n", total_paginas);
        return;
    }

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) {
        perror("Erro ao abrir indice");
        return;
    }

    bplus_no_t no;
    if (FSEEK(fp, raiz_offset, SEEK_SET) != 0) { fclose(fp); return; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }

    // Navega atA a folha mais A esquerda
    while (!no.eh_folha) {
        if (FSEEK(fp, no.filhos[0], SEEK_SET) != 0) { fclose(fp); return; }
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return; }
    }

    int contador = 0;
    int inicio_pagina = (pagina_desejada - 1) * tamanho_pagina + 1;
    int fim_pagina    = pagina_desejada * tamanho_pagina;

    // Percorre folhas encadeadas
    while (1) {
        for (int i = 0; i < no.num_chaves; i++) {
            contador++;
            if (contador >= inicio_pagina && contador <= fim_pagina) {
                printf("ID: %d -> Offset: %" PRId64 "\n", no.chaves[i], no.offsets[i]);
            }
            if (contador > fim_pagina) break; // ja exibiu a pagina completa
        }

        if (contador > fim_pagina || no.proximo == -1) break;

        if (FSEEK(fp, no.proximo, SEEK_SET) != 0) break;
        if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) break;
    }

    fclose(fp);

    printf("---- pagina %d de %d exibida ----\n", pagina_desejada, total_paginas);
}



__int64 dividir_folha(const char* entidade, __int64 folha_offset, int *chave_promovida) {
    if (entidade == NULL || chave_promovida == NULL || folha_offset < 0) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t folha;
    if (FSEEK(fp, folha_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&folha, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (folha.num_chaves < 2) { fclose(fp); return -1; } // noo hA como dividir

    int meio = folha.num_chaves / 2;

    bplus_no_t nova_folha;
    memset(&nova_folha, 0, sizeof(bplus_no_t));
    nova_folha.eh_folha = 1;
    nova_folha.num_chaves = folha.num_chaves - meio;

    for (int i = 0; i < nova_folha.num_chaves; i++) {
        nova_folha.chaves[i]  = folha.chaves[meio + i];
        nova_folha.offsets[i] = folha.offsets[meio + i];
    }

    folha.num_chaves = meio;

    // encadeamento
    nova_folha.proximo = folha.proximo;

    // posiAAo da nova folha no fim do arquivo
    if (FSEEK(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
    __int64 nova_offset = FTELL(fp);
    if (nova_offset < 0) { fclose(fp); return -1; }

    folha.proximo = nova_offset;

    // grava nova folha
    if (FSEEK(fp, nova_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fwrite(&nova_folha, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    // atualiza folha original
    if (FSEEK(fp, folha_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fwrite(&folha, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (fflush(fp) != 0) { fclose(fp); return -1; }

    fclose(fp);

    *chave_promovida = nova_folha.chaves[0];
    return nova_offset;
}


__int64 dividir_interno(const char* entidade, __int64 no_offset, int *chave_promovida) {
    if (entidade == NULL || chave_promovida == NULL || no_offset < 0) return -1;

    FILE *fp = fp_bplus(entidade);
    if (fp == NULL) return -1;

    bplus_no_t no;
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fread(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (no.num_chaves < 2) { fclose(fp); return -1; } // noo hA como dividir

    int meio = no.num_chaves / 2;

    // chave do meio serA promovida
    *chave_promovida = no.chaves[meio];

    bplus_no_t novo_no;
    memset(&novo_no, 0, sizeof(bplus_no_t));
    novo_no.eh_folha = 0;
    novo_no.num_chaves = no.num_chaves - meio - 1;

    // copiar chaves da metade direita
    for (int i = 0; i < novo_no.num_chaves; i++) {
        novo_no.chaves[i] = no.chaves[meio + 1 + i];
    }
    // copiar filhos da metade direita
    for (int i = 0; i <= novo_no.num_chaves; i++) {
        novo_no.filhos[i] = no.filhos[meio + 1 + i];
    }

    // ajustar no original (metade esquerda)
    no.num_chaves = meio;

    // salvar novo no no fim do arquivo
    if (FSEEK(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
    __int64 novo_offset = FTELL(fp);
    if (novo_offset < 0) { fclose(fp); return -1; }

    if (FSEEK(fp, novo_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fwrite(&novo_no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    // regravar no original
    if (FSEEK(fp, no_offset, SEEK_SET) != 0) { fclose(fp); return -1; }
    if (fwrite(&no, sizeof(bplus_no_t), 1, fp) != 1) { fclose(fp); return -1; }

    if (fflush(fp) != 0) { fclose(fp); return -1; }

    fclose(fp);
    return novo_offset;
}

// Edita um campo de string: mostra valor atual, pede novo valor e atualiza se noo for vazio
void editar_campo_string(const char *rotulo, char *campo, int tamanho) {
    char buffer[200];

    printf("%s atual: %s\n", rotulo, campo);
    printf("Digite novo %s (ou pressione Enter para manter): ", rotulo);

    if (fgets(buffer, sizeof(buffer), stdin) && buffer[0] != '\n') {
        buffer[strcspn(buffer, "\n")] = '\0'; // remove \n
        strncpy(campo, buffer, tamanho - 1);
        campo[tamanho - 1] = '\0'; // garante terminaAAo
    }
}
