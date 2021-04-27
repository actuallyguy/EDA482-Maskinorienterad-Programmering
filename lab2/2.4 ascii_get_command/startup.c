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
#define GPIO_D_MODER      ((volatile unsigned int *) (GPIO_D))
#define GPIO_D_OTYPER     ((volatile unsigned short *) (GPIO_D+0x4))
#define GPIO_D_PUPDR      ((volatile unsigned int *) (GPIO_D+0xC))
#define GPIO_D_IDR_LOW    ((volatile unsigned char *) (GPIO_D+0x10))
#define GPIO_D_IDR_HIGH   ((volatile unsigned char *) (GPIO_D+0x11))
#define GPIO_D_ODR_LOW    ((volatile unsigned char *) (GPIO_D+0x14))
#define GPIO_D_ODR_HIGH   ((volatile unsigned char *) (GPIO_D+0x15)) 

#define INITIAL 0
#define WAITING 1
keyb_state = INITIAL;

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
    delay_micro(1);
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
    ascii_command(0xc);
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



void kbd_activate(unsigned int row)
{
    switch(row)
    {
        case 1: *GPIO_D_ODR_HIGH = 0x10 ;  break;
        case 2: *GPIO_D_ODR_HIGH = 0x20 ;  break;
        case 3: *GPIO_D_ODR_HIGH = 0x40 ;  break;
        case 4: *GPIO_D_ODR_HIGH = 0x80 ;  break;
        case 5: *GPIO_D_ODR_HIGH = 0xF0 ;  break;
        case 0: *GPIO_D_ODR_HIGH = 0x00 ;  break;
    }
}

int kbd_get_col ()
{
    unsigned char c;
    c = *GPIO_D_IDR_HIGH;
    
    if (c & 0x8)      return 4;
    if (c & 0x4)      return 3;
    if (c & 0x2)      return 2;
    if (c & 0x1)      return 1;
    
    return 0;
}




unsigned char keyb_enhanced() 
{
    unsigned char key[]={1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
    int row, col;
    
    if(keyb_state == WAITING){
        kbd_activate(5);
        col = kbd_get_col ();
        if(col == 0){
            // no key pressed, return to initial state
            keyb_state = INITIAL;
        }
    }
    
    if(keyb_state == INITIAL){
        for (row = 1; row <=4 ; row++) {
            kbd_activate(row);    
            if((col = kbd_get_col ()))
            {
                keyb_state = WAITING;
                return key[ 4*(row - 1) + (col - 1) ];
            }
        }
    }
    
    return 0xFF;
}


void out7seg(unsigned char c)
{
    char seg_code[] = 
        { 0x3f, 0x06, 0x5b, 0x4f, 
          0x66, 0x6d, 0x7d, 0x07,
          0x7f, 0x67, 0x77, 0x7c, 
          0x39, 0x5e, 0x79, 0x71 };
    
    if(c > 15){
        *GPIO_D_ODR_LOW = 0;
        return;
    }

    *GPIO_D_ODR_LOW = seg_code[c];
}


void init_app()
{

    *GPIO_D_MODER = 0x55005555;
    *GPIO_D_PUPDR = 0x00AA0000;

    *GPIO_E_MODER = 0x55555555;
    *GPIO_E_OTYPER = 0;
}



int get_command(char* string)
{
    char c;
    int cursor_pos = 1;
    
    // TODO: check kontrollpunkt
    
    while(c != 0xD){
        c = keyb_enhanced();
        switch(c)
        {
            case 0xD: break; 
            case 0xB: 
                cursor_pos--;
                ascii_gotoxy(cursor_pos, 1);
                ascii_write_char(' ');
                ascii_gotoxy(cursor_pos, 1);
                break;
                    
            default:
            if(c < 10)
            {
                ascii_write_char(c + '0');
                string[cursor_pos - 1] = c+'0';
                cursor_pos++;
            }
        }
    }
    string[cursor_pos - 1] = 0;
    return cursor_pos;
}



int main()
{   
    char buffer[21];
    init_app();
    ascii_init();
    ascii_gotoxy(1, 1);
    while(1)
    {
        if(get_command(buffer))
        {
          return 1; // en textsträng finns i ‘buffer’
        }
    }
}
