









typedef struct {
  const uint8_t *data;
  uint32_t size;
  uint32_t pos;
  uint32_t bit_buf;
  int bit_left;


  uint8_t reservoir[4096];
  int res_len;
  int res_pos;
} mp3_decoder_t;


void mp3_init(mp3_decoder_t *dec, const uint8_t *data, uint32_t size);


int mp3_decode_frame(mp3_decoder_t *dec, int16_t *out, uint32_t *sample_rate,
                     int *channels);



