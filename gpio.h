#ifndef __GPIO_H__
#define __GPIO_H__

int gpio_open(int gpio, int val);//val < 0 for input; val>0 output high; val==0 output low
int gpio_close(int gpio);
int gpio_get(int gpio);// return < 0 for err; 0 for low; 1 for high;
int gpio_set(int gpio,int val);

static inline int gpio_num(const char * name){ // name: such as "PH18"
	return name[0]!='P'?-EINVAL: ( ( (name[1]-'A')<<5 ) + atoi(name+2) );
}

#endif
