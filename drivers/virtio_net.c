








static inline uint8_t  inb_v(uint16_t p){uint8_t  v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw_v(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint32_t inl_v(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb_v(uint16_t p,uint8_t  v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw_v(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline void outl_v(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static void v_memset(void*d,uint8_t c,size_t n){uint8_t*p=d;while(n--)*p++=c;}
static void v_memcpy(void*d,const void*s,size_t n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}



static uint32_t vpci_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg){
    outl_v(PCI_ADDR,0x80000000|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    return inl_v(PCI_DATA);
}
static void vpci_write(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t reg,uint32_t val){
    outl_v(PCI_ADDR,0x80000000|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(reg&0xFC));
    outl_v(PCI_DATA,val);
}
























typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} vring_desc_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} vring_avail_t;

typedef struct {
    uint32_t id;
    uint32_t len;
} vring_used_elem_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    vring_used_elem_t ring[];
} vring_used_t;


typedef struct {
    uint8_t  flags;
    uint8_t  gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
} __attribute__((packed)) virtio_net_hdr_t;





static vring_desc_t   rx_desc[VRING_RX_SIZE] __attribute__((aligned(VRING_ALIGN)));
static uint16_t       rx_avail_ring[VRING_RX_SIZE];
static uint16_t       rx_avail_flags_idx[2];
static vring_used_elem_t rx_used_ring[VRING_RX_SIZE];
static uint16_t       rx_used_flags_idx[2];
static uint8_t        rx_buf[VRING_RX_SIZE][VNET_BUF_SIZE];

static vring_desc_t   tx_desc[VRING_TX_SIZE] __attribute__((aligned(VRING_ALIGN)));
static uint16_t       tx_avail_ring[VRING_TX_SIZE];
static uint16_t       tx_avail_flags_idx[2];

static uint16_t       tx_used_flags_idx[2];
static uint8_t        tx_buf_v[VRING_TX_SIZE][VNET_BUF_SIZE+VNET_HDR_SIZE];

static uint16_t vio_base = 0;
static uint8_t  vio_mac[6];
static int      vio_ready = 0;
static uint16_t rx_last_used = 0;
static net_receive_cb_t vio_rx_cb = NULL;

static void vring_setup_rx(void){
    for(int i=0;i<VRING_RX_SIZE;i++){
        rx_desc[i].addr=(uint64_t)(uintptr_t)rx_buf[i];
        rx_desc[i].len=VNET_BUF_SIZE;
        rx_desc[i].flags=2;
        rx_desc[i].next=0;
        rx_avail_ring[i]=(uint16_t)i;
    }
    rx_avail_flags_idx[0]=0;
    rx_avail_flags_idx[1]=VRING_RX_SIZE;
    rx_used_flags_idx[0]=0;
    rx_used_flags_idx[1]=0;
}

int virtio_net_init(void){

    for(uint8_t bus=0;bus<8;bus++)
        for(uint8_t dev=0;dev<32;dev++){
            uint32_t id=vpci_read(bus,dev,0,0);
            uint16_t vid=id&0xFFFF, did=(uint16_t)(id>>16);
            if(vid!=0x1AF4) continue;
            if(did!=0x1000&&did!=0x1041) continue;

            uint32_t sub=vpci_read(bus,dev,0,0x2C);
            if((sub>>16)!=1&&did!=0x1041) continue;

            terminal_printf("[virtio-net] found at %d:%d vid=%04x did=%04x\n",bus,dev,vid,did);
            vpci_write(bus,dev,0,0x04,vpci_read(bus,dev,0,0x04)|0x07);
            uint16_t bar0=(uint16_t)(vpci_read(bus,dev,0,0x10)&~0x3);
            vio_base=bar0;


            outb_v(vio_base+VIRTIO_PCI_STATUS,0);
            outb_v(vio_base+VIRTIO_PCI_STATUS,VIRTIO_STATUS_ACKNOWLEDGE);
            outb_v(vio_base+VIRTIO_PCI_STATUS,VIRTIO_STATUS_ACKNOWLEDGE|VIRTIO_STATUS_DRIVER);


            uint32_t feat=inl_v(vio_base+VIRTIO_PCI_HOST_FEATURES);
            outl_v(vio_base+VIRTIO_PCI_GUEST_FEATURES,feat&VIRTIO_NET_F_MAC);


            for(int i=0;i<6;i++)
                vio_mac[i]=inb_v(vio_base+VIRTIO_PCI_CONFIG+i);
            terminal_printf("[virtio-net] MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                vio_mac[0],vio_mac[1],vio_mac[2],vio_mac[3],vio_mac[4],vio_mac[5]);


            vring_setup_rx();
            outw_v(vio_base+VIRTIO_PCI_QUEUE_SEL,0);
            outl_v(vio_base+VIRTIO_PCI_QUEUE_PFN,(uint32_t)((uintptr_t)rx_desc/VRING_ALIGN));


            v_memset(tx_desc,0,sizeof(tx_desc));
            tx_avail_flags_idx[0]=0; tx_avail_flags_idx[1]=0;
            tx_used_flags_idx[0]=0;  tx_used_flags_idx[1]=0;
            outw_v(vio_base+VIRTIO_PCI_QUEUE_SEL,1);
            outl_v(vio_base+VIRTIO_PCI_QUEUE_PFN,(uint32_t)((uintptr_t)tx_desc/VRING_ALIGN));

            outb_v(vio_base+VIRTIO_PCI_STATUS,
                VIRTIO_STATUS_ACKNOWLEDGE|VIRTIO_STATUS_DRIVER|VIRTIO_STATUS_DRIVER_OK);

            vio_ready=1;
            terminal_printf("[virtio-net] ready\n");
            return 0;
        }
    return -1;
}

int virtio_net_send(const uint8_t *data, size_t len){
    if(!vio_ready||len>1514) return -1;
    static uint16_t tx_idx=0;
    int di=tx_idx%VRING_TX_SIZE;
    virtio_net_hdr_t *hdr=(virtio_net_hdr_t*)tx_buf_v[di];
    v_memset(hdr,0,VNET_HDR_SIZE);
    v_memcpy(tx_buf_v[di]+VNET_HDR_SIZE,data,len);
    tx_desc[di].addr=(uint64_t)(uintptr_t)tx_buf_v[di];
    tx_desc[di].len=(uint32_t)(VNET_HDR_SIZE+len);
    tx_desc[di].flags=0;
    tx_desc[di].next=0;
    uint16_t avail_idx=tx_avail_flags_idx[1];
    tx_avail_ring[avail_idx%VRING_TX_SIZE]=(uint16_t)di;
    tx_avail_flags_idx[1]++;
    outw_v(vio_base+VIRTIO_PCI_QUEUE_NOTIFY,1);
    tx_idx++;
    for(volatile int i=0;i<50000;i++);
    return 0;
}

void virtio_net_poll(void){
    if(!vio_ready) return;
    uint16_t used_idx=rx_used_flags_idx[1];
    while(rx_last_used!=used_idx){
        int di=rx_used_ring[rx_last_used%VRING_RX_SIZE].id;
        uint32_t len=rx_used_ring[rx_last_used%VRING_RX_SIZE].len;
        if(vio_rx_cb&&len>VNET_HDR_SIZE)
            vio_rx_cb(rx_buf[di]+VNET_HDR_SIZE,len-VNET_HDR_SIZE);

        uint16_t ai=rx_avail_flags_idx[1];
        rx_avail_ring[ai%VRING_RX_SIZE]=(uint16_t)di;
        rx_avail_flags_idx[1]++;
        rx_last_used++;
    }
    outw_v(vio_base+VIRTIO_PCI_QUEUE_NOTIFY,0);
    (void)used_idx;
}

void virtio_net_get_mac(uint8_t mac[6]){ for(int i=0;i<6;i++) mac[i]=vio_mac[i]; }
void virtio_net_set_rx_cb(net_receive_cb_t cb){ vio_rx_cb=cb; }
int  virtio_net_is_ready(void){ return vio_ready; }










































static uint16_t ne_base = 0;
static uint8_t  ne_mac[6];
static int      ne_ready = 0;
static uint8_t  ne_curr = NE_RX_START;
static net_receive_cb_t ne_rx_cb = NULL;

static inline uint8_t  ne_in(uint8_t reg){ return inb_v(ne_base+reg); }
static inline void     ne_out(uint8_t reg,uint8_t v){ outb_v(ne_base+reg,v); }

static void ne_remote_read(uint16_t src, uint8_t *dst, uint16_t len){
    ne_out(NE_RBCR0,len&0xFF); ne_out(NE_RBCR1,(len>>8)&0xFF);
    ne_out(NE_RSAR0,src&0xFF); ne_out(NE_RSAR1,(src>>8)&0xFF);
    ne_out(NE_CMD,NE_CMD_PAGE0|NE_CMD_START|0x08);
    for(uint16_t i=0;i<len;i+=2){
        uint16_t w=inw_v(ne_base+NE_DATA);
        if(i<len)   dst[i]  =(uint8_t)(w&0xFF);
        if(i+1<len) dst[i+1]=(uint8_t)(w>>8);
    }
    ne_out(NE_ISR,0x40);
}

int ne2000_init(void){

    uint16_t io=0;
    for(uint8_t bus=0;bus<8&&!io;bus++)
        for(uint8_t dev=0;dev<32&&!io;dev++){
            uint32_t id=vpci_read(bus,dev,0,0);
            if(id==0x80291050||(id&0xFFFFFFFF)==0x80291043||
               ((id&0xFFFF)==0x10EC&&((id>>16)&0xFFFF)==0x8029)){
                vpci_write(bus,dev,0,0x04,vpci_read(bus,dev,0,0x04)|0x05);
                uint32_t bar=vpci_read(bus,dev,0,0x10);
                if(bar&1) io=(uint16_t)(bar&~3);
                terminal_printf("[ne2000] PCI at bus%d dev%d io=0x%x\n",bus,dev,io);
            }
        }

    if(!io){
        uint16_t probes[]={0x300,0x280,0x320,0x340,0};
        for(int i=0;probes[i];i++){
            uint16_t base=probes[i];

            outb_v(base+NE_RESET, inb_v(base+NE_RESET));
            for(volatile int j=0;j<50000;j++);

            if(!(inb_v(base+NE_ISR) & 0x80)) continue;

            outb_v(base+NE_CMD, NE_CMD_PAGE0|NE_CMD_STOP|NE_CMD_RD2);
            for(volatile int j=0;j<5000;j++);
            outb_v(base+0x0E, 0x49);
            uint8_t dcr_back = inb_v(base+0x0E);

            if(dcr_back == 0xFF || dcr_back == 0x00) continue;
            io = base;
            terminal_printf("[ne2000] ISA probe at 0x%x\n", io);
            break;
        }
    }
    if(!io) return -1;
    ne_base=io;


    ne_out(NE_CMD, NE_CMD_PAGE0|NE_CMD_STOP|NE_CMD_RD2);
    for(volatile int i=0;i<5000;i++);
    ne_out(NE_ISR,0xFF);
    ne_out(NE_DCR, 0x49);
    ne_out(NE_RBCR0,0); ne_out(NE_RBCR1,0);
    ne_out(NE_RCR,0x20);
    ne_out(NE_TCR,0x02);
    ne_out(NE_PSTART,NE_RX_START); ne_out(NE_PSTOP,NE_RX_STOP);
    ne_out(NE_BNRY,NE_RX_START);
    ne_out(NE_TPSR,NE_TX_PAGE);
    ne_out(NE_IMR,0x00);


    uint8_t prom[32];
    ne_out(NE_RBCR0,32); ne_out(NE_RBCR1,0);
    ne_out(NE_RSAR0,0);  ne_out(NE_RSAR1,0);
    ne_out(NE_CMD,NE_CMD_PAGE0|NE_CMD_START|0x08);
    for(int i=0;i<32;i++) prom[i]=inb_v(ne_base+NE_DATA);
    for(int i=0;i<6;i++) ne_mac[i]=prom[i*2];


    ne_out(NE_CMD,NE_CMD_PAGE1|NE_CMD_STOP|NE_CMD_RD2);
    for(int i=0;i<6;i++) ne_out(NE_PAR0+i,ne_mac[i]);
    for(int i=0;i<8;i++) ne_out(NE_MAR0+i,0xFF);
    ne_curr=NE_RX_START+1;
    ne_out(NE_CURR,ne_curr);

    ne_out(NE_CMD,NE_CMD_PAGE0|NE_CMD_START|NE_CMD_RD2);
    ne_out(NE_TCR,0x00);
    ne_out(NE_RCR,0x04|0x08);
    ne_out(NE_IMR,0x01);

    ne_ready=1;
    terminal_printf("[ne2000] MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        ne_mac[0],ne_mac[1],ne_mac[2],ne_mac[3],ne_mac[4],ne_mac[5]);
    terminal_printf("[ne2000] ready\n");
    return 0;
}

int ne2000_send(const uint8_t *data, size_t len){
    if(!ne_ready||len>1514) return -1;
    uint16_t dma_addr=(uint16_t)(NE_TX_PAGE*256);
    ne_out(NE_RBCR0,len&0xFF); ne_out(NE_RBCR1,(len>>8)&0xFF);
    ne_out(NE_RSAR0,dma_addr&0xFF); ne_out(NE_RSAR1,(dma_addr>>8)&0xFF);
    ne_out(NE_CMD,NE_CMD_PAGE0|NE_CMD_START|0x10);
    for(size_t i=0;i<len;i+=2){
        uint16_t w=(uint16_t)data[i]|(i+1<len?(uint16_t)data[i+1]<<8:0);
        outw_v(ne_base+NE_DATA,w);
    }
    ne_out(NE_ISR,0x40);
    ne_out(NE_TPSR,NE_TX_PAGE);
    ne_out(NE_TBCR0,len&0xFF); ne_out(NE_TBCR1,(len>>8)&0xFF);
    ne_out(NE_CMD,NE_CMD_PAGE0|NE_CMD_START|NE_CMD_TXP|NE_CMD_RD2);
    for(int i=0;i<10000;i++){if(ne_in(NE_ISR)&0x02)break;}
    ne_out(NE_ISR,0x0A);
    return 0;
}

void ne2000_poll(void){
    if(!ne_ready) return;
    ne_out(NE_CMD,NE_CMD_PAGE1|NE_CMD_START|NE_CMD_RD2);
    uint8_t curr=ne_in(NE_CURR);
    ne_out(NE_CMD,NE_CMD_PAGE0|NE_CMD_START|NE_CMD_RD2);
    uint8_t bnry=ne_in(NE_BNRY)+1;
    if(bnry>=NE_RX_STOP) bnry=NE_RX_START;
    while(bnry!=curr){
        uint16_t addr=(uint16_t)(bnry*256);
        uint8_t hdr[4]; ne_remote_read(addr,hdr,4);
        uint16_t pktlen=(uint16_t)(hdr[2]|(hdr[3]<<8));
        if(pktlen>4&&pktlen<=1520&&ne_rx_cb){
            uint8_t buf[1520];
            ne_remote_read(addr+4,buf,pktlen-4);
            ne_rx_cb(buf,pktlen-4);
        }
        bnry++; if(bnry>=NE_RX_STOP) bnry=NE_RX_START;
        ne_out(NE_BNRY,bnry>NE_RX_START?bnry-1:NE_RX_STOP-1);
    }
}

void ne2000_get_mac(uint8_t mac[6]){ for(int i=0;i<6;i++) mac[i]=ne_mac[i]; }
void ne2000_set_rx_cb(net_receive_cb_t cb){ ne_rx_cb=cb; }
int  ne2000_is_ready(void){ return ne_ready; }





