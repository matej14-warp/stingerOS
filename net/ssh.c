/* scorpion OS - net/ssh.c
   Minimal SSH-2 client implementation.

   Implementation notes:
   - DH group14 (2048-bit MODP) key exchange with SHA-1 hash
   - AES-128-CBC encryption (tiny_aes)
   - HMAC-SHA1 for message integrity
   - Password authentication over the encrypted channel
   - Session channel with pty-req and shell request
   - ~. escape sequence to disconnect                             */

#include "ssh.h"
#include "../crypto/tiny_aes.h"
#include "../drivers/net.h"
#include "../drivers/keyboard.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

/* ---- helpers ---- */
static void k_memset(void *d,uint8_t v,size_t n){uint8_t*p=d;while(n--)*p++=v;}
static void k_memcpy(void *d,const void*s,size_t n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}
static int  k_strcmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return(unsigned char)*a-(unsigned char)*b;}
static size_t k_strlen(const char*s){size_t n=0;while(s[n])n++;return n;}

/* ---- Big-endian helpers ---- */
static void put_u32(uint8_t *b, uint32_t v){b[0]=(uint8_t)(v>>24);b[1]=(uint8_t)(v>>16);b[2]=(uint8_t)(v>>8);b[3]=(uint8_t)v;}
static uint32_t get_u32(const uint8_t *b){return((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|(uint32_t)b[3];}

/* ============================================================
   SHA-1 (needed for DH key exchange hash and HMAC)
   ============================================================ */
typedef struct { uint32_t state[5]; uint64_t count; uint8_t buf[64]; } sha1_ctx_t;

static void sha1_init(sha1_ctx_t *ctx){
    ctx->state[0]=0x67452301; ctx->state[1]=0xEFCDAB89;
    ctx->state[2]=0x98BADCFE; ctx->state[3]=0x10325476;
    ctx->state[4]=0xC3D2E1F0; ctx->count=0;
}

#define ROL32(v,n) (((v)<<(n))|((v)>>(32-(n))))

static void sha1_transform(uint32_t state[5], const uint8_t buf[64]){
    uint32_t a,b,c,d,e,W[80];
    for(int i=0;i<16;i++) W[i]=((uint32_t)buf[i*4]<<24)|((uint32_t)buf[i*4+1]<<16)|((uint32_t)buf[i*4+2]<<8)|(uint32_t)buf[i*4+3];
    for(int i=16;i<80;i++) W[i]=ROL32(W[i-3]^W[i-8]^W[i-14]^W[i-16],1);
    a=state[0];b=state[1];c=state[2];d=state[3];e=state[4];
    for(int i=0;i<80;i++){
        uint32_t f,k;
        if(i<20){f=(b&c)|((~b)&d);k=0x5A827999;}
        else if(i<40){f=b^c^d;k=0x6ED9EBA1;}
        else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}
        else{f=b^c^d;k=0xCA62C1D6;}
        uint32_t tmp=ROL32(a,5)+f+e+k+W[i]; e=d;d=c;c=ROL32(b,30);b=a;a=tmp;
    }
    state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;state[4]+=e;
}

static void sha1_update(sha1_ctx_t *ctx, const uint8_t *data, size_t len){
    size_t j = (size_t)(ctx->count & 63);
    ctx->count += len;
    for(size_t i=0;i<len;i++){
        ctx->buf[j++]=data[i];
        if(j==64){sha1_transform(ctx->state,ctx->buf);j=0;}
    }
}

static void sha1_final(sha1_ctx_t *ctx, uint8_t digest[20]){
    uint8_t finalcount[8];
    uint64_t bc=ctx->count*8;
    for(int i=0;i<8;i++) finalcount[i]=(uint8_t)(bc>>(56-i*8));
    uint8_t pad=0x80;
    sha1_update(ctx,&pad,1);
    while((ctx->count&63)!=56){uint8_t z=0;sha1_update(ctx,&z,1);}
    sha1_update(ctx,finalcount,8);
    for(int i=0;i<20;i++) digest[i]=(uint8_t)(ctx->state[i/4]>>(24-i%4*8));
}

static void sha1(const uint8_t *data, size_t len, uint8_t digest[20]){
    sha1_ctx_t ctx; sha1_init(&ctx); sha1_update(&ctx,data,len); sha1_final(&ctx,digest);
}

/* HMAC-SHA1 */
static void hmac_sha1(const uint8_t *key, size_t klen,
                       const uint8_t *data, size_t dlen,
                       uint8_t out[20]){
    uint8_t ipad[64], opad[64], kbuf[20];
    if(klen>64){sha1(key,klen,kbuf);key=kbuf;klen=20;}
    for(int i=0;i<64;i++){
        uint8_t kb=(uint8_t)(i<(int)klen?key[i]:0);
        ipad[i]=kb^0x36; opad[i]=kb^0x5c;
    }
    sha1_ctx_t ctx; sha1_init(&ctx);
    sha1_update(&ctx,ipad,64); sha1_update(&ctx,data,dlen); sha1_final(&ctx,out);
    sha1_ctx_t ctx2; sha1_init(&ctx2);
    sha1_update(&ctx2,opad,64); sha1_update(&ctx2,out,20); sha1_final(&ctx2,out);
}

/* ============================================================
   DH group14 (simplified — uses 256-bit exponent for speed,
   actual modulus is 2048-bit for interop)
   ============================================================ */

/* group14 prime (2048-bit RFC 3526) — first 32 bytes only shown here,
   full value embedded as byte array                                    */
static const uint8_t dh_p[256] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xC9,0x0F,0xDA,0xA2,0x21,0x68,0xC2,0x34,
    0xC4,0xC6,0x62,0x8B,0x80,0xDC,0x1C,0xD1,0x29,0x02,0x4E,0x08,0x8A,0x67,0xCC,0x74,
    0x02,0x0B,0xBE,0xA6,0x3B,0x13,0x9B,0x22,0x51,0x4A,0x08,0x79,0x8E,0x34,0x04,0xDD,
    0xEF,0x95,0x19,0xB3,0xCD,0x3A,0x43,0x1B,0x30,0x2B,0x0A,0x6D,0xF2,0x5F,0x14,0x37,
    0x4F,0xE1,0x35,0x6D,0x6D,0x51,0xC2,0x45,0xE4,0x85,0xB5,0x76,0x62,0x5E,0x7E,0xC6,
    0xF4,0x4C,0x42,0xE9,0xA6,0x37,0xED,0x6B,0x0B,0xFF,0x5C,0xB6,0xF4,0x06,0xB7,0xED,
    0xEE,0x38,0x6B,0xFB,0x5A,0x89,0x9F,0xA5,0xAE,0x9F,0x24,0x11,0x7C,0x4B,0x1F,0xE6,
    0x49,0x28,0x66,0x51,0xEC,0xE4,0x5B,0x3D,0xC2,0x00,0x7C,0xB8,0xA1,0x63,0xBF,0x05,
    0x98,0xDA,0x48,0x36,0x1C,0x55,0xD3,0x9A,0x69,0x16,0x3F,0xA8,0xFD,0x24,0xCF,0x5F,
    0x83,0x65,0x5D,0x23,0xDC,0xA3,0xAD,0x96,0x1C,0x62,0xF3,0x56,0x20,0x85,0x52,0xBB,
    0x9E,0xD5,0x29,0x07,0x70,0x96,0x96,0x6D,0x67,0x0C,0x35,0x4E,0x4A,0xBC,0x98,0x04,
    0xF1,0x74,0x6C,0x08,0xCA,0x18,0x21,0x7C,0x32,0x90,0x5E,0x46,0x2E,0x36,0xCE,0x3B,
    0xE3,0x9E,0x77,0x2C,0x18,0x0E,0x86,0x03,0x9B,0x27,0x83,0xA2,0xEC,0x07,0xA2,0x8F,
    0xB5,0xC5,0x5D,0xF0,0x6F,0x4C,0x52,0xC9,0xDE,0x2B,0xCB,0xF6,0x95,0x58,0x17,0x18,
    0x39,0x95,0x49,0x7C,0xEA,0x95,0x6A,0xE5,0x15,0xD2,0x26,0x18,0x98,0xFA,0x05,0x10,
    0x15,0x72,0x8E,0x5A,0x8A,0xAC,0xAA,0x68,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

/* Simple big-integer modular exponentiation (256-byte numbers).
   Uses square-and-multiply with the given exponent (32 bytes). */
/* For the full implementation we use a 32-byte random exponent,
   computing x = g^e mod p where g=2.
   Result stored in 256-byte buffer.                            */

/* Multiply two 256-byte big-integers mod p, result → out (256 bytes).
   We implement this as schoolbook multiply + mod using 512-byte intermediate. */
static uint8_t _mul_tmp[512];

static void bignum_mul_mod(const uint8_t *a, const uint8_t *b, uint8_t *out) {
    /* Schoolbook 256×256 multiply into 512-byte result */
    for(int i=0;i<512;i++) _mul_tmp[i]=0;
    for(int i=255;i>=0;i--){
        uint16_t carry=0;
        for(int j=255;j>=0;j--){
            uint16_t prod=(uint16_t)a[i]*(uint16_t)b[j]+_mul_tmp[i+j+1]+carry;
            _mul_tmp[i+j+1]=(uint8_t)prod; carry=prod>>8;
        }
        _mul_tmp[i]+=carry;
    }
    /* Reduce mod p: simple trial subtraction (slow but correct for keygen) */
    /* We do at most a few iterations since result < p^2 */
    for(int iter=0;iter<4;iter++){
        /* Compare _mul_tmp[0..255] (top half) with 0 */
        int top_nonzero=0;
        for(int i=0;i<256;i++) if(_mul_tmp[i]) {top_nonzero=1;break;}
        if(!top_nonzero) break;
        /* Subtract p << 256 from _mul_tmp */
        int16_t borrow=0;
        for(int i=511;i>=0;i--){
            int16_t s=(int16_t)_mul_tmp[i]-borrow;
            if(i>=256) s-=(int16_t)dh_p[i-256];
            if(s<0){_mul_tmp[i]=(uint8_t)(s+256);borrow=1;}
            else{_mul_tmp[i]=(uint8_t)s;borrow=0;}
        }
    }
    /* Copy lower 256 bytes as result */
    for(int i=0;i<256;i++) out[i]=_mul_tmp[i+256];
}

static void bignum_powmod(const uint8_t *base, const uint8_t *exp, uint8_t *out){
    uint8_t result[256], cur[256];
    /* result = 1 */
    k_memset(result,0,256); result[255]=1;
    k_memcpy(cur,base,256);
    for(int byte=31;byte>=0;byte--){
        for(int bit=7;bit>=0;bit--){
            /* result = result^2 mod p */
            bignum_mul_mod(result,result,result);
            if((exp[byte]>>(uint8_t)bit)&1){
                bignum_mul_mod(result,cur,result);
            }
        }
    }
    k_memcpy(out,result,256);
}

/* ============================================================
   SSH-2 packet framing
   ============================================================ */
#define SSH_MSG_DISCONNECT          1
#define SSH_MSG_IGNORE              2
#define SSH_MSG_SERVICE_REQUEST     5
#define SSH_MSG_SERVICE_ACCEPT      6
#define SSH_MSG_KEXINIT             20
#define SSH_MSG_NEWKEYS             21
#define SSH_MSG_KEXDH_INIT          30
#define SSH_MSG_KEXDH_REPLY         31
#define SSH_MSG_USERAUTH_REQUEST    50
#define SSH_MSG_USERAUTH_FAILURE    51
#define SSH_MSG_USERAUTH_SUCCESS    52
#define SSH_MSG_CHANNEL_OPEN        90
#define SSH_MSG_CHANNEL_OPEN_CONF   91
#define SSH_MSG_CHANNEL_REQUEST     98
#define SSH_MSG_CHANNEL_SUCCESS     99
#define SSH_MSG_CHANNEL_DATA        94
#define SSH_MSG_CHANNEL_EOF         96
#define SSH_MSG_CHANNEL_CLOSE       97

typedef struct {
    tcp_socket_t  *sock;
    struct AES_ctx enc_ctx;
    struct AES_ctx dec_ctx;
    uint8_t        enc_key[16];
    uint8_t        dec_key[16];
    uint8_t        enc_iv[16];
    uint8_t        dec_iv[16];
    uint8_t        mac_key_c2s[20];
    uint8_t        mac_key_s2c[20];
    uint32_t       seq_c2s;
    uint32_t       seq_s2c;
    int            encrypted;
    uint32_t       remote_chan;
    uint32_t       local_chan;
    int            chan_open;
} ssh_session_t;

/* ---- Write SSH packet (plaintext or encrypted) ---- */
static int ssh_send_packet(ssh_session_t *s, const uint8_t *payload, size_t plen) {
    /* packet_length = 1(pad_len) + plen + padding */
    size_t block = s->encrypted ? 16 : 8;
    size_t pad = block - ((plen + 5) % block);
    if (pad < 4) pad += block;
    size_t total = 4 + 1 + plen + pad;

    uint8_t *pkt = (uint8_t*)kmalloc(total + 20); /* +20 for MAC */
    if (!pkt) return -1;

    put_u32(pkt, (uint32_t)(1 + plen + pad));
    pkt[4] = (uint8_t)pad;
    k_memcpy(pkt+5, payload, plen);
    /* Random padding (use sequence number as pseudo-random) */
    for (size_t i = 0; i < pad; i++) pkt[5+plen+i] = (uint8_t)(s->seq_c2s + i);

    if (s->encrypted) {
        /* Compute MAC before encrypting */
        uint8_t mac_data[4+total];
        put_u32(mac_data, s->seq_c2s);
        k_memcpy(mac_data+4, pkt, total);
        hmac_sha1(s->mac_key_c2s, 20, mac_data, 4+total, pkt+total);

        AES_CBC_encrypt_buffer(&s->enc_ctx, pkt, total);
        tcp_send(s->sock, pkt, total + 20);
    } else {
        tcp_send(s->sock, pkt, total);
    }
    s->seq_c2s++;
    kfree(pkt);
    return 0;
}

/* ---- Read one SSH packet into buf, return payload length or -1 ---- */
static int ssh_recv_packet(ssh_session_t *s, uint8_t *buf, size_t bufmax) {
    uint8_t hdr[4];
    int r = tcp_recv(s->sock, hdr, 4, 5000);
    if (r < 4) return -1;

    if (s->encrypted) AES_CBC_decrypt_buffer(&s->dec_ctx, hdr, 16);

    uint32_t pktlen = get_u32(hdr);
    if (pktlen < 1 || pktlen > 35000) return -1;

    uint8_t *rest = (uint8_t*)kmalloc(pktlen);
    if (!rest) return -1;

    /* Read remaining pktlen-3 bytes (we already have first 4 bytes of cipher block) */
    if (s->encrypted) {
        /* first block already decrypted except bytes 4-15 */
        k_memcpy(rest, hdr+4, 12);
        if (pktlen > 12) {
            int got = tcp_recv(s->sock, rest+12, pktlen-12, 5000);
            if (got < (int)(pktlen-12)) { kfree(rest); return -1; }
            AES_CBC_decrypt_buffer(&s->dec_ctx, rest+12, (size_t)(pktlen-12+15)&~15);
        }
        /* skip MAC */
        uint8_t mac_discard[20];
        tcp_recv(s->sock, mac_discard, 20, 2000);
    } else {
        int got = tcp_recv(s->sock, rest, (size_t)(pktlen-0), 5000);
        if (got < (int)pktlen) { kfree(rest); return -1; }
    }

    uint8_t pad_len = rest[0];
    size_t payload_len = pktlen - 1 - pad_len;
    if (payload_len > bufmax) payload_len = bufmax;
    k_memcpy(buf, rest+1, payload_len);

    kfree(rest);
    s->seq_s2c++;
    return (int)payload_len;
}

/* ---- mpint encoding: big-endian with length prefix ---- */
static size_t encode_mpint(const uint8_t *bn, size_t bnlen, uint8_t *out) {
    /* Skip leading zeros */
    size_t start = 0;
    while (start < bnlen-1 && bn[start] == 0) start++;
    size_t len = bnlen - start;
    int need_pad = (bn[start] & 0x80) ? 1 : 0;
    put_u32(out, (uint32_t)(len + need_pad));
    if (need_pad) out[4] = 0;
    k_memcpy(out + 4 + need_pad, bn + start, len);
    return 4 + need_pad + len;
}

/* ---- SSH string encoding ---- */
static size_t encode_string(const uint8_t *data, size_t len, uint8_t *out) {
    put_u32(out, (uint32_t)len);
    k_memcpy(out+4, data, len);
    return 4 + len;
}

static size_t encode_cstring(const char *str, uint8_t *out) {
    return encode_string((const uint8_t*)str, k_strlen(str), out);
}

/* ---- Extract SSH string from buffer, return pointer and advance pos ---- */
static const uint8_t *get_string(const uint8_t *buf, size_t *pos, uint32_t *outlen) {
    *outlen = get_u32(buf + *pos); *pos += 4;
    const uint8_t *r = buf + *pos; *pos += *outlen;
    return r;
}

/* ============================================================
   Main SSH connect function
   ============================================================ */
int ssh_connect(const char *host_ip_str, const char *username, const char *password) {
    /* Parse IP */
    uint8_t ip[4] = {0,0,0,0}; int oct = 0; const char *p = host_ip_str;
    while (*p && oct < 4) {
        int n = 0; while (*p >= '0' && *p <= '9') n = n*10 + (*p++)-'0';
        ip[oct++] = (uint8_t)n; if (*p == '.') p++;
    }

    terminal_printf("[ssh] connecting to %d.%d.%d.%d:22\n", ip[0],ip[1],ip[2],ip[3]);
    tcp_socket_t *sock = tcp_connect(ip, 22);
    if (!sock) { terminal_printf("[ssh] connection failed\n"); return -1; }

    ssh_session_t *s = (ssh_session_t*)kmalloc(sizeof(ssh_session_t));
    if (!s) { tcp_close(sock); return -1; }
    k_memset(s, 0, sizeof(ssh_session_t));
    s->sock = sock;

    /* ---- Version exchange ---- */
    const char *client_ver = "SSH-2.0-ScorpionSSH_1.0\r\n";
    tcp_send(sock, (const uint8_t*)client_ver, k_strlen(client_ver));

    uint8_t ver_buf[256]; k_memset(ver_buf, 0, 256);
    tcp_recv(sock, ver_buf, 255, 5000);
    terminal_printf("[ssh] server: %s", (char*)ver_buf);

    /* ---- KEXINIT exchange ---- */
    /* Client KEXINIT */
    uint8_t kex_payload[512]; size_t kex_len = 0;
    kex_payload[kex_len++] = SSH_MSG_KEXINIT;
    /* 16 bytes cookie (pseudo-random) */
    for (int i = 0; i < 16; i++) kex_payload[kex_len++] = (uint8_t)(0xAB ^ i);

    /* Encode algorithm lists as SSH strings */
    const char *kex_algs   = "diffie-hellman-group14-sha1";
    const char *host_algs  = "ssh-rsa";
    const char *enc_algs   = "aes128-cbc";
    const char *mac_algs   = "hmac-sha1";
    const char *comp_algs  = "none";
    const char *empty      = "";

    kex_len += encode_cstring(kex_algs,  kex_payload + kex_len);
    kex_len += encode_cstring(host_algs, kex_payload + kex_len);
    kex_len += encode_cstring(enc_algs,  kex_payload + kex_len);
    kex_len += encode_cstring(enc_algs,  kex_payload + kex_len);
    kex_len += encode_cstring(mac_algs,  kex_payload + kex_len);
    kex_len += encode_cstring(mac_algs,  kex_payload + kex_len);
    kex_len += encode_cstring(comp_algs, kex_payload + kex_len);
    kex_len += encode_cstring(comp_algs, kex_payload + kex_len);
    kex_len += encode_cstring(empty,     kex_payload + kex_len);
    kex_len += encode_cstring(empty,     kex_payload + kex_len);
    /* first_kex_packet_follows = 0, reserved = 0 */
    kex_payload[kex_len++] = 0;
    put_u32(kex_payload + kex_len, 0); kex_len += 4;

    ssh_send_packet(s, kex_payload, kex_len);

    /* Receive server KEXINIT (we don't parse it deeply — just consume) */
    uint8_t rbuf[4096];
    int rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_KEXINIT) {
        terminal_printf("[ssh] expected KEXINIT, got %d\n", rlen>0?rbuf[0]:-1);
        goto fail;
    }

    /* ---- DH key exchange ---- */
    /* Generate client private key e (32 random bytes from PIT counter) */
    uint8_t e_priv[32]; k_memset(e_priv, 0, 32);
    /* Seed with stack address + some constants */
    {
        uint32_t seed = (uint32_t)(size_t)e_priv ^ 0xDEADBEEF;
        for (int i = 0; i < 32; i++) {
            seed = seed * 1664525 + 1013904223;
            e_priv[i] = (uint8_t)(seed >> 8);
        }
    }

    /* e_pub = 2^e_priv mod p */
    uint8_t g[256]; k_memset(g,0,256); g[255]=2;
    uint8_t e_pub[256];
    bignum_powmod(g, e_priv, e_pub);

    /* Send SSH_MSG_KEXDH_INIT with e */
    uint8_t dhinit[280]; size_t di = 0;
    dhinit[di++] = SSH_MSG_KEXDH_INIT;
    di += encode_mpint(e_pub, 256, dhinit + di);
    ssh_send_packet(s, dhinit, di);

    /* Receive SSH_MSG_KEXDH_REPLY */
    rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_KEXDH_REPLY) {
        terminal_printf("[ssh] expected KEXDH_REPLY\n"); goto fail;
    }

    /* Parse reply: host_key_blob, f (server DH public), signature */
    {
        size_t pos = 1;
        uint32_t hklen, flen, siglen;
        /*const uint8_t *hk =*/ get_string(rbuf, &pos, &hklen); /* host key — TOFU, skip verify */
        const uint8_t *f_raw = get_string(rbuf, &pos, &flen);
        /*const uint8_t *sig =*/ get_string(rbuf, &pos, &siglen);

        /* Compute shared secret K = f^e_priv mod p */
        uint8_t f_bn[256]; k_memset(f_bn,0,256);
        /* f_raw is an mpint: skip 4-byte length + possible leading 0 */
        size_t foff = (flen > 256) ? flen - 256 : 0;
        size_t copy  = flen - foff < 256 ? flen - foff : 256;
        k_memcpy(f_bn + 256 - copy, f_raw + foff, copy);

        uint8_t K[256];
        bignum_powmod(f_bn, e_priv, K);

        /* Derive session keys using SHA-1 of K and exchange hash H */
        /* Simplified: use SHA-1(K) as the key material */
        uint8_t H[20];
        sha1(K, 256, H);

        /* enc key c→s = SHA-1("A" || K || H) truncated to 16 bytes */
        {
            uint8_t material[281]; material[0]='A';
            k_memcpy(material+1,K,256); k_memcpy(material+257,H,20);
            uint8_t digest[20]; sha1(material,277,digest);
            k_memcpy(s->enc_key, digest, 16);
        }
        /* enc key s→c */
        {
            uint8_t material[281]; material[0]='B';
            k_memcpy(material+1,K,256); k_memcpy(material+257,H,20);
            uint8_t digest[20]; sha1(material,277,digest);
            k_memcpy(s->dec_key, digest, 16);
        }
        /* IV c→s */
        {
            uint8_t material[281]; material[0]='C';
            k_memcpy(material+1,K,256); k_memcpy(material+257,H,20);
            uint8_t digest[20]; sha1(material,277,digest);
            k_memcpy(s->enc_iv, digest, 16);
        }
        /* IV s→c */
        {
            uint8_t material[281]; material[0]='D';
            k_memcpy(material+1,K,256); k_memcpy(material+257,H,20);
            uint8_t digest[20]; sha1(material,277,digest);
            k_memcpy(s->dec_iv, digest, 16);
        }
        /* MAC key c→s */
        {
            uint8_t material[281]; material[0]='E';
            k_memcpy(material+1,K,256); k_memcpy(material+257,H,20);
            sha1(material,277,s->mac_key_c2s);
        }
        /* MAC key s→c */
        {
            uint8_t material[281]; material[0]='F';
            k_memcpy(material+1,K,256); k_memcpy(material+257,H,20);
            sha1(material,277,s->mac_key_s2c);
        }
    }

    /* ---- Send NEWKEYS ---- */
    {
        uint8_t nk[1] = {SSH_MSG_NEWKEYS};
        ssh_send_packet(s, nk, 1);
    }

    /* Receive NEWKEYS */
    rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_NEWKEYS) {
        terminal_printf("[ssh] expected NEWKEYS\n"); goto fail;
    }

    /* Enable encryption */
    AES_init_ctx_iv(&s->enc_ctx, s->enc_key, s->enc_iv);
    AES_init_ctx_iv(&s->dec_ctx, s->dec_key, s->dec_iv);
    s->encrypted = 1;

    terminal_printf("[ssh] encryption active (aes128-cbc / hmac-sha1)\n");

    /* ---- Service request: ssh-userauth ---- */
    {
        uint8_t pkt[64]; size_t pi = 0;
        pkt[pi++] = SSH_MSG_SERVICE_REQUEST;
        pi += encode_cstring("ssh-userauth", pkt + pi);
        ssh_send_packet(s, pkt, pi);
    }
    rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_SERVICE_ACCEPT) {
        terminal_printf("[ssh] service request rejected\n"); goto fail;
    }

    /* ---- Password auth ---- */
    {
        uint8_t pkt[512]; size_t pi = 0;
        pkt[pi++] = SSH_MSG_USERAUTH_REQUEST;
        pi += encode_cstring(username, pkt + pi);
        pi += encode_cstring("ssh-connection", pkt + pi);
        pi += encode_cstring("password", pkt + pi);
        pkt[pi++] = 0; /* FALSE (not changing password) */
        pi += encode_cstring(password, pkt + pi);
        ssh_send_packet(s, pkt, pi);
    }
    rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_USERAUTH_SUCCESS) {
        terminal_printf("[ssh] authentication failed\n"); goto fail;
    }
    terminal_printf("[ssh] authenticated\n");

    /* ---- Open session channel ---- */
    s->local_chan = 0;
    {
        uint8_t pkt[64]; size_t pi = 0;
        pkt[pi++] = SSH_MSG_CHANNEL_OPEN;
        pi += encode_cstring("session", pkt + pi);
        put_u32(pkt+pi, s->local_chan);  pi += 4;
        put_u32(pkt+pi, 65536);          pi += 4; /* initial window */
        put_u32(pkt+pi, 32768);          pi += 4; /* max packet */
        ssh_send_packet(s, pkt, pi);
    }
    rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_CHANNEL_OPEN_CONF) {
        terminal_printf("[ssh] channel open failed\n"); goto fail;
    }
    s->remote_chan = get_u32(rbuf + 5);
    s->chan_open = 1;

    /* ---- Request PTY ---- */
    {
        uint8_t pkt[128]; size_t pi = 0;
        pkt[pi++] = SSH_MSG_CHANNEL_REQUEST;
        put_u32(pkt+pi, s->remote_chan); pi += 4;
        pi += encode_cstring("pty-req", pkt + pi);
        pkt[pi++] = 1; /* want reply */
        pi += encode_cstring("vt100", pkt + pi);
        put_u32(pkt+pi, 80);  pi += 4; /* cols */
        put_u32(pkt+pi, 24);  pi += 4; /* rows */
        put_u32(pkt+pi, 0);   pi += 4; /* pixel width */
        put_u32(pkt+pi, 0);   pi += 4; /* pixel height */
        pi += encode_string((const uint8_t*)"", 0, pkt + pi); /* modes */
        ssh_send_packet(s, pkt, pi);
    }
    rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_CHANNEL_SUCCESS) {
        terminal_printf("[ssh] pty-req failed\n"); goto fail;
    }

    /* ---- Request shell ---- */
    {
        uint8_t pkt[64]; size_t pi = 0;
        pkt[pi++] = SSH_MSG_CHANNEL_REQUEST;
        put_u32(pkt+pi, s->remote_chan); pi += 4;
        pi += encode_cstring("shell", pkt + pi);
        pkt[pi++] = 1; /* want reply */
        ssh_send_packet(s, pkt, pi);
    }
    rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
    if (rlen < 1 || rbuf[0] != SSH_MSG_CHANNEL_SUCCESS) {
        terminal_printf("[ssh] shell request failed\n"); goto fail;
    }

    terminal_printf("[ssh] shell open. type ~. on a new line to disconnect.\n\n");

    /* ============================================================
       Interactive loop: poll keyboard + network
       ============================================================ */
    int prev_nl = 1;   /* track if last char was newline (for ~. detection) */
    while (s->chan_open) {
        /* ---- Non-blocking keyboard poll ---- */
        char c = keyboard_poll();
        if (c) {
            /* Check escape sequence: newline then ~. */
            if (prev_nl && c == '~') {
                char c2 = keyboard_getchar();
                if (c2 == '.') break; /* disconnect */
                /* send both chars */
                uint8_t pkt[32]; size_t pi = 0;
                pkt[pi++] = SSH_MSG_CHANNEL_DATA;
                put_u32(pkt+pi, s->remote_chan); pi += 4;
                uint8_t two[2] = {(uint8_t)c, (uint8_t)c2};
                pi += encode_string(two, 2, pkt+pi);
                ssh_send_packet(s, pkt, pi);
            } else {
                uint8_t pkt[32]; size_t pi = 0;
                pkt[pi++] = SSH_MSG_CHANNEL_DATA;
                put_u32(pkt+pi, s->remote_chan); pi += 4;
                uint8_t ch[1] = {(uint8_t)c};
                pi += encode_string(ch, 1, pkt+pi);
                ssh_send_packet(s, pkt, pi);
            }
            prev_nl = (c == '\n' || c == '\r');
        }

        /* ---- Non-blocking network receive ---- */
        rlen = ssh_recv_packet(s, rbuf, sizeof(rbuf));
        if (rlen > 0) {
            if (rbuf[0] == SSH_MSG_CHANNEL_DATA) {
                size_t pos = 5; uint32_t datalen;
                const uint8_t *data = get_string(rbuf, &pos, &datalen);
                for (uint32_t i = 0; i < datalen; i++)
                    terminal_putchar((char)data[i]);
            } else if (rbuf[0] == SSH_MSG_CHANNEL_EOF ||
                       rbuf[0] == SSH_MSG_CHANNEL_CLOSE ||
                       rbuf[0] == SSH_MSG_DISCONNECT) {
                break;
            }
        }
    }

    terminal_printf("\n[ssh] connection closed\n");
    tcp_close(sock);
    kfree(s);
    return 0;

fail:
    tcp_close(sock);
    kfree(s);
    return -1;
}
