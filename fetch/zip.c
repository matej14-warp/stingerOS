







static void memcpy_z(void*d,const void*s,size_t n){
    uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;
}






typedef struct {
    uint32_t sig;
    uint16_t version;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t fname_len;
    uint16_t extra_len;
} __attribute__((packed)) zip_lfh_t;

static uint32_t read32le(const uint8_t *p){
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static uint16_t read16le(const uint8_t *p){
    return (uint16_t)p[0]|(uint16_t)p[1]<<8;
}


int zip_extract(const uint8_t *data, size_t len,
                zip_extract_cb cb, void *userdata) {
    if(!data||!cb) return -1;
    size_t pos=0;
    int count=0;

    while(pos+4 <= len) {
        uint32_t sig=read32le(data+pos);

        if(sig==ZIP_CD_SIG||sig==ZIP_EOCD_SIG) break;

        if(sig!=ZIP_LFH_SIG){

            pos++; continue;
        }

        if(pos+sizeof(zip_lfh_t)>len) break;
        const zip_lfh_t *lfh=(const zip_lfh_t*)(data+pos);
        size_t fname_len=read16le((uint8_t*)&lfh->fname_len);
        size_t extra_len=read16le((uint8_t*)&lfh->extra_len);
        uint32_t comp_size=read32le((uint8_t*)&lfh->compressed_size);
        uint32_t uncomp_size=read32le((uint8_t*)&lfh->uncompressed_size);
        uint16_t compression=read16le((uint8_t*)&lfh->compression);

        size_t hdr_end=pos+sizeof(zip_lfh_t)+fname_len+extra_len;
        if(hdr_end+comp_size>len) break;


        char fname[256];
        size_t fnlen=fname_len<255?fname_len:255;
        memcpy_z(fname,data+pos+sizeof(zip_lfh_t),fnlen);
        fname[fnlen]=0;

        const uint8_t *file_data=data+hdr_end;

        if(compression==0) {

            cb(fname,file_data,uncomp_size,userdata);
            count++;
        } else if(compression==8) {

            uint8_t *out=(uint8_t*)kmalloc(uncomp_size+1);
            if(out){
                int r=zip_inflate(file_data,comp_size,out,uncomp_size);
                if(r==0) cb(fname,out,uncomp_size,userdata);
                else terminal_printf("[zip] inflate failed for %s\n",fname);
                kfree(out);
                count++;
            }
        } else {
            terminal_printf("[zip] unsupported compression %d for %s\n",
                            compression, fname);
        }

        pos=hdr_end+comp_size;


        if(pos+4<=len && read32le(data+pos)==ZIP_DATA_SIG) pos+=16;
    }
    return count;
}


int zip_inflate(const uint8_t *in, size_t in_len,
                uint8_t *out, size_t out_len) {
    size_t ipos=0, opos=0;
    uint32_t bits=0; int nbits=0;


        if(nbits==0){ if(ipos>=in_len) return -1; bits=in[ipos++]; nbits=8; } \
        uint32_t b=bits&1; bits>>=1; nbits--; b; })

        uint32_t v=0; \
        for(int _i=0;_i<(n);_i++) v|=GETBIT()<<_i; \
        v; })

    while(1){
        uint32_t bfinal=GETBIT();
        uint32_t btype =GETBITS(2);

        if(btype==0){

            nbits=0; bits=0;
            if(ipos+4>in_len) return -1;
            uint16_t slen=(uint16_t)(in[ipos]|(in[ipos+1]<<8)); ipos+=2;
            ipos+=2;
            if(opos+slen>out_len) return -1;
            memcpy_z(out+opos,in+ipos,slen);
            opos+=slen; ipos+=slen;
        } else if(btype==1){
            while(1){

                uint32_t code=0;
                int codelen=7;
                code=GETBITS(7);

                uint32_t rc=0;
                for(int i=0;i<7;i++) rc=(rc<<1)|((code>>i)&1);

                uint32_t sym;
                if(rc<=0x17){ sym=256+rc; codelen=7; }
                else if(rc<=0x5F){
                    code=GETBIT(); rc=(rc<<1)|code; codelen=8;
                    sym=rc-0x30;
                    if(sym>143){ sym=rc-0x190+280; }
                } else { codelen=8; sym=0; }

                (void)codelen;

                if(sym==256) break;
                if(sym<256){
                    if(opos>=out_len) return -1;
                    out[opos++]=(uint8_t)sym;
                } else {

                    uint32_t length=sym-257+3;
                    uint32_t dist_code=GETBITS(5);
                    uint32_t dist=dist_code+1;
                    if(opos+length>out_len) return -1;
                    for(uint32_t i=0;i<length;i++){
                        out[opos]=out[opos-dist];
                        opos++;
                    }
                }
            }
        } else {
            return -1;
        }

        if(bfinal) break;
    }
    return 0;


}




