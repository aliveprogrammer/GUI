#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <linux/input.h>
#include <stdlib.h>
#include <signal.h>
#include <linux/fb.h>
#include <sys/mman.h>

struct fb_var_screeninfo vinfo = {0};
struct input_event t;
struct draw unit;
int *ps=NULL;
FILE *flog=NULL;
struct event_solver *solver_lib;
int lnum=0;

struct draw{
    int major;
    int minor;
    int center_pos_x;
    int center_pos_y;
};

struct event_solver{
    int *set;
    void (*func)();
    int type;
    int code;
};

void draw_finger(){
    int x=unit.center_pos_x/40*9;
    int y=unit.center_pos_y/5;
    int major=unit.major/40*9;
    int minor=unit.minor/5;

    int start_x=x-minor/2;
    int start_y=y-major/2;
    if(start_x<0){

    }
    fprintf(flog,"drawlog:%d %d %d %d\n",x,y,major,minor);
    fflush(flog);
    int line_start=start_y*2160+start_x;
    for(int y=0;y<major;y++){
        for(int x=0;x<minor;x++){
            *(ps+line_start+x)=0xffffff;
        }
        line_start+=2160;
    }
}



void event_set(){
    for(int i=0;i<lnum;i++){
        if(t.type==solver_lib[i].type&&t.code==solver_lib[i].code){
            if(solver_lib[i].func)
                (*solver_lib[i].func)();
            
            fprintf(flog,"codelog:%d %d %d \n",t.type,t.code,t.value);
            if(solver_lib[i].set)
                *(solver_lib[i].set)=t.value;
            break;
        }
    }
    return;
}


int main(){
    flog=fopen("log.txt","w");
    int fscreen=open("/dev/fb0",O_RDWR);
    perror("open");
    int ftouch=open("/dev/input/event4",O_RDONLY);
    perror("open");

    struct fb_fix_screeninfo finfo = {0};
    ioctl(fscreen,FBIOGET_VSCREENINFO,&vinfo);
    ioctl(fscreen,FBIOGET_FSCREENINFO,&finfo);
    
    ps=mmap(NULL,finfo.smem_len,PROT_READ|PROT_WRITE,MAP_SHARED,fscreen,0);
    perror("mmap");
   
    sleep(1);
    printf("grab dev\n");
    ioctl(ftouch,EVIOCGRAB,1);
    perror("ioctl");  

    struct event_solver posx={&unit.center_pos_x,draw_finger,EV_ABS,ABS_MT_TOOL_X};
    struct event_solver posy={&unit.center_pos_y,NULL,EV_ABS,ABS_MT_TOOL_Y};
    struct event_solver major={&unit.major,NULL,EV_ABS,ABS_MT_TOUCH_MAJOR};
    struct event_solver minor={&unit.minor,NULL,EV_ABS,ABS_MT_TOUCH_MINOR};
    struct event_solver btn={NULL,draw_finger,EV_KEY,BTN_TOUCH};
    struct event_solver combine[]={
        posx,posy,major,minor,btn
    };
    solver_lib=combine;
    lnum=5;

    int color=0x000000;
    for(int i=0;i<(finfo.smem_len/4);i++){
        *(ps+i)=color;
    }

    while(1)
    {       
        read(ftouch,&t,sizeof(t));
        event_set();
    }

    ioctl(ftouch,EVIOCGRAB, 0);
    close(ftouch);
    munmap(ps,finfo.smem_len);
    close(fscreen);


    return 0;
}   
