#include "_bin_common.h"
#include "../drivers/net.h"
void bin_nc(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<3){terminal_printf("usage: nc <host> <port>\n");return;}
    uint8_t ip[4];
    if(dns_resolve(argv[1],ip)!=0){
        extern int dns_resolve(const char*,uint8_t*);
        /* try dotted decimal */
        int ok=1;for(int i=0;argv[1][i];i++)if(argv[1][i]!='.'&&(argv[1][i]<'0'||argv[1][i]>'9')){ok=0;break;}
        if(!ok){terminal_printf("nc: cannot resolve %s\n",argv[1]);return;}
        int o=0;const char*p=argv[1];
        while(*p&&o<4){int n=0;while(*p>='0'&&*p<='9')n=n*10+(*p++)-'0';ip[o++]=(uint8_t)n;if(*p=='.')p++;}
    }
    tcp_socket_t*s=tcp_connect(ip,(uint16_t)b_atoi(argv[2]));
    if(!s){terminal_printf("nc: connection refused\n");return;}
    terminal_printf("connected to %s:%s -- type lines, blank line to quit\n",argv[1],argv[2]);
    char buf[512];
    while(1){
        int i=0;char c;
        while((c=keyboard_getchar())!='\n'&&i<511){terminal_putchar(c);buf[i++]=c;}
        terminal_putchar('\n');buf[i++]='\r';buf[i++]='\n';buf[i]=0;
        if(i==2)break;
        tcp_send(s,(uint8_t*)buf,(size_t)i);
        uint8_t rb[1024];int r=tcp_recv(s,rb,sizeof(rb)-1,2000);
        if(r>0){rb[r]=0;terminal_printf("%s",(char*)rb);}
    }
    tcp_close(s);}