#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define VILA_TAM 10
#define FASE1_TAM 10
#define FASE2_TAM 20
#define FASE3_TAM 40
#define MAX_REINICIOS 3
#define MAX_INIMIGOS 5
#define MAX_ESPINHOS 20

typedef struct {
    int x, y;
} Posicao;

typedef struct {
    Posicao pos;
    int ativo;
    int tipo; // 0 = monstro normal (X), 1 = monstro inteligente (V)
} Inimigo;

typedef struct {
    Posicao pos;
    int ativo;
} Espinho;

Posicao jogador;
Inimigo inimigos[MAX_INIMIGOS];
Espinho espinhos[MAX_ESPINHOS];
int numInimigos = 0;
int numEspinhos = 0;
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

// -------------------- Animation Functions -------------------- //

void microsleep(int microseconds) {
    usleep(microseconds);
}

void fadeOut() {
    // Fade out effect with dots
    printf("\n");
    for (int i = 0; i < 10; i++) {
        printf(".");
        fflush(stdout);
        microsleep(50000); // 50ms
    }
    printf("\n");
}

void fadeIn() {
    // Clear screen with smooth transition
    printf("\033[2J\033[H"); // Clear screen and move cursor to home
    fflush(stdout);
    microsleep(100000); // 100ms pause
}

void slideText(const char* text, int delay_ms) {
    int len = strlen(text);
    for (int i = 0; i <= len; i++) {
        printf("\r");
        for (int j = 0; j < i; j++) {
            printf("%c", text[j]);
        }
        fflush(stdout);
        microsleep(delay_ms * 1000);
    }
    printf("\n");
}

void typewriterEffect(const char* text, int delay_ms) {
    int len = strlen(text);
    for (int i = 0; i < len; i++) {
        printf("%c", text[i]);
        fflush(stdout);
        microsleep(delay_ms * 1000);
    }
}

void animatedBorder() {
    const char* corners[] = {"┌", "┐", "┘", "└"};
    
    // Animate border drawing
    for (int i = 0; i < 4; i++) {
        printf("%s", corners[i]);
        fflush(stdout);
        microsleep(100000);
    }
}

void limparTela() {
    fadeOut();
    int result = system("clear");
    (void)result; // Suppress unused warning
    fadeIn();
}

// -------------------- Funções utilitárias -------------------- //

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

// -------------------- Funções de Espinhos -------------------- //

void inicializarEspinhos() {
    for (int i = 0; i < MAX_ESPINHOS; i++) {
        espinhos[i].ativo = 0;
    }
    numEspinhos = 0;
}

int posicaoTemEspinho(int x, int y) {
    for (int i = 0; i < numEspinhos; i++) {
        if (espinhos[i].ativo && espinhos[i].pos.x == x && espinhos[i].pos.y == y) {
            return 1;
        }
    }
    return 0;
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

int posicaoTemInimigo(int x, int y) {
    for (int i = 0; i < numInimigos; i++) {
        if (inimigos[i].ativo && inimigos[i].pos.x == x && inimigos[i].pos.y == y) {
            return 1;
        }
    }
    return 0;
}

void ativarEspinhos(char **mapa, int altura, int largura, int fase) {
    printf("Ativando espinhos...\n");
    sleep(1);
    
    // Número de espinhos baseado na fase
    int novosEspinhos = 3 + fase * 2; // Fase 1: 5 espinhos, Fase 2: 7 espinhos, Fase 3: 9 espinhos
    
    for (int i = 0; i < novosEspinhos && numEspinhos < MAX_ESPINHOS; i++) {
        int tentativas = 0;
        while (tentativas < 100) {
            int x = rand() % altura;
            int y = rand() % largura;
            
            // Verifica se a posição é válida para colocar espinho
            if (mapa[x][y] == ' ' && 
                !(x == jogador.x && y == jogador.y) &&
                !posicaoTemInimigo(x, y) &&
                !posicaoTemEspinho(x, y)) {
                
                espinhos[numEspinhos].pos.x = x;
                espinhos[numEspinhos].pos.y = y;
                espinhos[numEspinhos].ativo = 1;
                mapa[x][y] = '#';
                numEspinhos++;
                break;
            }
            tentativas++;
        }
    }
    
    printf("Espinhos ativados! Cuidado onde pisa!\n");
    sleep(2);
}

int posicaoOcupada(char **mapa, int x, int y, int altura, int largura) {
    // Verifica se a posição está fora dos limites
    if (x < 0 || y < 0 || x >= altura || y >= largura) return 1;
    
    // Verifica se há parede ou objeto fixo
    char cell = mapa[x][y];
    if (cell == '*' || cell == '@' || cell == 'P' || cell == 'O' || cell == 'D' || cell == '=' || cell == '#') return 1;
    
    // Verifica se há outro inimigo na posição
    if (posicaoTemInimigo(x, y)) return 1;
    
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

void interagir(char **mapa, int altura, int largura, int fase) {
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
            printf("NPC: Use WASD para mover, 'i' para interagir, 'o' para ativar espinhos! Cuidado com os monstros X e V!\n");
            sleep(2);
            return;
        } else if (obj == 'O') {
            printf("Você ativou o botão dos espinhos!\n");
            ativarEspinhos(mapa, altura, largura, fase);
            return;
        }
    }
}

// -------------------- Declaração de funções -------------------- //

char **criarFase(int tam, int fase);
char **reiniciarFase(int altura, int largura, int fase);

// -------------------- Criar Vila -------------------- //

char **criarVila() {
    char **mapa = alocarMapa(VILA_TAM, VILA_TAM);
    inicializarInimigos();
    inicializarEspinhos();

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
    mapa[3][3] = 'O'; // Botão para ativar espinhos

    return mapa;
}

// -------------------- Criar Fase -------------------- //

char **criarFase(int tam, int fase) {
    char **mapa = alocarMapa(tam, tam);
    inicializarInimigos();
    inicializarEspinhos();

    // Inicializar mapa vazio
    for (int i = 0; i < tam; i++) {
        for (int j = 0; j < tam; j++) {
            mapa[i][j] = ' ';
        }
    }

    // Criar bordas
    for (int i = 0; i < tam; i++) {
        mapa[0][i] = mapa[tam-1][i] = '*';
        mapa[i][0] = mapa[i][tam-1] = '*';
    }

    // Adicionar algumas paredes internas
    int numParedes = tam / 4;
    for (int i = 0; i < numParedes; i++) {
        int x = (rand() % (tam - 4)) + 2;
        int y = (rand() % (tam - 4)) + 2;
        if (!(x == 1 && y == 1)) {
            mapa[x][y] = '*';
        }
    }

    // Posicionar jogador
    mapa[1][1] = '&';
    jogador.x = 1;
    jogador.y = 1;

    // Posicionar objetos
    int x, y;
    
    // Chave
    do {
        x = (rand() % (tam - 4)) + 2;
        y = (rand() % (tam - 4)) + 2;
    } while (mapa[x][y] != ' ');
    mapa[x][y] = '@';

    // NPC
    do {
        x = (rand() % (tam - 4)) + 2;
        y = (rand() % (tam - 4)) + 2;
    } while (mapa[x][y] != ' ');
    mapa[x][y] = 'P';

    // Botão dos espinhos
    do {
        x = (rand() % (tam - 4)) + 2;
        y = (rand() % (tam - 4)) + 2;
    } while (mapa[x][y] != ' ');
    mapa[x][y] = 'O';

    // Porta
    do {
        x = (rand() % (tam - 4)) + 2;
        y = (rand() % (tam - 4)) + 2;
    } while (mapa[x][y] != ' ');
    mapa[x][y] = 'D';

    // Saída
    mapa[tam-2][tam-2] = '=';

    // Adicionar inimigos
    int numInimigosParaFase = fase + 1;
    if (numInimigosParaFase > MAX_INIMIGOS) numInimigosParaFase = MAX_INIMIGOS;
    
    for (int i = 0; i < numInimigosParaFase; i++) {
        do {
            x = (rand() % (tam - 4)) + 2;
            y = (rand() % (tam - 4)) + 2;
        } while (mapa[x][y] != ' ');
        
        int tipo = (i < numInimigosParaFase / 2) ? 0 : 1; // Metade normal, metade inteligente
        adicionarInimigo(x, y, tipo);
        mapa[x][y] = (tipo == 0) ? 'X' : 'V';
    }

    return mapa;
}

// -------------------- Reiniciar Fase -------------------- //

char **reiniciarFase(int altura, int largura __attribute__((unused)), int fase) {
    chaveColetada = 0;
    
    if (altura == VILA_TAM) {
        return criarVila();
    } else {
        return criarFase(altura, fase);
    }
}

// -------------------- Jogar Fase -------------------- //

int jogarFase(char **mapa, int altura, int largura, int fase) {
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
                mapa = reiniciarFase(altura, largura, fase);
                morreu = 1;
                continue;
            }
        }
        
        morreu = 0;
        mostrarMapa(mapa, altura, largura);
        printf("Fase: %d | Comando (WASD: mover, i: interagir, o: ativar espinhos, q: sair): ", fase);

        comando = getch();

        if (comando == 'q') return 0;
        if (comando == 'i') interagir(mapa, altura, largura, fase);
        else if (comando == 'o') {
            // Verifica se há um botão 'O' próximo
            int dirs[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
            int botaoEncontrado = 0;
            
            for (int i = 0; i < 4; i++) {
                int nx = jogador.x + dirs[i][0];
                int ny = jogador.y + dirs[i][1];
                
                if (nx >= 0 && ny >= 0 && nx < altura && ny < largura && mapa[nx][ny] == 'O') {
                    botaoEncontrado = 1;
                    break;
                }
            }
            
            if (botaoEncontrado) {
                ativarEspinhos(mapa, altura, largura, fase);
            } else {
                printf("Não há nenhum botão de espinhos por perto!\n");
                sleep(1);
            }
        }
        else {
            int resultado = moverJogador(mapa, altura, largura, comando);
            if (resultado == 1) {
                printf("Você passou pela porta!\n");
                sleep(1);
                return 1;
            } else if (resultado == -1) {
                if (reinicios >= MAX_REINICIOS) break;
                
                liberarMapa(mapa, altura);
                mapa = reiniciarFase(altura, largura, fase);
                morreu = 1;
                continue;
            }
        }
    }

    return 0;
}

// -------------------- Menu Principal -------------------- //

void mostrarMenu() {
    printf("\033[2J\033[H"); // Clear screen
    fflush(stdout);
    
    // Animated title reveal
    typewriterEffect("╔════════════════════════════════════════╗\n", 15);
    typewriterEffect("║            DUNGEON CRAWLER             ║\n", 15);
    typewriterEffect("╠════════════════════════════════════════╣\n", 15);
    
    microsleep(200000); // Pause for effect
    
    // Slide in menu options one by one
    typewriterEffect("║                                        ║\n", 10);
    slideText("║  1. Jogar Tutorial (Vila)              ║", 5);
    slideText("║  2. Jogar Fase 1 (10x10)              ║", 5);
    slideText("║  3. Jogar Fase 2 (20x20)              ║", 5);
    slideText("║  4. Jogar Fase 3 (40x40)              ║", 5);
    slideText("║  5. Jogar Campanha Completa           ║", 5);
    slideText("║  6. Ver Instrucoes  + Historia                    ║", 5);
    slideText("║  7. Sair                               ║", 5);
    
    typewriterEffect("║                                        ║\n", 10);
    typewriterEffect("╚════════════════════════════════════════╝\n", 15);
    
    microsleep(300000);
    
    // Animated prompt
    printf("\n");
    typewriterEffect("Escolha uma opcao (1-7): ", 30);
}

void mostrarInstrucoes() {
    // Smooth transition to instructions
    fadeOut();
    printf("\033[2J\033[H");
    microsleep(200000);

    // História introdutória
    typewriterEffect("╔════════════════════════════════════════╗\n", 20);
    typewriterEffect("║             HISTORIA DO JOGO           ║\n", 20);
    typewriterEffect("╠════════════════════════════════════════╣\n", 20);
    
    microsleep(300000);
    
    slideText("║ Você desperta nas profundezas de uma   ║", 8);
    slideText("║ masmorra esquecida, onde lendas falam  ║", 8);
    slideText("║ de tesouros ocultos e monstros mortais.║", 8);
    slideText("║ Para escapar, precisará encontrar a    ║", 8);
    slideText("║ chave que abre a saída, evitando       ║", 8);
    slideText("║ armadilhas e enfrentando perigos.      ║", 8);
    slideText("║                                        ║", 8);
    slideText("║ Cuidado! Monstros inteligentes e       ║", 8);
    slideText("║ armadilhas letais guardam cada canto.  ║", 8);
    slideText("║ Você conseguirá sobreviver?            ║", 8);
    
    typewriterEffect("╚════════════════════════════════════════╝\n", 20);
    microsleep(400000);

    // Animated instructions reveal
    typewriterEffect("╔════════════════════════════════════════╗\n", 20);
    typewriterEffect("║              INSTRUCOES                ║\n", 20);
    typewriterEffect("╠════════════════════════════════════════╣\n", 20);
    
    microsleep(300000);
    
    typewriterEffect("║                                        ║\n", 10);
    slideText("║ SÍMBOLOS:                              ║", 8);
    slideText("║   & = Jogador                          ║", 5);
    slideText("║   X = Monstro Normal                   ║", 5);
    slideText("║   V = Monstro Inteligente              ║", 5);
    slideText("║   @ = Chave                            ║", 5);
    slideText("║   D = Porta Trancada                   ║", 5);
    slideText("║   = = Porta Aberta/Saída               ║", 5);
    slideText("║   P = NPC                              ║", 5);
    slideText("║   O = Botão dos Espinhos               ║", 5);
    slideText("║   # = Espinho (armadilha)              ║", 5);
    slideText("║   * = Parede                           ║", 5);
    
    typewriterEffect("║                                        ║\n", 10);
    microsleep(200000);
    
    slideText("║ CONTROLES:                             ║", 8);
    slideText("║   WASD = Mover                         ║", 5);
    slideText("║   I = Interagir                        ║", 5);
    slideText("║   O = Ativar espinhos                  ║", 5);
    slideText("║   Q = Sair para menu                   ║", 5);
    
    typewriterEffect("║                                        ║\n", 10);
    microsleep(200000);
    
    slideText("║ OBJETIVO:                              ║", 8);
    slideText("║   1. Colete a chave                    ║", 5);
    slideText("║   2. Evite monstros e espinhos         ║", 5);
    slideText("║   3. Chegue até a saída                ║", 5);
    
    typewriterEffect("║                                        ║\n", 10);
    typewriterEffect("╚════════════════════════════════════════╝\n", 20);
    
    microsleep(400000);
    printf("\n");
    typewriterEffect("Pressione qualquer tecla para voltar...", 40);
    getch();
}


void animatedPhaseTransition(const char* phaseName) {
    fadeOut();
    printf("\033[2J\033[H");
    microsleep(300000);
    
    // Center the phase announcement
    printf("\n\n\n\n");
    printf("          ╔══════════════════════════════╗\n");
    printf("          ║                              ║\n");
    printf("          ║     ");
    typewriterEffect(phaseName, 50);
    printf("     ║\n");
    printf("          ║                              ║\n");
    printf("          ╚══════════════════════════════╝\n");
    
    microsleep(500000);
    
    printf("\n");
    typewriterEffect("          Preparando ambiente...", 30);
    
    // Loading animation
    printf("\n          ");
    for (int i = 0; i < 20; i++) {
        printf("█");
        fflush(stdout);
        microsleep(100000);
    }
    
    microsleep(300000);
    printf("\n\n");
    typewriterEffect("          Pressione qualquer tecla para comecar...", 40);
    getch();
}

void animatedResultScreen(int sucesso, const char* message) {
    fadeOut();
    printf("\033[2J\033[H");
    microsleep(200000);
    
    printf("\n\n\n");
    
    if (sucesso) {
        // Victory animation
        printf("          ╔══════════════════════════════╗\n");
        printf("          ║         ");
        typewriterEffect(" SUCESSO!  ", 80);
        printf("         ║\n");
        printf("          ╠══════════════════════════════╣\n");
        printf("          ║                              ║\n");
        printf("          ║   ");
        typewriterEffect(message, 30);
        printf("   ║\n");
        printf("          ║                              ║\n");
        printf("          ╚══════════════════════════════╝\n");
    } else {
        // Game over animation
        printf("          ╔══════════════════════════════╗\n");
        printf("          ║         ");
        typewriterEffect(" GAME OVER ", 80);
        printf("         ║\n");
        printf("          ╠══════════════════════════════╣\n");
        printf("          ║                              ║\n");
        printf("          ║   ");
        typewriterEffect(message, 30);
        printf("   ║\n");
        printf("          ║                              ║\n");
        printf("          ╚══════════════════════════════╝\n");
    }
    
    microsleep(800000);
    printf("\n");
    typewriterEffect("          Pressione qualquer tecla para continuar...", 40);
    getch();
}

int executarFase(int tipoFase) {
    char **mapa;
    int tamanho, fase;
    const char* phaseName;
    
    switch(tipoFase) {
        case 1: // Vila/Tutorial
            phaseName = "VILA (Tutorial)";
            animatedPhaseTransition(phaseName);
            mapa = criarVila();
            tamanho = VILA_TAM;
            fase = 0;
            break;
        case 2: // Fase 1
            phaseName = "FASE 1";
            animatedPhaseTransition(phaseName);
            mapa = criarFase(FASE1_TAM, 1);
            tamanho = FASE1_TAM;
            fase = 1;
            break;
        case 3: // Fase 2
            phaseName = "FASE 2";
            animatedPhaseTransition(phaseName);
            mapa = criarFase(FASE2_TAM, 2);
            tamanho = FASE2_TAM;
            fase = 2;
            break;
        case 4: // Fase 3
            phaseName = "FASE 3";
            animatedPhaseTransition(phaseName);
            mapa = criarFase(FASE3_TAM, 3);
            tamanho = FASE3_TAM;
            fase = 3;
            break;
        default:
            return 0;
    }
    
    int resultado = jogarFase(mapa, tamanho, tamanho, fase);
    liberarMapa(mapa, tamanho);
    
    if (resultado) {
        animatedResultScreen(1, "Parabéns! Fase completada!");
    } else {
        animatedResultScreen(0, "Você foi derrotado nesta fase.");
    }
    
    return resultado;
}

void jogarCampanha() {
    fadeOut();
    printf("\033[2J\033[H");
    microsleep(200000);
    
    printf("\n\n\n");
    printf("      ╔══════════════════════════════════════╗\n");
    printf("      ║         ");
    typewriterEffect("CAMPANHA COMPLETA", 60);
    printf("         ║\n");
    printf("      ╠══════════════════════════════════════╣\n");
    printf("      ║                                      ║\n");
    printf("      ║   ");
    typewriterEffect("Você vai jogar todas as fases", 30);
    printf("   ║\n");
    printf("      ║       ");
    typewriterEffect("em sequência!", 40);
    printf("              ║\n");
    printf("      ║                                      ║\n");
    printf("      ╚══════════════════════════════════════╝\n");
    
    microsleep(800000);
    printf("\n");
    typewriterEffect("      Pressione qualquer tecla para comecar...", 40);
    getch();
    
    // Vila (tutorial)
    if (!executarFase(1)) {
        return;
    }
    
    // Fase 1
    if (!executarFase(2)) {
        return;
    }
    
    // Fase 2
    if (!executarFase(3)) {
        return;
    }
    
    // Fase 3
    if (!executarFase(4)) {
        return;
    }
    
    // Vitória completa com animação especial
    fadeOut();
    printf("\033[2J\033[H");
    microsleep(500000);
    
    printf("\n\n\n");
    printf("      ╔══════════════════════════════════════╗\n");
    printf("      ║           ");
    typewriterEffect(" PARABENS! ", 100);
    printf("           ║\n");
    printf("      ╠══════════════════════════════════════╣\n");
    printf("      ║                                      ║\n");
    printf("      ║   ");
    typewriterEffect("Você completou toda a campanha!", 40);
    printf("   ║\n");
    printf("      ║        ");
    typewriterEffect("Você é um verdadeiro", 30);
    printf("        ║\n");
    printf("      ║       ");
    typewriterEffect("explorador de dungeons!", 30);
    printf("       ║\n");
    printf("      ║                                      ║\n");
    
    // Animated stars
    microsleep(500000);
    printf("      ║          ");
    for (int i = 0; i < 5; i++) {
        printf("★ ");
        fflush(stdout);
        microsleep(200000);
    }
    printf("         ║\n");
    
    printf("      ╚══════════════════════════════════════╝\n");
    
    microsleep(1000000);
    printf("\n");
    typewriterEffect("      Pressione qualquer tecla para voltar ao menu...", 40);
    getch();
}

// -------------------- Função Principal -------------------- //

int main() {
    srand(time(NULL));
    
    char opcao;
    int continuar = 1;
    
    while (continuar) {
        mostrarMenu();
        opcao = getch();
        
        switch(opcao) {
            case '1':
                executarFase(1);
                break;
            case '2':
                executarFase(2);
                break;
            case '3':
                executarFase(3);
                break;
            case '4':
                executarFase(4);
                break;
            case '5':
                jogarCampanha();
                break;
            case '6':
                mostrarInstrucoes();
                break;
            case '7':
                continuar = 0;
                break;
            default:
                // Animated error message
                printf("\n");
                typewriterEffect(" Opcao invalida! ", 50);
                printf("Escolha entre 1-7.\n");
                
                // Brief shake effect
                for (int i = 0; i < 3; i++) {
                    printf("\r");
                    fflush(stdout);
                    microsleep(100000);
                    printf(" ");
                    fflush(stdout);
                    microsleep(100000);
                    printf("\r");
                    fflush(stdout);
                }
                
                typewriterEffect("Pressione qualquer tecla para continuar...", 40);
                getch();
                break;
        }
    }
    
    // Animated exit sequence
    fadeOut();
    printf("\033[2J\033[H");
    microsleep(300000);
    
    printf("\n\n\n\n");
    printf("          ╔══════════════════════════════╗\n");
    printf("          ║                              ║\n");
    printf("          ║    ");
    typewriterEffect("Obrigado por jogar!", 60);
    printf("    ║\n");
    printf("          ║                              ║\n");
    printf("          ║      ");
    typewriterEffect("DUNGEON CRAWLER", 80);
    printf("      ║\n");
    printf("          ║                              ║\n");
    printf("          ║   ");
    typewriterEffect("Até a próxima aventura!", 50);
    printf("   ║\n");
    printf("          ║                              ║\n");
    printf("          ╚══════════════════════════════╝\n");
    
    microsleep(800000);
    
    // Fade out effect
    printf("\n");
    for (int i = 0; i < 15; i++) {
        printf(".");
        fflush(stdout);
        microsleep(100000);
    }
    printf("\n\n");
    
    return 0;
}
