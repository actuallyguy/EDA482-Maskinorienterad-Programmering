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

#define GPIO_E          0x40021000
#define GPIO_E_MODER      ((volatile unsigned int *) (GPIO_E))
#define GPIO_E_OTYPER     ((volatile unsigned short *) (GPIO_E+0x4))
#define GPIO_E_PUPDR      ((volatile unsigned int *) (GPIO_E+0xC))
#define GPIO_E_IDR_LOW    ((volatile unsigned char *) (GPIO_E+0x10))
#define GPIO_E_IDR_HIGH   ((volatile unsigned char *) (GPIO_E+0x11))
#define GPIO_E_ODR_LOW    ((volatile unsigned char *) (GPIO_E+0x14))
#define GPIO_E_ODR_HIGH   ((volatile unsigned char *) (GPIO_E+0x15)) 

#define SYSTICK     0xE000E010
#define STK_CTRL    ((volatile unsigned int *) (SYSTICK))  
#define STK_LOAD    ((volatile unsigned int *) (SYSTICK + 4))  
#define STK_VAL     ((volatile unsigned int *) (SYSTICK + 8))  

#define B_E      0x40
#define B_SELECT 4
#define B_RW     2
#define B_RS     1

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


void ascii_ctrl_bit_set(unsigned char x)
{
    // set bit x and B_SELECT 
    *GPIO_E_ODR_LOW |= (x | B_SELECT);
}

void ascii_ctrl_bit_clear(unsigned char x)
{   
    unsigned char c;
    c = *GPIO_E_ODR_LOW;

    // clear bit x
    c &= ~x;

    // set bit B_SELECT
    c |= B_SELECT;

    *GPIO_E_ODR_LOW = c;
}

void    ascii_write_controller(unsigned char c)
{
    ascii_ctrl_bit_set(B_E);
    delay_250ns(); 
    *GPIO_E_ODR_HIGH = c;
    delay_250ns();
    ascii_ctrl_bit_clear(B_E);
    delay_250ns();
}

void    ascii_write_cmd(unsigned char c)
{
    ascii_ctrl_bit_clear(B_RS);
    ascii_ctrl_bit_clear(B_RW);
    ascii_write_controller(c);
}

void    ascii_write_data(unsigned char c)
{
    ascii_ctrl_bit_set(B_RS);
    ascii_ctrl_bit_clear(B_RW);
    ascii_write_controller(c);
}

unsigned char ascii_read_controller()
{
    
    ascii_ctrl_bit_set(B_E);
    delay_250ns(); 
    delay_250ns();  
    unsigned char rv = *GPIO_E_IDR_HIGH;
    ascii_ctrl_bit_clear(B_E);
    return rv;
}

unsigned char ascii_read_status()
{
    *GPIO_E_MODER = 0x00005555;       
    ascii_ctrl_bit_clear(B_RS);
    ascii_ctrl_bit_set(B_RW);
    unsigned char rv = ascii_read_controller();
    *GPIO_E_MODER = 0x55555555;
    return rv;
}

void ascii_command(char cmd)
{
    while((ascii_read_status() & 0x80)== 0x80);
    delay_micro(8); 
    ascii_write_cmd(cmd);
    delay_milli(1);
}

void    ascii_write_char(unsigned char c)
{
    while((ascii_read_status() & 0x80)== 0x80);
    ascii_write_data(c);
    delay_micro(39);
}

void ascii_init()
{
    ascii_command(0x38);
    ascii_command(0x0C);
    ascii_command(1);
    ascii_command(6);
}


void ascii_gotoxy(unsigned char x, unsigned char y) {
    unsigned char address = x - 1;
    if(y != 1){
        address += 0x40;
    }
    ascii_write_cmd(0x80 | address);
}


void init_app()
{
    *GPIO_E_MODER = 0x55555555;
    *GPIO_E_OTYPER = 0;
}


int main()
{
    unsigned char *s ;
    unsigned  char test1[] = "Alfanumerisk ";
    unsigned  char test2[] = "Display - test";

    init_app();
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while(*s)
        ascii_write_char(*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while(*s)
        ascii_write_char(*s++);
    return 0;
}
