




typedef void (*zip_extract_cb)(const char *filename,
                                const uint8_t *data, size_t len,
                                void *userdata);

int zip_extract(const uint8_t *data, size_t len,
                zip_extract_cb cb, void *userdata);

int zip_inflate(const uint8_t *in, size_t in_len,
                uint8_t *out, size_t out_len);






