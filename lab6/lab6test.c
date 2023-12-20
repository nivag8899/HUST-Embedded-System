#include "../common/common.h"
#include "stdbool.h"

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

int board[110][110]={0}; // 棋盘状态,0为空,-1为黑棋,1为白棋
int pointsx[110]={0}; // 棋盘坐标
int pointsy[110]={0};// 棋盘纵坐标
int current_player = 1; // 当前棋手(黑棋先手)
int grid_count;//每一列最多能下的棋子个数
static int touch_fd;

// 绘制棋盘
void draw_chessboard() {
    //屏幕高度600 
    grid_count = (SCREEN_HEIGHT - 2 * WHITE_SIZE)/ GRID_SIZE;
    int width_count = (SCREEN_WIDTH - grid_count * GRID_SIZE) / 2;//宽度留白
    int w1 = SCREEN_HEIGHT - WHITE_SIZE;
    int w2 = SCREEN_WIDTH - width_count;

    // 绘制棋盘线条
    //init x,y
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
    // for (int i = 0; i < grid_count + 1; i++){
    //     printf("x: %d y:%d \n",pointsx[i],pointsy[i]);
    // }
 
    // 更新显示
    fb_update();
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
// int check_winner(int x, int y) {
//     // 方向数组，分别为水平、垂直、两个对角线方向
//     int directions[4][2] = {
//         {0, 1},  // 水平方向
//         {1, 0},  // 垂直方向
//         {1, 1},  // 主对角线方向
//         {1, -1}  // 副对角线方向
//     };
//     int player = board[x][y]; // 当前落子的玩家
//     if (player == 0) {
//         // 没有棋子的位置不需要判断胜利
//         return 0;
//     }

//     // 遍历每个方向
//     for (int i = 0; i < 4; i++) {
//         int count = 1;  // 连续相同棋子的数量
//         int dx = directions[i][0];
//         int dy = directions[i][1];

//         // 检查当前位置前后的棋子
//         for (int j = 1; j < 5; j++) {
//             int newX = x + j * dx;
//             int newY = y + j * dy;
//             if (newX >= 0 && newX < grid_count && newY >= 0 && newY < grid_count && board[newX][newY] == player) {
//                 count++;
//             }
//             else {
//                 break;
//             }
//         }

//         for (int j = 1; j < 5; j++) {
//             int newX = x - j * dx;
//             int newY = y - j * dy;
//             if (newX >= 0 && newX < grid_count && newY >= 0 && newY < grid_count && board[newX][newY] == player) {
//                 count++;
//             }
//             else {
//                 break;
//             }
//         }

//         // 如果找到五个连续的同色棋子，则当前玩家胜利
//         if (count >= 5) {
//             return player;
//         }
//     }

//     // 如果没有找到胜利的玩家，则返回0
//     return 0;
// }

int check_winner(int x, int y) {
    int player = board[x][y];
    printf("current i:%d j:%d  player:%d\n",x,y,player);

    // 获取刚刚下子的玩家
    int count;

    // 检查水平方向
    count = 1;
    for (int i = x - 1; i >= 0 && board[i][y] == player; i--) count++;  // 检查左边
    for (int i = x + 1; i < grid_count && board[i][y] == player; i++) count++;  // 检查右边
    if (count >= 5) return player;

    // 检查垂直方向
    count = 1;
    for (int j = y - 1; j >= 0 && board[x][j] == player; j--) count++;  // 检查上面
    for (int j = y + 1; j < grid_count && board[x][j] == player; j++) count++;  // 检查下面
    if (count >= 5) return player;

    // 检查对角线（从左上到右下）
    count = 1;
    for (int i = x - 1, j = y - 1; i >= 0 && j >= 0 && board[i][j] == player; i--, j--) count++;  // 检查左上
    for (int i = x + 1, j = y + 1; i < grid_count && j < grid_count && board[i][j] == player; i++, j++) count++;  // 检查右下
    if (count >= 5) return player;

    // 检查对角线（从右上到左下）
    count = 1;
    for (int i = x - 1, j = y + 1; i >= 0 && j < grid_count && board[i][j] == player; i--, j++) count++;  // 检查右上
    for (int i = x + 1, j = y - 1; i < grid_count && j >= 0 && board[i][j] == player; i++, j--) count++;  // 检查左下
    if (count >= 5) return player;

    return 0;  // 没有赢家
}



static void touch_event_cb(int fd){
    int x, y, finger, event;
    event = touch_read(touch_fd, &x, &y, &finger);
    if (event == TOUCH_PRESS) {
        printf("TOUCH_PRESS: x=%d, y=%d, finger=%d\n", x, y, finger);
        // 检查合法性
        bool valid = false;
        int xx, yy;//需要画的棋盘交点
        int ii,jj; //棋盘交点逻辑位置
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
        if (valid){
            // 检查是否有子
            if (board[ii][jj] == 0) {
                // 落子
                play(xx, yy);
                board[ii][jj] = current_player;
            }
            int end = check_winner(ii, jj);
            if (end == -1) {
                fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
                fb_draw_text(80, SCREEN_HEIGHT / 2, "Black side wins!", 24, BLACK);//在屏幕中间偏左的地方写字
                printf("Black Wins!\n");
                fb_update();
            }
            else if (end == 1) {
                fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
                fb_draw_text(80, SCREEN_HEIGHT / 2, "White side wins!", 24, BLACK);//在屏幕中间偏左的地方写字
                printf("White Wins!\n");
                fb_update();
            }
        }
            
    }
}


int main() {
    fb_init("/dev/fb0");
    font_init("./font.ttc");
    //清屏
    fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
    fb_update();
    // 绘制棋盘
    draw_chessboard();
    // 打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event2");
    task_add_file(touch_fd, touch_event_cb);
    // while (1) {
        
    //     while (1) {
    //         // 读取触摸事件
    //         event = touch_read(touch_fd, &x, &y, &finger);
            
    //     }
    //     while (1) {
    //         // 不点击就不跳转下一局
    //         event = touch_read(touch_fd, &x, &y, &finger);
    //         if (event == TOUCH_PRESS) break;
    //     }
    // }
    task_loop();
    return 0;
}
