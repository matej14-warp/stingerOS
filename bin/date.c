
static inline uint8_t cr(uint8_t r){__asm__ volatile("outb %0,$0x70"::"a"(r));uint8_t v;__asm__ volatile("inb $0x71,%0":"=a"(v));return v;}
static int bcd(uint8_t v){return(v>>4)*10+(v&0xF);}
void bin_date(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    while(cr(0x0A)&0x80);uint8_t b=cr(0x0B);
    int sc=b&4?cr(0):bcd(cr(0)),mn=b&4?cr(2):bcd(cr(2)),hr=b&4?cr(4):bcd(cr(4)),dy=b&4?cr(7):bcd(cr(7)),mo=b&4?cr(8):bcd(cr(8)),yr=(b&4?cr(9):bcd(cr(9)))+2000;
    static const char*mon[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    terminal_printf("%s %02d %04d %02d:%02d:%02d UTC\n",mo<=12?mon[mo]:"???",dy,yr,hr,mn,sc);}



