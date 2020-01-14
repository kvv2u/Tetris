#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

extern int errno;
struct termios otty, ntty;
int kbhit(void);    // キー入力があったかどうか
int getch(void);    // キー入力1文字読み込み
int tinit(void);    // 端末の初期化

#define clearScreen() printf("\e[2J")
#define setPosition(x,y) printf("\e[%d;%dH",(y)+1,(x)*2+1)
#define cursolOn() printf("\e[?25h") //カーソルを表示
#define cursolOff() printf("\e[?25l") //カーソルを非表示
#define WIDTH 10 //フィールドの幅
#define HEIGHT 24 //フィールドの高さ

#define setCharColor(n) printf("\e[3%dm",(n))
#define setBackColor(n) printf("\e[4%dm",(n))
#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7
#define DEFAULT 9

#define setAttribute(n) printf("\e[%dm",(n))
#define NORMAL 0 //通常
#define BLIGHT 1 //明るく
#define DIM 2 //暗く
#define UNDERBAR 4 //下線
#define BLINK 5 //点滅
#define REVERSE 7 //明暗反転
#define HIDE 8 //隠れ(見えない)
#define STRIKE 9 //取り消し線
#define BLOCK_SIZE 4 // ブロックのサイズ
#define BLOCK_NUM 7 // ブロックの種類

typedef struct cell {
    char c;
    int charcolor;
    int backcolor;
    int attribute;
} Cell;

int wait(int msec); //①関数プロトタイプ宣言
void initialize(void); //画面の初期化 (①関数プロトタイプ部分)
void reset(void); //画面の復元 (①関数プロトタイプ部分)
int checkRange(Cell a, int x, int y);
int printCell(Cell c, int x, int y);
int clearCell(Cell c, int x, int y);
void copyBlock(Cell src[BLOCK_SIZE][BLOCK_SIZE], Cell dst[BLOCK_SIZE][BLOCK_SIZE]);
int printBlock(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y);
int clearBlock(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y);
void rotateBlock(Cell src[BLOCK_SIZE][BLOCK_SIZE], Cell dst[BLOCK_SIZE][BLOCK_SIZE]);
int checkCell(Cell a, int x, int y);                                // セルの衝突検知
int checkMap(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y);     // ブロックの衝突検知
void putMap(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y);      // マップにセルを登録
void printMap(void);        // 画面にマップに積み重なったセルを全部表示する
int checkLine(int y);       // 一行揃ったかどうか
void deleteLine(int ys);    // 揃った行を一行マップから消滅させる
int deleteMap(void);     // 揃った行を全部マップから消去する

Cell block_type[BLOCK_NUM][BLOCK_SIZE][BLOCK_SIZE] = {
 {{'\0', RED, BLACK, NORMAL,
   '\0', RED, BLACK, NORMAL,
   ' ', RED, BLACK, REVERSE,
   '\0', RED, BLACK, NORMAL},
  {'\0', RED, BLACK, NORMAL,
   ' ', RED, BLACK, REVERSE,
   ' ', RED, BLACK, REVERSE,
   '\0', RED, BLACK, NORMAL},
  {'\0', RED, BLACK, NORMAL,
   ' ', RED, BLACK, REVERSE,
   '\0', RED, BLACK, NORMAL,
   '\0', RED, BLACK, NORMAL},
  {'\0', RED, BLACK, NORMAL,
   '\0', RED, BLACK, NORMAL,
   '\0', RED, BLACK, NORMAL,
   '\0', RED, BLACK, NORMAL}},
  
 {{'\0', BLUE, BLACK, NORMAL,
   ' ', BLUE, BLACK, REVERSE,
   ' ', BLUE, BLACK, REVERSE,
   '\0', BLUE, BLACK, NORMAL},
  {'\0', BLUE, BLACK, NORMAL,
   ' ', BLUE, BLACK, REVERSE,
   '\0', BLUE, BLACK, NORMAL,
   '\0', BLUE, BLACK, NORMAL},
  {'\0', BLUE, BLACK, NORMAL,
   ' ', BLUE, BLACK, REVERSE,
   '\0', BLUE, BLACK, NORMAL,
   '\0', BLUE, BLACK, NORMAL},
  {'\0', BLUE, BLACK, NORMAL,
   '\0', BLUE, BLACK, NORMAL,
   '\0', BLUE, BLACK, NORMAL,
   '\0', BLUE, BLACK, NORMAL}},
 
 {{'\0', GREEN, BLACK, NORMAL,
   '\0', GREEN, BLACK, NORMAL,
   '\0', GREEN, BLACK, NORMAL,
   '\0', GREEN, BLACK, NORMAL},
  {'\0', GREEN, BLACK, NORMAL,
   ' ', GREEN, BLACK, REVERSE,
   ' ', GREEN, BLACK, REVERSE,
   '\0', GREEN, BLACK, NORMAL},
  {' ', GREEN, BLACK, REVERSE,
   ' ', GREEN, BLACK, REVERSE,
   '\0', GREEN, BLACK, NORMAL,
   '\0', GREEN, BLACK, NORMAL},
  {'\0', GREEN, BLACK, NORMAL,
   '\0', GREEN, BLACK, NORMAL,
   '\0', GREEN, BLACK, NORMAL,
   '\0', GREEN, BLACK, NORMAL}},
 
 {{'\0', YELLOW, BLACK, NORMAL,
   '\0', YELLOW, BLACK, NORMAL,
   '\0', YELLOW, BLACK, NORMAL,
   '\0', YELLOW, BLACK, NORMAL},
  {'\0', YELLOW, BLACK, NORMAL,
   ' ', YELLOW, BLACK, REVERSE,
   ' ', YELLOW, BLACK, REVERSE,
   '\0', YELLOW, BLACK, NORMAL},
  {'\0', YELLOW, BLACK, NORMAL,
   ' ', YELLOW, BLACK, REVERSE,
   ' ', YELLOW, BLACK, REVERSE,
   '\0', YELLOW, BLACK, NORMAL},
  {'\0', YELLOW, BLACK, NORMAL,
   '\0', YELLOW, BLACK, NORMAL,
   '\0', YELLOW, BLACK, NORMAL,
   '\0', YELLOW, BLACK, NORMAL}},
 
 {{'\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL},
  {' ', CYAN, BLACK, REVERSE,
   ' ', CYAN, BLACK, REVERSE,
   ' ', CYAN, BLACK, REVERSE,
   ' ', CYAN, BLACK, REVERSE},
  {'\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL},
  {'\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL,
   '\0', CYAN, BLACK, NORMAL}},
 
 {{'\0', MAGENTA, BLACK, NORMAL,
   '\0', MAGENTA, BLACK, NORMAL,
   '\0', MAGENTA, BLACK, NORMAL,
   '\0', MAGENTA, BLACK, NORMAL},
  {' ', MAGENTA, BLACK, REVERSE,
   ' ', MAGENTA, BLACK, REVERSE,
   ' ', MAGENTA, BLACK, REVERSE,
   '\0', MAGENTA, BLACK, NORMAL},
  {'\0', MAGENTA, BLACK, NORMAL,
   ' ', MAGENTA, BLACK, REVERSE,
   '\0', MAGENTA, BLACK, NORMAL,
   '\0', MAGENTA, BLACK, NORMAL},
  {'\0', MAGENTA, BLACK, NORMAL,
   '\0', MAGENTA, BLACK, NORMAL,
   '\0', MAGENTA, BLACK, NORMAL,
   '\0', MAGENTA, BLACK, NORMAL}},
 
 {{'\0', WHITE, BLACK, NORMAL,
   ' ', WHITE, BLACK, REVERSE,
   '\0', WHITE, BLACK, NORMAL,
   '\0', WHITE, BLACK, NORMAL},
  {'\0', WHITE, BLACK, NORMAL,
   ' ', WHITE, BLACK, REVERSE,
   '\0', WHITE, BLACK, NORMAL,
   '\0', WHITE, BLACK, NORMAL},
  {'\0', WHITE, BLACK, NORMAL,
   ' ', WHITE, BLACK, REVERSE,
   ' ', WHITE, BLACK, REVERSE,
   '\0', WHITE, BLACK, NORMAL},
  {'\0', WHITE, BLACK, NORMAL,
   '\0', WHITE, BLACK, NORMAL,
   '\0', WHITE, BLACK, NORMAL,
   '\0', WHITE, BLACK, NORMAL}}
};

Cell map[HEIGHT][WIDTH];

int main(int argc, char *argv[])
{
    int x, y, c, prex, prey, t, next, score;
    Cell block[BLOCK_SIZE][BLOCK_SIZE], block_tmp[BLOCK_SIZE][BLOCK_SIZE];
    struct timeval start_time, now_time, pre_time;
    double duration, thold;
    score = 0;
    t = rand()%BLOCK_NUM;               // 最初のブロックの種類を決定
    next = rand()%BLOCK_NUM;
    copyBlock(block_type[t], block);
    initialize();
    printNext(next);
    printScore(score);
    x = 5;
    y = BLOCK_SIZE;
    thold = 0.5;                       // 落下の時間間隔
    printBlock(block, x, y);            // 初期表示
    gettimeofday(&start_time, NULL);    // 開始時刻
    pre_time = start_time;              // 前回落下時刻(最初は落下時刻)
    for(; ;)
    {   
        prex = x;
        prey = y;
        if(kbhit())
        {
            //printf("ssssss");
            //clearBlock(block, x, y);
            c = getch();
            if(c == 0x1b)
            {
                c = getch();
                if(c == 0x5b)
                {
                    c = getch();
                    switch(c)
                    {
                        case 0x41:  // UP
                            rotateBlock(block, block_tmp);      // ブロック回転
                            clearBlock(block, x, y);            // 元のブロック消去
                            printBlock(block_tmp, x, y);        // 回転後のブロック表示
                            copyBlock(block_tmp, block);        // 元のブロックに上書き
                            break;
                        case 0x42:  // DOWN
                            while(checkMap(block, x, y+1) == 0) {
                                y++;
                            }
                            score += y-prey;
                            printScore(score);
                            break;
                        case 0x43:  // RIGHT
                            if(checkMap(block, x+1, y) == 0) {
                                x++;
                            }
                            break;
                        case 0x44:  // LEFT
                            if(checkMap(block, x-1, y) == 0) {
                                x--;
                            } else {
                                x--;
                            }
                            break;
                    }
                } else {
                    reset();
                    exit(1);
                }  
            }
        }
        gettimeofday(&now_time, NULL);
        duration = now_time.tv_sec - pre_time.tv_sec
            + (now_time.tv_usec - pre_time.tv_usec)/1000000.0;   // 前回からの経過時間
        if (duration > thold)   // もしも落下時間間隔以上に時間経過していたら
        {
            pre_time = now_time;    // 落下時間を現在時刻に
            thold -= 0.001;
            if (checkMap(block, x, y+1) == 0) {       //  ブロックが落下し終わっていなければ
                y++;                // 一つ落下
            } else {
                int line;
                if (y == BLOCK_SIZE) {           // y が0(天井)だったら
                    reset();            // 終了処理
                    exit(1);
                }
                putMap(block, x, y);    // ブロック積み上げ
                line = deleteMap();     //  ライン消滅確認
                switch (line)
                {
                    case 1: score += 40;    break;
                    case 2: score += 100;   break;
                    case 3: score += 300;   break;
                    case 4: score += 1200;  break;
                }
                printScore(score);
                x = 5;                  // 最初から落下し直す
                y = BLOCK_SIZE;
                prex = 5;
                prey = BLOCK_SIZE;
                t = next;
                next = rand()%BLOCK_NUM;
                printNext(next);
                copyBlock(block_type[t], block);
                printBlock(block, x, y);
            }
            
        }
        if (prex != x || prey != y)         // もしもブロックが左右移動/落下していたら
        {
            clearBlock(block, prex, prey);  // 前回位置のブロックを消して
            printBlock(block, x, y);        // 新しい場所に表示
        }
    }
    reset();
}

int wait(int msec) //②関数の本体
{
 struct timespec r = {0, msec * 1000L * 1000L};
 return nanosleep( &r, NULL );
}
void initialize(void) //画面の初期化 (②関数本体)
{
    int x, y;
    Cell a = {'\0', BLACK, BLACK, NORMAL};
    tinit();
    setAttribute(NORMAL);
    setBackColor(BLACK);
    setCharColor(WHITE);
    clearScreen();
    cursolOff();
    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            map[y][x] = a;      // マップを空に
        }
    }
}
void reset(void) //画面の復元 (②関数本体)
{
    setBackColor(BLACK);
    setCharColor(WHITE);
    setAttribute(NORMAL);
    clearScreen();
    cursolOn();
    tcsetattr(1, TCSADRAIN, &otty);     //端末の復元、追加
    write(1, "\n", 1);
}
int checkRange(Cell a, int x, int y)
{
    if(a.c == '\0' || x<0 || y<0 || x>=WIDTH || y>=HEIGHT) {
        return -1; //失敗
    } else {
        return 0; //成功
    }
}
int printCell(Cell c, int x, int y)
{
    if(checkRange(c,x,y)==(-1)) {
        return -1;
    }
    setPosition(x, y);
    setAttribute(c.attribute);
    setBackColor(c.backcolor);
    setCharColor(c.charcolor);
    printf("%c%c", c.c, c.c);
    fflush(stdout);
    return 0;
}
int clearCell(Cell c, int x, int y)
{
    if(checkRange(c,x,y)==(-1)) {
        return -1;
    }
    setPosition(x, y);
    setAttribute(NORMAL);
    setBackColor(BLACK);
    setCharColor(BLACK);
    printf("  ");
    fflush(stdout);
    return 0;
}

void copyBlock(Cell src[BLOCK_SIZE][BLOCK_SIZE], Cell dst[BLOCK_SIZE][BLOCK_SIZE])
{
    int i, j;
    for (j = 0; j < BLOCK_SIZE; j++) {
        for (i = 0; i < BLOCK_SIZE; i++) {
            dst[j][i] = src[j][i];
        }
    }
}

int printBlock(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y)
{
    int i, j;
    for (j = 0; j < BLOCK_SIZE; j++) {
        for (i = 0; i < BLOCK_SIZE; i++) {
            printCell(block[j][i], i + x, j + y);
        }
    }
    return 0;    
}
int clearBlock(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y)
{
    int i, j;
    for (j = 0; j < BLOCK_SIZE; j++) {
        for (i = 0; i < BLOCK_SIZE; i++) {
            clearCell(block[j][i], i + x, j + y);
        }
    }
    return 0;    
}

int kbhit(void)
{
    int ret;
    fd_set rfd;     // FD: ファイルディスクリプタの略
    struct timeval timeout = {0, 0};
    FD_ZERO(&rfd);
    FD_SET(0, &rfd);    // 0: stdin
    ret = select(1, &rfd, NULL, NULL, &timeout);
    if (ret == 1) {
        return 1;
    } else {
        return 0;
    }
}
int getch(void)
{
    unsigned char c;
    int n;
    while ((n = read(0, &c, 1)) < 0 && errno == EINTR);
    if (n == 0) {   // 読み込むものがない(ファイルの終わり)
        return -1;
    } else {
        return (int)c;
    }
}
static void onsignal(int sig)   // 内部利用のシグナルハンドラ
{
    signal(sig, SIG_IGN);
    switch (sig) {
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
        case SIGHUP:
            exit(1);
            break;
    }
}
int tinit(void)
{
    if (tcgetattr(1, &otty) < 0) {
        return -1;
    }
    ntty = otty;
    ntty.c_iflag &= ~(INLCR|ICRNL|IXON|IXOFF|ISTRIP);
    ntty.c_oflag &= ~OPOST;
    ntty.c_lflag &= ~(ICANON|ECHO);
    ntty.c_cc[VMIN] = 1;
    ntty.c_cc[VTIME] = 0;
    tcsetattr(1, TCSADRAIN, &ntty);
    signal(SIGINT, onsignal);
    signal(SIGQUIT, onsignal);
    signal(SIGTERM, onsignal);
    signal(SIGHUP, onsignal);
}

void rotateBlock(Cell src[BLOCK_SIZE][BLOCK_SIZE], Cell dst[BLOCK_SIZE][BLOCK_SIZE])
{
    int i, j;
    for (j = 0; j < BLOCK_SIZE; j++)
    {
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            dst[i][BLOCK_SIZE -1 -j] = src[j][i];
        }
    }
}

int checkCell(Cell a, int x, int y)
{
    if (checkRange(a, x, y) || map[y][x].c != '\0')
    {
        return -1;      // 失敗
    }
    return 0;           // 成功
}
int checkMap(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y)
{
    int i, j;
    for (j = 0; j < BLOCK_SIZE; j++)
    {
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            if (block[j][i].c != '\0')
            {
                if (checkCell(block[j][i], x+i, y+j))
                {
                    return -1;      // どれか一つでも失敗ならブロック全体として失敗
                }
            }
        }
    }
    return 0;       // 全部表示できそうならば成功
}
void putMap(Cell block[BLOCK_SIZE][BLOCK_SIZE], int x, int y)
{
    int i, j;
    for (j = 0; j < BLOCK_SIZE; j++)
    {
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            if (checkCell(block[j][i], x+i, y+j) == 0)
            {
                map[y + j][x + i] = block[j][i];
            }
        }
    }
}

void printMap(void)
{
    int x, y;
    for(y = 0; y < HEIGHT; y++) {
        for(x = 0; x < WIDTH; x++) {
            printCell(map[y][x], x, y);
        }
    }
}
int checkLine(int y)
{
    int x;
    for(x = 0; x < WIDTH; x++) {
        if(map[y][x].c == '\0') {   // 空のセルがないか確認
            return -1;
        }
    }
    return 0;
}
void deleteLine(int ys)
{
    int x, y;
    for(y = ys; y > 0; y--) {           // 対象ラインより上のラインを繰り返し
        for(x = 0; x < WIDTH; x++) {    // 一行全部繰り返し
            map[y][x] = map[y-1][x];    // 一つ上のセルを一つ下に落とす
        }
    }
    setBackColor(BLACK);
    clearScreen();                      // 画面クリア
    printMap();                         // 再描画
}
int deleteMap(void)
{
    int y, count;
    count = 0;
    for(y = 0; y < HEIGHT; y++) {
        if(checkLine(y) == 0) {         // 揃ったラインがあるか確認
            deleteLine(y);              // 一行削除
            count++;
        }
    }
    return count;
}

void printNext(int type)
{
    int i, j;
    Cell a = {' ', BLACK, NORMAL};
    setPosition(0, 0);      // (0, 0) に文字表示
    setAttribute(NORMAL);
    setBackColor(WHITE);
    setCharColor(BLACK);
    printf("NEXT");
    for (j = 0; j < BLOCK_SIZE; j++) {
        for (i =0; i < BLOCK_SIZE; i++) {
            printCell(a, 5+i, 0+j);         // 空のセルを表示して画面消去
        }
    }
    printBlock(block_type[type], 5, 0);     // 座標(5, 0)にブロック表示
}
void printScore(int score)
{
    setPosition(WIDTH,10);
    setAttribute(NORMAL);
    setBackColor(WHITE);
    setCharColor(BLACK);
    printf("Score: %d", score);
}