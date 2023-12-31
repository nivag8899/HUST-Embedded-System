#include "../common/common.h"
#include "stdbool.h"
#include "string.h"

#define RED FB_COLOR(255, 0, 0)
#define ORANGE FB_COLOR(255, 165, 0)
#define YELLOW FB_COLOR(255, 255, 0)
#define GREEN FB_COLOR(0, 255, 0)
#define CYAN FB_COLOR(0, 127, 255)
#define BLUE FB_COLOR(0, 0, 255)
#define PURPLE FB_COLOR(139, 0, 255)
#define BLACK FB_COLOR(0, 0, 0)
#define WHITE FB_COLOR(255,255,255)

#define BACKGROUND_COLOR FB_COLOR(250,250,210)
#define GRID_SIZE 40
#define LINE_COLOR BLACK
#define WHITE_SIZE 80 //留白
#define BORDER 2
#define R 10

int board[15][15]={0}; // 棋盘状态,0为空,-1为黑棋,1为白棋
int pointsx[110]={0}; // 棋盘坐标
int pointsy[110]={0};// 棋盘纵坐标
int current_player = 1; // 当前棋手(黑棋先手)
int grid_count;//每一列最多能下的棋子个数
static int touch_fd;
static int t[64];

int xx, yy;//需要画的棋盘交点
int ii,jj; //棋盘交点逻辑位置

int black_wins = 0;
int white_wins = 0;
int historyFlag = 0;
int isWin = 0;


typedef struct{
    int x;
} position;

//蓝牙连接队友的板子
static position op_board, * op_p_board;
//蓝牙fd
static int bluetooth_fd1;
static int bluetooth_fd2;

void draw_chessboard(void);
void draw_menu(void);

void start_game(){
    fb_init("/dev/fb0");
    font_init("./font.ttc");
    //清屏
    isWin = 0;
    fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
    fb_update();
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
            board[i][j] = 0;
    
    current_player = 1;
    draw_chessboard();
    draw_menu();
}


// 绘制棋盘
void draw_chessboard() {
    //屏幕高度600 
    grid_count = (SCREEN_HEIGHT - 2 * WHITE_SIZE)/ GRID_SIZE;
    int width_count = (SCREEN_WIDTH - grid_count * GRID_SIZE) / 2;//宽度留白
    int w1 = SCREEN_HEIGHT - WHITE_SIZE;
    int w2 = SCREEN_WIDTH - width_count;

    // 绘制棋盘线条
    int x=50;  
    int y=40;
    for (int i = 0; i < grid_count + 1 ; i++) {
        x += GRID_SIZE;
        fb_draw_line(x, WHITE_SIZE, x, w1, LINE_COLOR);
        //printf("%d\n",x);
        //fb_draw_line(width_count, x, w2, x, LINE_COLOR);
        pointsx[i]= x;
        pointsy[i] = x + width_count;
    }

    for (int i = 0; i < grid_count + 1; i++) {
        y += GRID_SIZE;
        //fb_draw_line(x, WHITE_SIZE, x, w1, LINE_COLOR);
        fb_draw_line(90, y, 90 + w1 - WHITE_SIZE, y, LINE_COLOR);
        pointsy[i] = y;
    }
    fb_update();
}


//绘制菜单
void draw_menu(){
    fb_draw_text(600, 100, "Welcome to the Gomoku game!", 24, BLACK);

    // 绘制重新开始按钮
    int button1_x = 600;
    int button1_y = 150;
    int button1_width = 150;
    int button1_height = 40;
    fb_draw_rect(button1_x, button1_y, button1_width, button1_height, CYAN);
    fb_draw_text(button1_x + 35, button1_y + 30, "Restart", 24, BLACK);

    //绘制历史记录按钮
    int button2_x = 600;
    int button2_y = 230;
    int button2_width = 150;
    int button2_height = 40;
    fb_draw_rect(button2_x, button2_y, button2_width, button2_height, CYAN);
    fb_draw_text(button2_x + 35, button2_y + 30, "History", 24, BLACK);

    fb_update();
}

void handle_menu_click(int x, int y)
{
    int button1_x = 600;
    int button1_y = 150;
    int button1_width = 150;
    int button1_height = 40;

    if (x >= button1_x && x <= button1_x + button1_width &&
        y >= button1_y && y <= button1_y + button1_height)
    {
        // 重新开始游戏
        
        t[3] = 1;
        myWrite_nonblock(bluetooth_fd2, t, 4 * sizeof(int));
        start_game();
        t[3] = 0;

    }

    int button2_x = 600;
    int button2_y = 230;
    int button2_width = 150;
    int button2_height = 40;
    char str[5];
    
    if (x >= button2_x && x <= button2_x + button2_width &&
        y >= button2_y && y <= button2_y + button2_height)
    {
        if(historyFlag == 0){
            fb_draw_text(600, 350, "Black Wins: ", 24, BLACK);
            fb_draw_text(600, 400, "White Wins: ", 24, BLACK);
            memset(str,'\0',5);
            sprintf(str,"%d",black_wins);
            fb_draw_text(750, 350, str, 24, BLACK);
            memset(str,'\0',5);
            sprintf(str,"%d",white_wins);
            fb_draw_text(750, 400, str, 24, BLACK);
            fb_update();
            historyFlag = 1;
        }else if(historyFlag ==1){
            fb_draw_rect(600, 300, 600, 500 , BACKGROUND_COLOR);
            fb_update();
            historyFlag = 0;
        }
        
    }
}

// 在交点附近落子
void play(int x, int y) {
    int color;
    if (current_player == 1) {
        // 黑棋
        color = BLACK;
        fb_draw_circle(x, y, R, color);
        fb_update();
    }
    else {
        // 白棋
        color = WHITE;
        // 首先绘制较大的圆，将成为边框的一部分
        fb_draw_circle(x, y, R, BLACK);
        // 然后绘制稍小的圆，作为填充部分
        fb_draw_circle(x, y, R - BORDER, color);
        fb_update();
    }
    // 切换棋手

    current_player = -current_player;
}

//判断胜利
int check_winner(int x, int y) {
    int player = board[x][y];
    int end = 0;
    printf("current i:%d j:%d  player:%d\n",x,y,player);

    // 获取刚刚下子的玩家
    int count;

    // 检查水平方向
    count = 1;
    for (int i = x - 1; i >= 0 && board[i][y] == player; i--) count++;  // 检查左边
    for (int i = x + 1; i < grid_count && board[i][y] == player; i++) count++;  // 检查右边
    if (count >= 5) end = player;

    // 检查垂直方向
    count = 1;
    for (int j = y - 1; j >= 0 && board[x][j] == player; j--) count++;  // 检查上面
    for (int j = y + 1; j < grid_count && board[x][j] == player; j++) count++;  // 检查下面
    if (count >= 5) end = player;

    // 检查对角线（从左上到右下）
    count = 1;
    for (int i = x - 1, j = y - 1; i >= 0 && j >= 0 && board[i][j] == player; i--, j--) count++;  // 检查左上
    for (int i = x + 1, j = y + 1; i < grid_count && j < grid_count && board[i][j] == player; i++, j++) count++;  // 检查右下
    if (count >= 5) end = player;

    // 检查对角线（从右上到左下）
    count = 1;
    for (int i = x - 1, j = y + 1; i >= 0 && j < grid_count && board[i][j] == player; i--, j++) count++;  // 检查右上
    for (int i = x + 1, j = y - 1; i < grid_count && j >= 0 && board[i][j] == player; i++, j--) count++;  // 检查左下
    if (count >= 5) end = player;

    if (end == -1) {
        fb_draw_rect(0, 0, 550, SCREEN_HEIGHT, BACKGROUND_COLOR);
        fb_draw_text(80, SCREEN_HEIGHT / 2, "Black side wins!", 24, BLACK);//在屏幕中间偏左的地方写字
        black_wins ++;
        isWin = 1; 
        printf("Black Wins!\n");
        fb_update();
    }
    else if (end == 1) {
        fb_draw_rect(0, 0, 550, SCREEN_HEIGHT, BACKGROUND_COLOR);
        fb_draw_text(80, SCREEN_HEIGHT / 2, "White side wins!", 24, BLACK);//在屏幕中间偏左的地方写字
        white_wins ++;
        isWin = 1;
        printf("White Wins!\n");
        fb_update();
    }
    return end;  // 返回判断结果
}

//落子合法性
bool check_valid(int x,int y){
    bool valid = false;
    if(isWin == 1) return valid;
    for (int i = 0; i < grid_count + 1; i++) {//只有当触摸点在交点20像素范围内,才认为合法落子
        for (int j = 0; j < grid_count + 1; j++) {
            int dx = abs(x - pointsx[i]);
            int dy = abs(y - pointsy[j]);
            if (dx < 20 && dy < 20) {
                valid = true;
                xx = pointsx[i];
                yy = pointsy[j];
                ii = i;
                jj = j;
                break;
            }
        }
    }
    return valid;
}

static void touch_event_cb(int fd){
    int x, y, finger, event;
    event = touch_read(touch_fd, &x, &y, &finger);
    if (event == TOUCH_PRESS) {
        printf("TOUCH_PRESS: x=%d, y=%d, finger=%d\n", x, y, finger);
        handle_menu_click(x, y);

        if (check_valid(x,y)){
            // 检查是否有子
            if (board[ii][jj] == 0) {
                // 落子
                play(xx, yy);
                board[ii][jj] = current_player;
                t[0] = check_winner(ii, jj);
                t[1] = xx;
                t[2] = yy;
                myWrite_nonblock(bluetooth_fd2, t, 3 * sizeof(int));
            }
        }
            
    }
}

//蓝牙事件
static void bluetooth_tty_event_cb(int fd)
{
    int t[128];
    int n;

    n = myRead_nonblock(fd, t, 4 * sizeof(int));
    if (n <= 0) {
        printf("close bluetooth tty fd\n");
        exit(0);
        return;
    }
    else {
        //更新对面的棋
        play(t[1], t[2]);
        //看有没有赢
        if (t[0] == -1) {
            fb_draw_rect(0, 0, 550, SCREEN_HEIGHT, BACKGROUND_COLOR);
            fb_draw_text(80, SCREEN_HEIGHT / 2, "Black side wins!", 24, BLACK);//在屏幕中间偏左的地方写字
            black_wins ++;
            isWin = 1; 
            printf("Black Wins!\n");
            fb_update();
        }
        else if (t[0] == 1) {
            fb_draw_rect(0, 0, 550, SCREEN_HEIGHT, BACKGROUND_COLOR);
            fb_draw_text(80, SCREEN_HEIGHT / 2, "White side wins!", 24, BLACK);//在屏幕中间偏左的地方写字
            white_wins ++;
            isWin = 1;
            printf("White Wins!\n");
            fb_update();
        }
        if(t[3] == 1) start_game();
    }
    return;
}

//打开蓝牙文件
static int bluetooth_tty_init(const char* dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK); /*非阻塞模式*/
    if (fd < 0) {
        printf("bluetooth_tty_init open %s error(%d): %s\n", dev, errno, strerror(errno));
        return -1;
    }
    return fd;
}

int main() {

    start_game();
    // 打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event2");
    task_add_file(touch_fd, touch_event_cb);

    //蓝牙

    
    bluetooth_fd1 = bluetooth_tty_init("/dev/rfcomm0");
    if (bluetooth_fd1 == -1) return 0;
    task_add_file(bluetooth_fd1, bluetooth_tty_event_cb);
    //写，向服务端写入信息
    bluetooth_fd2 = bluetooth_tty_init("/dev/rfcomm1");
    if (bluetooth_fd2 == -1) return 0; 
    
    
    task_loop();
    return 0;
}
