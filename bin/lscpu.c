#include "_bin_common.h"
extern int g_cpu_count;
void bin_lscpu(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    uint32_t eax,ebx,ecx,edx;char vendor[13];vendor[12]=0;
    __asm__ volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0));
    ((uint32_t*)vendor)[0]=ebx;((uint32_t*)vendor)[1]=edx;((uint32_t*)vendor)[2]=ecx;
    __asm__ volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(1));
    terminal_printf("Architecture: i686\nCPU(s):       %d\nVendor:       %s\nFamily:       %d\nModel:        %d\nStepping:     %d\nFlags:        %s%s%s\n",
        g_cpu_count,vendor,(eax>>8)&0xF,(eax>>4)&0xF,eax&0xF,edx&1?"fpu ":"",edx>>26&1?"sse2 ":"",ecx&1?"sse3":"");}