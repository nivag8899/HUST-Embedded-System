#include "common.h"
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>

static int LCD_FB_FD;
static int *LCD_FB_BUF = NULL;
static int DRAW_BUF[SCREEN_WIDTH*SCREEN_HEIGHT];

static struct area {
	int x1, x2, y1, y2;
} update_area = {0,0,0,0};

#define AREA_SET_EMPTY(pa) do {\
	(pa)->x1 = SCREEN_WIDTH;\
	(pa)->x2 = 0;\
	(pa)->y1 = SCREEN_HEIGHT;\
	(pa)->y2 = 0;\
} while(0)

void fb_init(char *dev)
{
	int fd;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;

	if(LCD_FB_BUF != NULL) return; /*already done*/

	//进入终端图形模式
	fd = open("/dev/tty0",O_RDWR,0);
	ioctl(fd, KDSETMODE, KD_GRAPHICS);
	close(fd);

	//First: Open the device
	if((fd = open(dev, O_RDWR)) < 0){
		printf("Unable to open framebuffer %s, errno = %d\n", dev, errno);
		return;
	}
	if(ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix) < 0){
		printf("Unable to FBIOGET_FSCREENINFO %s\n", dev);
		return;
	}
	if(ioctl(fd, FBIOGET_VSCREENINFO, &fb_var) < 0){
		printf("Unable to FBIOGET_VSCREENINFO %s\n", dev);
		return;
	}

	printf("framebuffer info: bits_per_pixel=%u,size=(%d,%d),virtual_pos_size=(%d,%d)(%d,%d),line_length=%u,smem_len=%u\n",
		fb_var.bits_per_pixel, fb_var.xres, fb_var.yres, fb_var.xoffset, fb_var.yoffset,
		fb_var.xres_virtual, fb_var.yres_virtual, fb_fix.line_length, fb_fix.smem_len);

	//Second: mmap
	void *addr = mmap(NULL, fb_fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == (void *)-1){
		printf("failed to mmap memory for framebuffer.\n");
		return;
	}

	if((fb_var.xoffset != 0) ||(fb_var.yoffset != 0))
	{
		fb_var.xoffset = 0;
		fb_var.yoffset = 0;
		if(ioctl(fd, FBIOPAN_DISPLAY, &fb_var) < 0) {
			printf("FBIOPAN_DISPLAY framebuffer failed\n");
		}
	}

	LCD_FB_FD = fd;
	LCD_FB_BUF = addr;

	//set empty
	AREA_SET_EMPTY(&update_area);
	return;
}

static void _copy_area(int *dst, int *src, struct area *pa)
{
	int x, y, w, h;
	x = pa->x1; w = pa->x2-x;
	y = pa->y1; h = pa->y2-y;
	src += y*SCREEN_WIDTH + x;
	dst += y*SCREEN_WIDTH + x;
	while(h-- > 0){
		memcpy(dst, src, w*4);
		src += SCREEN_WIDTH;
		dst += SCREEN_WIDTH;
	}
}

static int _check_area(struct area *pa)
{
	if(pa->x2 == 0) return 0; //is empty

	if(pa->x1 < 0) pa->x1 = 0;
	if(pa->x2 > SCREEN_WIDTH) pa->x2 = SCREEN_WIDTH;
	if(pa->y1 < 0) pa->y1 = 0;
	if(pa->y2 > SCREEN_HEIGHT) pa->y2 = SCREEN_HEIGHT;

	if((pa->x2 > pa->x1) && (pa->y2 > pa->y1))
		return 1; //no empty

	//set empty
	AREA_SET_EMPTY(pa);
	return 0;
}

void fb_update(void)
{
	if(_check_area(&update_area) == 0) return; //is empty
	_copy_area(LCD_FB_BUF, DRAW_BUF, &update_area);
	AREA_SET_EMPTY(&update_area); //set empty
	return;
}

/*======================================================================*/

static void * _begin_draw(int x, int y, int w, int h)
{
	int x2 = x+w;
	int y2 = y+h;
	if(update_area.x1 > x) update_area.x1 = x;
	if(update_area.y1 > y) update_area.y1 = y;
	if(update_area.x2 < x2) update_area.x2 = x2;
	if(update_area.y2 < y2) update_area.y2 = y2;
	return DRAW_BUF;
}

void fb_draw_pixel(int x, int y, int color)
{
	if(x<0 || y<0 || x>=SCREEN_WIDTH || y>=SCREEN_HEIGHT) return;
	int *buf = _begin_draw(x,y,1,1);
/*---------------------------------------------------*/
	*(buf + y*SCREEN_WIDTH + x) = color;
/*---------------------------------------------------*/
	return;
}

void fb_draw_rect(int x, int y, int w, int h, int color)
{
	if(x < 0) { w += x; x = 0;}
	if(x+w > SCREEN_WIDTH) { w = SCREEN_WIDTH-x;}
	if(y < 0) { h += y; y = 0;}
	if(y+h >SCREEN_HEIGHT) { h = SCREEN_HEIGHT-y;}
	if(w<=0 || h<=0) return;
	int *buf = _begin_draw(x,y,w,h);
/*---------------------------------------------------*/
	//printf("you need implement fb_draw_rect()\n"); exit(0);
	for(int j=0; j<h; j++){
		for(int i=0; i<w; i++){
			*(buf + (y+j)*SCREEN_WIDTH +x+i) = color;
		}
	}
/*---------------------------------------------------*/
	return;
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color)
{
	if(x1<0 || y1<0 || x1>=SCREEN_WIDTH || y1>=SCREEN_HEIGHT) return;
	if(x2<0 || y2<0 || x2>=SCREEN_WIDTH || y2>=SCREEN_HEIGHT) return;
	int *buf;
	//uint16_t	i = 0;
	int16_t		delta_x = 0, delta_y = 0;
	int8_t		incx = 0, incy = 0;
	uint16_t	distance = 0;
	uint16_t  t = 0;
	uint16_t	x = 0, y = 0;
	uint16_t 	x_temp = 0, y_temp = 0;


/*---------------------------------------------------*/
	//printf("you need implement fb_draw_line()\n"); exit(0);
	if(y1 == y2){
		int begin = x1<x2?x1:x2;
		int length = fabs(x1-x2);
		buf = _begin_draw(begin,y1,length,1);
		for(int i=0; i<length; i++){
			*(buf + y1*SCREEN_WIDTH + begin + i) = color;
		}
	}
	else{
		delta_x = x2 - x1;
		delta_y = y2 - y1;
		if(delta_x > 0){
			//斜线(从左到右)
			incx = 1;
		}
		else if(delta_x == 0){
			//垂直斜线(竖线)
			incx = 0;
		}
		else{
			//斜线(从右到左)
			incx = -1;
			delta_x = -delta_x;
		}
		if(delta_y > 0){
			//斜线(从左到右)
			incy = 1;
		}
		else if(delta_y == 0){
			//水平斜线(水平线)
			incy = 0;
		}
		else{
			//斜线(从右到左)
			incy = -1;
			delta_y = -delta_y;
		}

		/* 计算画笔打点距离(取两个间距中的最大值) */
		if(delta_x > delta_y){
			distance = delta_x;
		}
		else{
			distance = delta_y;
		}
		/* 开始打点 */
		x = x1;
		y = y1;

		for(t = 0; t <= distance + 1;t++)
		{
			fb_draw_pixel(x, y, color);
		
			/* 判断离实际值最近的像素点 */
			x_temp += delta_x;	
			if(x_temp > distance)
			{
				//x方向越界，减去距离值，为下一次检测做准备
				x_temp -= distance;		
				//在x方向递增打点
				x += incx;
					
			}
			y_temp += delta_y;
			if(y_temp > distance)
			{
				//y方向越界，减去距离值，为下一次检测做准备
				y_temp -= distance;
				//在y方向递增打点
				y += incy;
			}
		}
	}
/*---------------------------------------------------*/
	return;
}

// 八对称性
inline void _draw_circle_8(int xc, int yc, int x, int y, int c) {
    // 参数 c 为颜色值
	fb_draw_pixel(xc + x, yc + y, c);
	fb_draw_pixel( xc - x, yc + y, c);
	fb_draw_pixel( xc + x, yc - y, c);
	fb_draw_pixel( xc - x, yc - y, c);
	fb_draw_pixel(xc + y, yc + x, c);
	fb_draw_pixel(xc - y, yc + x, c);
	fb_draw_pixel(xc + y, yc - x, c);
	fb_draw_pixel( xc - y, yc - x, c);

}
 
//Bresenham's circle algorithm
void fb_draw_circle(int xc, int yc, int r,int c) {
    // (xc, yc) 为圆心，r 为半径
    // fill 为是否填充
    // c 为颜色值
 
    // 如果圆在图片可见区域外，直接退出
    if (xc + r < 0 || xc - r >=SCREEN_WIDTH ||
            yc + r < 0 || yc - r >=SCREEN_HEIGHT) return;
 
    int x = 0, y = r, yi, d;
    d = 3 - 2 * r;
 
    
        // 如果填充（画实心圆）
        while (x <= y) {
            for (yi = x; yi <= y; yi ++)
                _draw_circle_8(xc, yc, x, yi, c);
 
            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y --;
            }
            x++;
        }
}

void fb_draw_line_circle(int x1, int y1, int x2, int y2, int radius,int color)
{
	if(x1<0 || y1<0 || x1>=SCREEN_WIDTH || y1>=SCREEN_HEIGHT) return;
	if(x2<0 || y2<0 || x2>=SCREEN_WIDTH || y2>=SCREEN_HEIGHT) return;
	//uint16_t	i = 0;
	int16_t		delta_x = 0, delta_y = 0;
	int8_t		incx = 0, incy = 0;
	uint16_t	distance = 0;
	uint16_t  t = 0;
	uint16_t	x = 0, y = 0;
	uint16_t 	x_temp = 0, y_temp = 0;


/*---------------------------------------------------*/
	//printf("you need implement fb_draw_line()\n"); exit(0);
	if(y1 == y2){
		int begin = x1<x2?x1:x2;
		int length = fabs(x1-x2);
		for(int i=0; i<length; i++){
			fb_draw_circle( begin + i,y1*SCREEN_WIDTH,radius,color);
		}
	}
	else{
		delta_x = x2 - x1;
		delta_y = y2 - y1;
		if(delta_x > 0){
			//斜线(从左到右)
			incx = 1;
		}
		else if(delta_x == 0){
			//垂直斜线(竖线)
			incx = 0;
		}
		else{
			//斜线(从右到左)
			incx = -1;
			delta_x = -delta_x;
		}
		if(delta_y > 0){
			//斜线(从左到右)
			incy = 1;
		}
		else if(delta_y == 0){
			//水平斜线(水平线)
			incy = 0;
		}
		else{
			//斜线(从右到左)
			incy = -1;
			delta_y = -delta_y;
		}

		/* 计算画笔打点距离(取两个间距中的最大值) */
		if(delta_x > delta_y){
			distance = delta_x;
		}
		else{
			distance = delta_y;
		}
		/* 开始打点 */
		x = x1;
		y = y1;

		for(t = 0; t <= distance + 1;t++)
		{
			fb_draw_circle(x,y,radius,color);
			//fb_draw_pixel(x, y, color);
		
			/* 判断离实际值最近的像素点 */
			x_temp += delta_x;	
			if(x_temp > distance)
			{
				//x方向越界，减去距离值，为下一次检测做准备
				x_temp -= distance;		
				//在x方向递增打点
				x += incx;
					
			}
			y_temp += delta_y;
			if(y_temp > distance)
			{
				//y方向越界，减去距离值，为下一次检测做准备
				y_temp -= distance;
				//在y方向递增打点
				y += incy;
			}
		}
	}
/*---------------------------------------------------*/
	return;
}

void fb_draw_image(int x, int y, fb_image *image, int color)
{
	if(image == NULL) return;

	int ix = 0; //image x
	int iy = 0; //image y
	int w = image->pixel_w; //draw width
	int h = image->pixel_h; //draw height

	if(x<0) {w+=x; ix-=x; x=0;}
	if(y<0) {h+=y; iy-=y; y=0;}
	
	if(x+w > SCREEN_WIDTH) {
		w = SCREEN_WIDTH - x;
	}
	if(y+h > SCREEN_HEIGHT) {
		h = SCREEN_HEIGHT - y;
	}
	if((w <= 0)||(h <= 0)) return;

	int *buf = _begin_draw(x,y,w,h);
/*---------------------------------------------------------------*/
	char *dst = (char *)(buf + y*SCREEN_WIDTH + x);
	//char *src; //不同的图像颜色格式定位不同
/*---------------------------------------------------------------*/

	int alpha;
	int cur = 0;

	if(image->color_type == FB_COLOR_RGB_8880) /*lab3: jpg*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGB_8880\n"); exit(0);
		for(int j=0; j<h; j++){
			for(int i=0; i<w; i++){
				char b = image->content[cur++];
				char g = image->content[cur++];
				char r = image->content[cur++];
				cur++;
				*(buf+(y+j)*SCREEN_WIDTH + x+i) = (0xff000000|(r<<16)|(g<<8)|b);
			}
		}
		return;
	}
	else if(image->color_type == FB_COLOR_RGBA_8888) /*lab3: png*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGBA_8888\n"); exit(0);
		for(int j=0; j<h; j++){
			for(int i=0; i<w; i++){
				char b = image->content[cur++];
				char g = image->content[cur++];
				char r = image->content[cur++];
				alpha = (int)(image->content[cur++]);
				dst = (char*)(buf+(y+j)*SCREEN_WIDTH + x+i);
				//b *= alpha;
				//*(buf+(y+j)*SCREEN_WIDTH + x+i) = ((alpha<<24)|(r<<16)|(g<<8)|b)+(((1-alpha)<<24)|*(buf+(y+j)*SCREEN_WIDTH + x+i));
				//*(buf+(y+j)*SCREEN_WIDTH + x+i) = ((alpha<<24)|(r<<16)|(g<<8)|b);
				switch(alpha){
					case 0:
						break;
					case 255:
						dst[0] = b;
						dst[1] = g;
						dst[2] = r;
						break;
					default:
						dst[0] += ((b-dst[0])*alpha)>>8;
						dst[1] += ((g-dst[1])*alpha)>>8;
						dst[2] += ((r-dst[2])*alpha)>>8;
						break;
				}
			}
		}
		return;
	}
	else if(image->color_type == FB_COLOR_ALPHA_8) /*lab3: font*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_ALPHA_8\n"); exit(0);
		for(int j=0; j<h; j++){
			for(int i=0; i<w; i++){
				//cur += 1;
				alpha = (int)(image->content[(iy+j)*image->pixel_w + ix+i]);
				//alpha = (int*)(image->content[(iy+j)*w + ix+i]);
				int b = 0x000000ff & color;
				int g = (0x0000ff00 & color)>>8;
				int r = (0x00ff0000 & color)>>16;
				dst = (char *)(buf+(y+j)*SCREEN_WIDTH + x+i);

				switch(alpha){
					case 0: break;
					case 255:
						//*dst = color;
						dst[0] = b;
						dst[1] = g;
						dst[2] = r;
						dst[3] = 0xff000000 & color;
						break;
					default:
						dst[0] += ((b-dst[0])*alpha)>>8;
						dst[1] += ((g-dst[1])*alpha)>>8;
						dst[2] += ((r-dst[2])*alpha)>>8;
						dst[3] = alpha >> 24;
						//*(buf+(y+j)*SCREEN_WIDTH + x+i) = alpha<< 24 | color;
						break;
				}
				
			}
		}
		return;
	}
/*---------------------------------------------------------------*/
	return;
}

void fb_draw_border(int x, int y, int w, int h, int color)
{
	if(w<=0 || h<=0) return;
	fb_draw_rect(x, y, w, 1, color);
	if(h > 1) {
		fb_draw_rect(x, y+h-1, w, 1, color);
		fb_draw_rect(x, y+1, 1, h-2, color);
		if(w > 1) fb_draw_rect(x+w-1, y+1, 1, h-2, color);
	}
}

/** draw a text string **/
void fb_draw_text(int x, int y, char *text, int font_size, int color)
{
	fb_image *img;
	fb_font_info info;
	int i=0;
	int len = strlen(text);
	while(i < len)
	{
		img = fb_read_font_image(text+i, font_size, &info);
		if(img == NULL) break;
		fb_draw_image(x+info.left, y-info.top, img, color);
		fb_free_image(img);

		x += info.advance_x;
		i += info.bytes;
	}
	return;
}