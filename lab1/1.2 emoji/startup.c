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


void kbd_activate( unsigned int row )
{
    switch( row )
    {
        case 1: *GPIO_ODR_HIGH = 0x10 ;  break;
        case 2: *GPIO_ODR_HIGH = 0x20 ;  break;
        case 3: *GPIO_ODR_HIGH = 0x40 ;  break;
        case 4: *GPIO_ODR_HIGH = 0x80 ;  break;
        case 0: *GPIO_ODR_HIGH = 0x00 ;  break;
    }
}

int kbd_get_col ( void )
{
    unsigned char c;
    c = *GPIO_IDR_HIGH;
    
    if ( c & 0x8 )      return 4;
    if ( c & 0x4 )      return 3;
    if ( c & 0x2 )      return 2;
    if ( c & 0x1 )      return 1;
    
    return 0;
}




unsigned char keyb(void) 
{
    unsigned char key[]={1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
    int row, col;
    
    for (row = 1; row <=4 ; row++ ) {
        kbd_activate( row );    
        if( (col = kbd_get_col () ) )
        {
            return key[ 4*(row - 1) + (col - 1) ];
        }
    }
    
    *GPIO_ODR_HIGH = 0;
    return 0xFF;
}


void out_emoji( unsigned char c )
{
    char seg_code[]=
        { 0x40, 0x62, 0x54, 0x49, 0x5d, 0x6b, 0x64 };
    
    
    if( c > 5){
        *GPIO_ODR_LOW = seg_code[6];
        return;
    }

    *GPIO_ODR_LOW = seg_code[c];
}


void init_app( void )
{

    *GPIO_MODER = 0x55005555;
    *GPIO_PUPDR = 0x00AA0000;

}


void main(void) {
    char c;
    init_app();

    while (1) {
        c = keyb();
        out_emoji(c);
    }
}