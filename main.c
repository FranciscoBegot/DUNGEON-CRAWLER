#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#define VILA_TAM 10
#define FASE1_TAM 10
#define FASE2_TAM 20
#define FASE3_TAM 40
#define MAX_REINICIOS 3
#define MAX_INIMIGOS 5

typedef struct {
    int x, y;
} Posicao;

typedef struct {
    Posicao pos;
    int ativo;
    int tipo; // 0 = monstro normal (X), 1 = monstro inteligente (V)
} Inimigo;

Posicao jogador;
Inimigo inimigos[MAX_INIMIGOS];
int numInimigos = 0;
int reinicios = 0;
int chaveColetada = 0;

// -------------------- Função getch -------------------- //

char getch() {
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0) perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr ~ICANON");
    return buf;
}

// -------------------- Funções utilitárias -------------------- //

void limparTela() {
    system("clear");
}

char **alocarMapa(int linhas, int colunas) {
    char **mapa = malloc(linhas * sizeof(char *));
    for (int i = 0; i < linhas; i++) {
        mapa[i] = malloc(colunas * sizeof(char));
    }
    return mapa;
}

void liberarMapa(char **mapa, int linhas) {
    for (int i = 0; i < linhas; i++) {
        free(mapa[i]);
    }
    free(mapa);
}

void mostrarMapa(char **mapa, int linhas, int colunas) {
    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            printf("%c", mapa[i][j]);
        }
        printf("\n");
    }
}

// -------------------- Funções de Inimigos -------------------- //

void inicializarInimigos() {
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        inimigos[i].ativo = 0;
        inimigos[i].tipo = 0;
    }
    numInimigos = 0;
}

void adicionarInimigo(int x, int y, int tipo) {
    if (numInimigos < MAX_INIMIGOS) {
        inimigos[numInimigos].pos.x = x;
        inimigos[numInimigos].pos.y = y;
        inimigos[numInimigos].ativo = 1;
        inimigos[numInimigos].tipo = tipo;
        numInimigos++;
    }
}

int posicaoOcupada(char **mapa, int x, int y, int altura, int largura) {
    // Verifica se a posição está fora dos limites
    if (x < 0 || y < 0 || x >= altura || y >= largura) return 1;
    
    // Verifica se há parede ou objeto fixo
    char cell = mapa[x][y];
    if (cell == '*' || cell == '@' || cell == 'P' || cell == 'O' || cell == 'D' || cell == '=') return 1;
    
    // Verifica se há outro inimigo na posição
    for (int i = 0; i < numInimigos; i++) {
        if (inimigos[i].ativo && inimigos[i].pos.x == x && inimigos[i].pos.y == y) return 1;
    }
    
    return 0;
}

void moverInimigos(char **mapa, int altura, int largura) {
    // Primeiro, remove todos os inimigos do mapa
    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            if (mapa[i][j] == 'X' || mapa[i][j] == 'V') {
                mapa[i][j] = ' ';
            }
        }
    }
    
    // Move cada inimigo
    for (int i = 0; i < numInimigos; i++) {
        if (!inimigos[i].ativo) continue;
        
        int tentativas = 0;
        int moveu = 0;
        
        if (inimigos[i].tipo == 0) {
            // Monstro normal (X) - movimento aleatório
            while (!moveu && tentativas < 10) {
                int direcao = rand() % 4;
                int nx = inimigos[i].pos.x;
                int ny = inimigos[i].pos.y;
                
                switch (direcao) {
                    case 0: nx--; break; // Cima
                    case 1: nx++; break; // Baixo
                    case 2: ny--; break; // Esquerda
                    case 3: ny++; break; // Direita
                }
                
                // Verifica se a nova posição é válida
                if (!posicaoOcupada(mapa, nx, ny, altura, largura)) {
                    inimigos[i].pos.x = nx;
                    inimigos[i].pos.y = ny;
                    moveu = 1;
                }
                tentativas++;
            }
        } else if (inimigos[i].tipo == 1) {
            // Monstro inteligente (V) - segue o jogador mas mais devagar
            // Move apenas 50% das vezes para ser mais fácil
            if (rand() % 2 == 0) {
                int dx = 0, dy = 0;
                
                // Calcula a direção para o jogador
                if (jogador.x < inimigos[i].pos.x) dx = -1;
                else if (jogador.x > inimigos[i].pos.x) dx = 1;
                
                if (jogador.y < inimigos[i].pos.y) dy = -1;
                else if (jogador.y > inimigos[i].pos.y) dy = 1;
                
                // 70% chance de seguir o jogador, 30% movimento aleatório
                if (rand() % 100 < 70) {
                    // Tenta mover na direção do jogador
                    int nx = inimigos[i].pos.x + dx;
                    int ny = inimigos[i].pos.y;
                    
                    if (dx != 0 && !posicaoOcupada(mapa, nx, ny, altura, largura)) {
                        inimigos[i].pos.x = nx;
                        moveu = 1;
                    } else {
                        // Se não conseguiu mover em X, tenta em Y
                        nx = inimigos[i].pos.x;
                        ny = inimigos[i].pos.y + dy;
                        
                        if (dy != 0 && !posicaoOcupada(mapa, nx, ny, altura, largura)) {
                            inimigos[i].pos.y = ny;
                            moveu = 1;
                        }
                    }
                }
                
                // Se não moveu ou movimento aleatório
                if (!moveu) {
                    while (!moveu && tentativas < 3) {
                        int direcao = rand() % 4;
                        int nx = inimigos[i].pos.x;
                        int ny = inimigos[i].pos.y;
                        
                        switch (direcao) {
                            case 0: nx--; break;
                            case 1: nx++; break;
                            case 2: ny--; break;
                            case 3: ny++; break;
                        }
                        
                        if (!posicaoOcupada(mapa, nx, ny, altura, largura)) {
                            inimigos[i].pos.x = nx;
                            inimigos[i].pos.y = ny;
                            moveu = 1;
                        }
                        tentativas++;
                    }
                }
            }
        }
    }
    
    // Recoloca os inimigos no mapa
    for (int i = 0; i < numInimigos; i++) {
        if (inimigos[i].ativo) {
            char simbolo = (inimigos[i].tipo == 0) ? 'X' : 'V';
            mapa[inimigos[i].pos.x][inimigos[i].pos.y] = simbolo;
        }
    }
}

int verificarColisaoInimigo() {
    for (int i = 0; i < numInimigos; i++) {
        if (inimigos[i].ativo && 
            inimigos[i].pos.x == jogador.x && 
            inimigos[i].pos.y == jogador.y) {
            return 1;
        }
    }
    return 0;
}

// -------------------- Movimentação -------------------- //

int moverJogador(char **mapa, int altura, int largura, char comando) {
    int dx = 0, dy = 0;

    switch (comando) {
        case 'w': case 'W': dx = -1; break;
        case 's': case 'S': dx = 1; break;
        case 'a': case 'A': dy = -1; break;
        case 'd': case 'D': dy = 1; break;
        default: return 0;
    }

    int nx = jogador.x + dx;
    int ny = jogador.y + dy;

    if (nx < 0 || ny < 0 || nx >= altura || ny >= largura) return 0;

    char destino = mapa[nx][ny];

    if (destino == '*') return 0;

    if (destino == 'D' && !chaveColetada) {
        printf("A porta está fechada! Pegue a chave primeiro.\n");
        sleep(1);
        return 0;
    }

    if (destino == '#' || destino == 'X' || destino == 'V') {
        reinicios++;
        printf("Você foi pego! Reiniciando a fase (%d/%d)...\n", reinicios, MAX_REINICIOS);
        sleep(2);
        return -1;
    }

    if (destino == '=' && chaveColetada) {
        return 1;
    }

    if (destino == '@') {
        chaveColetada = 1;
        printf("Você pegou a chave!\n");
        sleep(1);
        for (int i = 0; i < altura; i++) {
            for (int j = 0; j < largura; j++) {
                if (mapa[i][j] == 'D') mapa[i][j] = '=';
            }
        }
    }

    mapa[jogador.x][jogador.y] = ' ';
    jogador.x = nx;
    jogador.y = ny;
    mapa[jogador.x][jogador.y] = '&';

    return 0;
}

// -------------------- Interação -------------------- //

void interagir(char **mapa, int altura, int largura) {
    int dirs[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};

    for (int i = 0; i < 4; i++) {
        int nx = jogador.x + dirs[i][0];
        int ny = jogador.y + dirs[i][1];

        if (nx < 0 || ny < 0 || nx >= altura || ny >= largura) continue;

        char obj = mapa[nx][ny];

        if (obj == '@') {
            chaveColetada = 1;
            mapa[nx][ny] = ' ';
            printf("Você pegou a chave!\n");
            sleep(1);
            for (int i2 = 0; i2 < altura; i2++) {
                for (int j2 = 0; j2 < largura; j2++) {
                    if (mapa[i2][j2] == 'D') mapa[i2][j2] = '=';
                }
            }
            return;
        } else if (obj == 'P') {
            printf("NPC: Use WASD para mover, 'i' para interagir! Cuidado com os monstros X e V!\n");
            sleep(2);
            return;
        } else if (obj == 'O') {
            printf("Você ativou um botão misterioso! Nada aconteceu ainda...\n");
            sleep(2);
            return;
        }
    }
}

// -------------------- Declaração de funções -------------------- //

char **criarFase(int tam);
char **reiniciarFase(int altura, int largura);

// -------------------- Criar Vila -------------------- //

char **criarVila() {
    char **mapa = alocarMapa(VILA_TAM, VILA_TAM);
    inicializarInimigos();

    for (int i = 0; i < VILA_TAM; i++) {
        for (int j = 0; j < VILA_TAM; j++) {
            mapa[i][j] = ' ';
        }
    }

    for (int i = 0; i < VILA_TAM; i++) {
        mapa[0][i] = mapa[VILA_TAM-1][i] = '*';
        mapa[i][0] = mapa[i][VILA_TAM-1] = '*';
    }

    mapa[1][1] = '&'; jogador.x = 1; jogador.y = 1;
    mapa[2][2] = 'P';
    mapa[4][4] = '@';
    mapa[6][6] = 'D';

    return mapa;
}

// -------------------- Reiniciar Fase -------------------- //

char **reiniciarFase(int altura, int largura) {
    // Reset das variáveis globais
    chaveColetada = 0;
    
    // Cria novo mapa baseado no tamanho
    if (altura == VILA_TAM) {
        return criarVila();
    } else if (altura == FASE1_TAM) {
        return criarFase(FASE1_TAM);
    } else if (altura == FASE2_TAM) {
        return criarFase(FASE2_TAM);
    } else if (altura == FASE3_TAM) {
        return criarFase(FASE3_TAM);
    }
    
    // Fallback - cria vila se não identificar o tamanho
    return criarVila();
}

// -------------------- Jogar Fase -------------------- //

int jogarFase(char **mapa, int altura, int largura) {
    char comando;
    reinicios = 0;
    chaveColetada = 0;
    int morreu = 0;

    while (reinicios < MAX_REINICIOS) {
        limparTela();
        
        // Move inimigos antes de mostrar o mapa (só se não morreu)
        if (numInimigos > 0 && !morreu) {
            moverInimigos(mapa, altura, largura);
            
            // Verifica colisão após movimento dos inimigos
            if (verificarColisaoInimigo()) {
                reinicios++;
                printf("Você foi pego por um monstro! Reiniciando a fase (%d/%d)...\n", reinicios, MAX_REINICIOS);
                sleep(2);
                
                if (reinicios >= MAX_REINICIOS) break;
                
                // Reinicia a fase
                liberarMapa(mapa, altura);
                mapa = reiniciarFase(altura, largura);
                morreu = 1;
                continue;
            }
        }
        
        morreu = 0; // Reset da flag de morte
        mostrarMapa(mapa, altura, largura);
        printf("Comando (WASD: mover, i: interagir, q: sair): ");

        comando = getch();

        if (comando == 'q') return 0;
        if (comando == 'i') interagir(mapa, altura, largura);
        else {
            int resultado = moverJogador(mapa, altura, largura, comando);
            if (resultado == 1) {
                printf("Você passou pela porta!\n");
                sleep(1);
                return 1;
            } else if (resultado == -1) {
                // Jogador foi pego durante o movimento
                if (reinicios >= MAX_REINICIOS) break;
                
                // Reinicia a fase
                liberarMapa(mapa, altura);
                mapa = reiniciarFase(altura, largura);
                morreu = 1;
                continue;
            }
        }
    }
    return -1;
}

// -------------------- Criar Fases -------------------- //

char **criarFase(int tam) {
    char **mapa = alocarMapa(tam, tam);
    inicializarInimigos();
    
    for (int i = 0; i < tam; i++) {
        for (int j = 0; j < tam; j++) {
            mapa[i][j] = ' ';
        }
    }

    for (int i = 0; i < tam; i++) {
        mapa[0][i] = mapa[tam-1][i] = '*';
        mapa[i][0] = mapa[i][tam-1] = '*';
    }

    mapa[1][1] = '&'; jogador.x = 1; jogador.y = 1;
    mapa[3][3] = '@';
    mapa[tam-2][tam-2] = 'D';

    // Adiciona inimigos nas fases 1, 2 e 3
    if (tam == FASE1_TAM) {
        // Fase 1: 2 inimigos normais (X)
        adicionarInimigo(5, 5, 0);
        adicionarInimigo(7, 3, 0);
        mapa[5][5] = 'X';
        mapa[7][3] = 'X';
    } else if (tam == FASE2_TAM) {
        // Fase 2: 2 inimigos normais (X) e 1 inteligente (V)
        adicionarInimigo(8, 8, 0);
        adicionarInimigo(12, 5, 0);
        adicionarInimigo(6, 15, 1);
        mapa[8][8] = 'X';
        mapa[12][5] = 'X';
        mapa[6][15] = 'V';
    } else if (tam == FASE3_TAM) {
        // Fase 3: 1 inimigo normal (X) e 2 inteligentes (V)
        adicionarInimigo(10, 10, 0);
        adicionarInimigo(15, 20, 1);
        adicionarInimigo(25, 8, 1);
        mapa[10][10] = 'X';
        mapa[15][20] = 'V';
        mapa[25][8] = 'V';
    }

    return mapa;
}

// -------------------- Telas -------------------- //

void telaCreditos() {
    limparTela();
    printf("=== Créditos ===\n");
    printf("Desenvolvido por: Francisco Begot, Adisson Pires e Apoliano Neto\n");
    printf("Atualização: Sistema de inimigos com movimento aleatório\n");
    printf("Pressione ENTER para voltar...\n");
    getchar(); getchar();
}

void telaVitoria() {
    limparTela();
    printf("Parabéns! Você completou todas as fases!\n");
    printf("Você é um verdadeiro herói da masmorra!\n");
    printf("Conseguiu escapar de todos os monstros!\n");
    printf("Pressione ENTER para voltar ao menu principal...\n");
    getchar(); getchar();
}

void telaDerrota() {
    limparTela();
    printf("Você perdeu! Os monstros foram mais espertos desta vez...\n");
    printf("Tente novamente da próxima vez!\n");
    printf("Pressione ENTER para voltar ao menu principal...\n");
    getchar(); getchar();
}

void telaSair() {
    limparTela();
    printf("Obrigado por jogar! Até a próxima!\n");
    sleep(2);
}

void telaInstrucoes() {
    limparTela();
    printf("=== Instruções ===\n");
    printf("Símbolos do jogo:\n");
    printf("& - Jogador\n");
    printf("X - Monstro nível 1 (movimento aleatório)\n");
    printf("V - Monstro nível 2 (segue o jogador!)\n");
    printf("@ - Chave\n");
    printf("D - Porta fechada\n");
    printf("= - Porta aberta\n");
    printf("P - NPC\n");
    printf("* - Parede\n");
    printf("O - Botão\n\n");
    printf("Controles:\n");
    printf("WASD - Mover\n");
    printf("i - Interagir\n");
    printf("q - Sair da fase\n\n");
    printf("Objetivo: Pegue a chave e passe pela porta sem ser pego pelos monstros!\n");
    printf("Pressione ENTER para voltar...\n");
    getchar(); getchar();
}

// -------------------- Menu Principal -------------------- //

void telaMenu() {
    int opcao;
    do {
        limparTela();
        printf("==== Dungeon Crawler ====\n");
        printf("1 - Jogar\n");
        printf("2 - Instruções\n");
        printf("3 - Créditos\n");
        printf("4 - Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);

        if (opcao == 1) {
            char **vila = criarVila();
            if (jogarFase(vila, VILA_TAM, VILA_TAM) == 1) {
                liberarMapa(vila, VILA_TAM);
                char **fase1 = criarFase(FASE1_TAM);
                if (jogarFase(fase1, FASE1_TAM, FASE1_TAM) == 1) {
                    liberarMapa(fase1, FASE1_TAM);
                    char **fase2 = criarFase(FASE2_TAM);
                    if (jogarFase(fase2, FASE2_TAM, FASE2_TAM) == 1) {
                        liberarMapa(fase2, FASE2_TAM);
                        char **fase3 = criarFase(FASE3_TAM);
                        if (jogarFase(fase3, FASE3_TAM, FASE3_TAM) == 1) {
                            liberarMapa(fase3, FASE3_TAM);
                            telaVitoria();
                        } else {
                            liberarMapa(fase3, FASE3_TAM);
                            telaDerrota();
                        }
                    } else {
                        liberarMapa(fase2, FASE2_TAM);
                        telaDerrota();
                    }
                } else {
                    liberarMapa(fase1, FASE1_TAM);
                    telaDerrota();
                }
            } else {
                liberarMapa(vila, VILA_TAM);
                telaDerrota();
            }
        } else if (opcao == 2) {
            telaInstrucoes();
        } else if (opcao == 3) {
            telaCreditos();
        }
    } while (opcao != 4);

    telaSair();
}

// -------------------- Função principal -------------------- //

int main() {
    srand(time(NULL));
    telaMenu();
    return 0;
}
