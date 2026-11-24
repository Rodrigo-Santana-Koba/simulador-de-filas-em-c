#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <errno.h>
#endif

typedef struct NoSolicitacao
{
    int prioridade;
    char identificacao[50];
    struct NoSolicitacao *prox;
} Tipo_Solicitacao;

typedef struct Fila
{
    Tipo_Solicitacao *inicio;
    Tipo_Solicitacao *fim;
} Tipo_Fila;

/* Inicializa uma fila vazia. */
void inicializa_fila(Tipo_Fila *fila)
{
    fila->inicio = NULL;
    fila->fim = NULL;
}


/* Retorna 1 se a fila estiver vazia e 0 se possuir pedidos. */
int fila_vazia(Tipo_Fila *fila)
{
    if(fila->inicio == NULL){
        return 1;
    }

    return 0;
}


/* Aloca e preenche um novo pedido. */
Tipo_Solicitacao *cria_solicitacao(int prioridade,
                                  char identificacao[])
{
    Tipo_Solicitacao *novo;

    if(prioridade < 1 || prioridade > 3){
        return NULL;
    }

    if(strlen(identificacao) == 0 ||
       strlen(identificacao) >= 50){
        return NULL;
    }

    novo = malloc(sizeof(Tipo_Solicitacao));

    if(novo == NULL){
        return NULL;
    }

    novo->prioridade = prioridade;
    strcpy(novo->identificacao, identificacao);
    novo->prox = NULL;

    return novo;
}


/* Coloca um no ja existente no fim da fila. */
void insere_no_fila(Tipo_Fila *fila, Tipo_Solicitacao *novo)
{
    novo->prox = NULL;

    if(fila_vazia(fila)){
        fila->inicio = novo;
    }
    else{
        fila->fim->prox = novo;
    }

    fila->fim = novo;
}


/* Cria um pedido e o coloca no fim da fila. */
int enfileira_solicitacao(Tipo_Fila *fila, int prioridade,
                         char identificacao[])
{
    Tipo_Solicitacao *novo;

    novo = cria_solicitacao(prioridade, identificacao);

    if(novo == NULL){
        return 0;
    }

    insere_no_fila(fila, novo);

    return 1;
}

/*
Retira o primeiro no da fila e retorna seu endereco.
Nao libera a memoria, pois esse no pode ir para outra fila.
*/
Tipo_Solicitacao *retira_no_fila(Tipo_Fila *fila)
{
    Tipo_Solicitacao *retirado;

    if(fila_vazia(fila)){
        return NULL;
    }

    retirado = fila->inicio;
    fila->inicio = retirado->prox;

    if(fila->inicio == NULL){
        fila->fim = NULL;
    }

    retirado->prox = NULL;

    return retirado;
}


/* Remove e libera todos os nos de uma fila. */
void libera_fila(Tipo_Fila *fila)
{
    Tipo_Solicitacao *retirado;

    while(!fila_vazia(fila)){
        retirado = retira_no_fila(fila);
        free(retirado);
    }
}


/*
Le os pedidos do arquivo e os coloca na fila de entrada.
Retorna a quantidade carregada ou -1 em caso de erro.
*/
int carrega_fila(Tipo_Fila *entrada, char nomeArquivo[])
{
    FILE *arquivo;
    int prioridade;
    char identificacao[50];
    char separador;
    int leitura;
    int quantidade = 0;

    arquivo = fopen(nomeArquivo, "r");

    if(arquivo == NULL){
        printf("Nao foi possivel abrir o arquivo %s.\n",
               nomeArquivo);
        return -1;
    }

    for(;;){
        leitura = fscanf(arquivo, " %d , %49[^; \t\r\n] %c",
                         &prioridade, identificacao, &separador);

        if(leitura == EOF){
            break;
        }

        if(leitura != 3 || separador != ';'){
            printf("Formato invalido no pedido %d.\n",
                   quantidade + 1);

            fclose(arquivo);
            return -1;
        }

        if(prioridade < 1 || prioridade > 3){
            printf("Prioridade invalida no pedido %d.\n",
                   quantidade + 1);

            fclose(arquivo);
            return -1;
        }

        if(!enfileira_solicitacao(entrada, prioridade,
                                 identificacao)){
            printf("Nao foi possivel alocar o pedido.\n");

            fclose(arquivo);
            return -1;
        }

        quantidade++;
    }

    if(ferror(arquivo)){
        printf("Erro na leitura do arquivo.\n");

        fclose(arquivo);
        return -1;
    }

    fclose(arquivo);

    return quantidade;
}


/* Mostra os pedidos do inicio ate o fim de uma fila. */
void mostra_fila(Tipo_Fila *fila, char nome[])
{
    Tipo_Solicitacao *atual = fila->inicio;
    int quantidade = 0;

    printf("\n%s:\n", nome);

    if(fila_vazia(fila)){
        printf("Fila vazia.\n");
        return;
    }

    while(atual != NULL){
        printf("[Prioridade: %d | Identificacao: %s]\n",
               atual->prioridade, atual->identificacao);

        quantidade++;

        atual = atual->prox;
    }

    printf("Total: %d pedido(s).\n", quantidade);
}


/* Mostra a situacao das quatro filas. */
void mostra_filas(Tipo_Fila *entrada, Tipo_Fila *fila0,
                  Tipo_Fila *fila1, Tipo_Fila *fila2)
{
    mostra_fila(entrada, "Fila de entrada");
    mostra_fila(fila0, "Fila 0 - prioridade maxima");
    mostra_fila(fila1, "Fila 1 - prioridade normal");
    mostra_fila(fila2, "Fila 2 - prioridade baixa");

    fflush(stdout);
}


/* Aguarda meio segundo. Retorna 0 se ocorrer um erro. */
int espera_meio_segundo(void)
{
#ifdef _WIN32

    Sleep(500);

#else

    struct timespec tempo;
    struct timespec restante;

    tempo.tv_sec = 0;
    tempo.tv_nsec = 500000000L;

    while(nanosleep(&tempo, &restante) == -1){
        if(errno != EINTR){
            return 0;
        }

        tempo = restante;
    }

#endif

    return 1;
}


/*
Transfere ate cinco pedidos para suas filas de prioridade.
Retorna quantos pedidos foram transferidos.
*/
int distribui_pedidos(Tipo_Fila *entrada, Tipo_Fila *fila0,
                     Tipo_Fila *fila1, Tipo_Fila *fila2)
{
    Tipo_Solicitacao *pedido;
    int quantidade = 0;

    while(quantidade < 5 && !fila_vazia(entrada)){
        pedido = retira_no_fila(entrada);

        if(pedido->prioridade == 1){
            insere_no_fila(fila0, pedido);
        }
        else if(pedido->prioridade == 2){
            insere_no_fila(fila1, pedido);
        }
        else{
            insere_no_fila(fila2, pedido);
        }

        quantidade++;
    }

    return quantidade;
}

/* Organiza toda a execucao da parte 2. */
void aplicacao_fila(void)
{
    Tipo_Fila entrada;
    Tipo_Fila fila0;
    Tipo_Fila fila1;
    Tipo_Fila fila2;

    int quantidade;
    int transferidos;
    int rodada = 1;

    inicializa_fila(&entrada);
    inicializa_fila(&fila0);
    inicializa_fila(&fila1);
    inicializa_fila(&fila2);

    quantidade = carrega_fila(&entrada, "solicitacoes.txt");

    if(quantidade >= 0 && quantidade < 50){
        printf("O arquivo precisa ter pelo menos 50 pedidos.\n");
        printf("Foram encontrados apenas %d.\n", quantidade);
    }
    else if(quantidade >= 50){
        printf("\nForam carregados %d pedidos.\n", quantidade);

        printf("\nSituacao inicial:\n");
        mostra_filas(&entrada, &fila0, &fila1, &fila2);

        while(!fila_vazia(&entrada)){
            if(!espera_meio_segundo()){
                printf("Erro ao aguardar a proxima distribuicao.\n");
                break;
            }

            transferidos = distribui_pedidos(
                &entrada, &fila0, &fila1, &fila2
            );

            printf("\nRodada %d: %d pedido(s) transferido(s).\n",
                   rodada, transferidos);

            mostra_filas(&entrada, &fila0, &fila1, &fila2);

            rodada++;
        }

        if(fila_vazia(&entrada)){
            printf("\nDistribuicao concluida.\n");
        }
    }

    libera_fila(&entrada);
    libera_fila(&fila0);
    libera_fila(&fila1);
    libera_fila(&fila2);
}

int main(void)
{
    aplicacao_fila();

    return 0;
}
