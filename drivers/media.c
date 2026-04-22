















static inline uint32_t u32le(const uint8_t *b) {
  return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
         ((uint32_t)b[3] << 24);
}
static int ends_with(const char *s, const char *suf) {
  int sl = 0, xl = 0;
  for (const char *p = s; *p; p++)
    sl++;
  for (const char *p = suf; *p; p++)
    xl++;
  if (xl > sl)
    return 0;
  s += sl - xl;
  while (*suf) {
    if (*s != *suf)
      return 0;
    s++;
    suf++;
  }
  return 1;
}



static int media_play_saf_sync(sfs_node_t *file) {
    if (!file || !file->data || file->size < 16) return -1;
    const uint8_t *raw = (const uint8_t *)file->data;


    uint32_t sample_rate = u32le(raw + 8);
    uint32_t num_samples = u32le(raw + 12);


    if (sample_rate < 4000 || sample_rate > 48000) sample_rate = 8000;
    if (num_samples == 0 || num_samples > file->size - 16)
        num_samples = (uint32_t)(file->size - 16);

    const uint8_t *samples = raw + 16;

    terminal_printf("[media] saf: %u samples @ %u Hz\n", num_samples, sample_rate);

    if (hda_available()) {

        uint32_t out_frames = (num_samples * 48000u) / sample_rate;
        uint32_t chunk_frames = 32768u;
        int16_t *pcm = (int16_t*)kmalloc(chunk_frames * 2 * sizeof(int16_t));
        if (!pcm) return -1;

        uint32_t out_done = 0;
        while (out_done < out_frames) {
            uint32_t chunk = out_frames - out_done;
            if (chunk > chunk_frames) chunk = chunk_frames;

            for (uint32_t i = 0; i < chunk; i++) {
                uint32_t src_idx = ((out_done + i) * sample_rate) / 48000u;
                if (src_idx >= num_samples) src_idx = num_samples - 1;
                int16_t s = (int16_t)(((int32_t)samples[src_idx] - 128) * 256);
                pcm[i*2 + 0] = s;
                pcm[i*2 + 1] = s;
            }

            hda_play_pcm(pcm, chunk);
            out_done += chunk;
        }
        hda_stop();
        kfree(pcm);
    } else {

        for (uint32_t i = 0; i < num_samples; i++) {
            uint8_t s = samples[i];
            sound_beep(200u + (uint32_t)s * 30u, 1000u / sample_rate);
        }
        sound_stop();
    }
    return 0;
}

static void media_play_saf_thread(void *arg) {
  sfs_node_t *file = (sfs_node_t *)arg;
  media_play_saf_sync(file);
  thread_exit();
}

int media_play_saf(sfs_node_t *file) {
  if (!file)
    return -1;
  thread_create("music_stream", media_play_saf_thread, file);
  return 0;
}


static int media_play_mp3_sync(sfs_node_t *file) {
    if (!file || !file->data || file->size < 4) return -1;

    if (!hda_available()) {
        terminal_printf("[media] mp3: no HDA audio — cannot play MP3\n");
        return -1;
    }

    mp3_decoder_t *dec = (mp3_decoder_t *)kmalloc(sizeof(mp3_decoder_t));
    if (!dec) return -1;

    mp3_init(dec, (const uint8_t *)file->data, (uint32_t)file->size);


    int16_t *pcm = (int16_t *)kmalloc(MP3_MAX_SAMPLES * sizeof(int16_t));
    if (!pcm) { kfree(dec); return -1; }

    uint32_t sample_rate = 44100;
    int channels = 2;
    int total_frames = 0;
    int n;

    terminal_printf("[media] mp3: starting playback\n");

    while ((n = mp3_decode_frame(dec, pcm, &sample_rate, &channels)) > 0) {

        uint32_t frames;
        if (channels == 2)
            frames = (uint32_t)n / 2;
        else {

            frames = (uint32_t)n;
            for (int i = (int)frames - 1; i >= 0; i--) {
                pcm[i*2 + 1] = pcm[i];
                pcm[i*2 + 0] = pcm[i];
            }
        }


        if (sample_rate != 48000 && sample_rate > 0) {
            uint32_t out_frames = (frames * 48000u) / sample_rate;
            if (out_frames == 0) out_frames = 1;

            static int16_t rs_buf[MP3_MAX_SAMPLES * 2];
            uint32_t lim = out_frames;
            if (lim * 2 > (uint32_t)(MP3_MAX_SAMPLES * 2)) lim = (uint32_t)(MP3_MAX_SAMPLES);
            for (uint32_t i = 0; i < lim; i++) {
                uint32_t si = (i * sample_rate) / 48000u;
                if (si >= frames) si = frames - 1;
                rs_buf[i*2 + 0] = pcm[si*2 + 0];
                rs_buf[i*2 + 1] = pcm[si*2 + 1];
            }
            hda_play_pcm(rs_buf, lim);
        } else {
            hda_play_pcm(pcm, frames);
        }
        total_frames += (int)frames;
    }

    hda_stop();
    terminal_printf("[media] mp3: done (%d frames)\n", total_frames);
    kfree(pcm);
    kfree(dec);
    return 0;
}

static void media_play_mp3_thread(void *arg) {
    sfs_node_t *file = (sfs_node_t *)arg;
    media_play_mp3_sync(file);
    thread_exit();
}

int media_play_mp3(sfs_node_t *file) {
    if (!file) return -1;
    thread_create("mp3_stream", media_play_mp3_thread, file);
    return 0;
}


static int media_play_svf_sync(sfs_node_t *file) {
  if (!file || !file->data || file->size < 32) {
    terminal_printf("[media] svf: bad file\n");
    return -1;
  }
  if (!vbe_active()) {
    terminal_printf("[media] svf: no VBE framebuffer\n");
    return -1;
  }
  const uint8_t *d = (const uint8_t *)file->data;
  uint32_t version = u32le(d + 4);
  uint32_t width = u32le(d + 8);
  uint32_t height = u32le(d + 12);
  uint32_t fps = u32le(d + 16);
  uint32_t frame_count = u32le(d + 20);
  if (version != 1) {
    terminal_printf("[media] svf: unsupported version\n");
    return -1;
  }
  if (!width || !height || !fps || fps > 60) {
    terminal_printf("[media] svf: bad geometry %ux%u@%u\n", width, height, fps);
    return -1;
  }
  uint32_t frame_bytes = width * height * 3u;
  if (file->size < 32u + frame_count * frame_bytes) {
    terminal_printf("[media] svf: truncated\n");
    return -1;
  }
  uint32_t ms_per_frame = 1000u / fps;
  if (!ms_per_frame)
    ms_per_frame = 1;
  terminal_printf("[media] svf: rendering %ux%u @ %u fps (background)\n", width,
                  height, fps);

  const uint8_t *fb = d + 32;
  for (uint32_t f = 0; f < frame_count; f++) {
    const uint8_t *px = fb + f * frame_bytes;
    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = 0; x < width; x++) {
        uint32_t o = (y * width + x) * 3u;
        uint32_t col = ((uint32_t)px[o] << 16) | ((uint32_t)px[o + 1] << 8) |
                       (uint32_t)px[o + 2];
        vbe_put_pixel((int)x, (int)y, col);
      }
    }
    vbe_flip();
    thread_sleep(ms_per_frame);
  }
  return 0;
}

static void media_play_svf_thread(void *arg) {
  sfs_node_t *file = (sfs_node_t *)arg;
  media_play_svf_sync(file);
  thread_exit();
}

int media_play_svf(sfs_node_t *file) {
  if (!file)
    return -1;
  thread_create("video_render", media_play_svf_thread, file);
  return 0;
}


int media_play(sfs_node_t *file, const char *filename) {
  if (ends_with(filename, ".saf"))
    return media_play_saf(file);
  if (ends_with(filename, ".mp3"))
    return media_play_mp3(file);
  if (ends_with(filename, ".svf"))
    return media_play_svf(file);
  terminal_printf("[media] unknown format: %s\n", filename);
  terminal_printf("[media] supported: .saf (audio), .mp3 (audio), .svf (video)\n");
  return -1;
}




