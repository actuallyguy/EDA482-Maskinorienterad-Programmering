/*
 *  startup.c
 *
 */
__attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");      /* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");                 /* call main */
__asm__ volatile(".L1: B .L1\n");               /* never return */
}

#define GPIO_D          0x40020C00
#define GPIO_MODER      ((volatile unsigned int *) (GPIO_D))
#define GPIO_OTYPER     ((volatile unsigned short *) (GPIO_D+0x4))
#define GPIO_PUPDR      ((volatile unsigned int *) (GPIO_D+0xC))
#define GPIO_IDR_LOW    ((volatile unsigned char *) (GPIO_D+0x10))
#define GPIO_IDR_HIGH   ((volatile unsigned char *) (GPIO_D+0x11))
#define GPIO_ODR_LOW    ((volatile unsigned char *) (GPIO_D+0x14))
#define GPIO_ODR_HIGH   ((volatile unsigned char *) (GPIO_D+0x15)) 

#define SYSTICK     0xE000E010
#define STK_CTRL    ((volatile unsigned int *) (SYSTICK))  
#define STK_LOAD    ((volatile unsigned int *) (SYSTICK + 4))  
#define STK_VAL     ((volatile unsigned int *) (SYSTICK + 8))  

#define BARGRAPH GPIO_ODR_LOW

#define SIMULATOR

void delay_250ns()
{
    *STK_CTRL = 0;
    *STK_LOAD = (168/4 - 1); // System clock = 168 MHz
    *STK_VAL = 0;
    *STK_CTRL = 5;
    while((*STK_CTRL & 0x10000) == 0);
    *STK_CTRL = 0;
}


void delay_micro(unsigned int us)
{
    while(us)
    {
        delay_250ns();
        delay_250ns();
        delay_250ns();
        delay_250ns();
        us--;
    }
}


void delay_milli(unsigned int ms)
{
#ifdef SIMULATOR
    ms /= 1000;
    ms++;
#endif
    while(ms)
    {
        delay_micro(1000);
        ms--;
    }
}


void init_app()
{
    *GPIO_MODER = 0x5555;
    *GPIO_OTYPER = 0;
}


void main()
{
    init_app();
    while(1)
    {
        *BARGRAPH = 0;
        delay_milli(500);
        *BARGRAPH = 0xFF;
        delay_milli(500);
    }
}
