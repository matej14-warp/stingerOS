
void bin_sl(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    static const char*train[]={
        "      ====        ________                ___________       ",
        "  _D _|  |_______/        \\__I_I_____===__|_________|       ",
        "   |(_)---  |   H\\________/ |   |        =|___ ___|        ",
        "   /     |  |   H  |  |     |   |         ||_| |_||        ",
        "  |      |  |   H  |__--------------------| [___] |        ",
        "  | ________|___H__/__|_____/[][]~\\_______|       |        ",
        "  |/ |   |-----------I_____I [][] []  D   |=======|__      ",
        "__/ =| o |=-O=====O=====O=====O \\ ____Y___________|  __   ",
        " |/-=|___|=    ||    ||    ||    |_____/~\\___/         |  ",
        "  \\_/      \\__/  \\__/  \\__/  \\__/      \\_/          ",NULL};
    terminal_clear();
    for(int f=0;f<30;f++){
        terminal_printf("\x1b[H");
        for(int r=0;train[r];r++)terminal_printf("\x1b[%d;%dH%s",r+3,f+1,train[r]);
        for(volatile int i=0;i<3000000;i++);
        if(keyboard_keyready()){keyboard_poll();break;}
    }
    terminal_clear();}



