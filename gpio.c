#include <stdio.h>
#include <errno.h>
#include <string.h>

int gpio_get(int gpio){
	FILE * fd;
	if(gpio<0)return -EINVAL;
	char path[32]="/sys/class/gpio/gpio";
	sprintf(path+strlen(path),"%d/value",gpio);
	fd=fopen(path,"r");
	if(fd<0)return -ENODEV;
	path[0]=fgetc(fd);
	fclose(fd);
	return path[0]=='0'?0:1;					
}

int gpio_set(int gpio, int val){
        FILE * fd;
        if(gpio<0)return -EINVAL;
        char path[32]="/sys/class/gpio/gpio";
        sprintf(path+strlen(path),"%d/value",gpio);
        fd=fopen(path,"w");
        if(fd<0)return -ENODEV;
        fputc(val?'1':'0',fd);
        fclose(fd);
        return 0;
}

int gpio_open(int gpio, int val){ //set to output if val>=0; val=0 set low; else set high; val<0 set to input
	FILE * fd;
	char num[8];
	char path[40]="/sys/class/gpio/gpio";
	if(gpio<0)return -EINVAL;
	fd=fopen("/sys/class/gpio/export","w");
	if(fd<0)return -ENODEV;
	sprintf(num,"%d",gpio);
	fputs(num,fd);
	fclose(fd);
	strcat(path,num);
	strcat(path,"/direction");
	fd=fopen(path,"r");
	if(fd<0)return -ENODEV;
	fclose(fd);
	fd=fopen(path,"w");
	if(fd<0)return -ENODEV;
	if(val<0)fputs("in",fd);
	else fputs("out",fd);
	fclose(fd);
	if(val>=0)gpio_set(gpio,val);
	return 0;
}

int gpio_close(int gpio){
	FILE * fd;
	if(gpio<0)return -EINVAL;
	fd=fopen("/sys/class/gpio/unexport","w");
	if(fd<0)return -ENODEV;
	fprintf(fd,"%d",gpio);
	fclose(fd);
	return 0;	
}
