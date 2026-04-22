





























































int  gpu_init_i915(void);


int  i915_active(void);


void i915_blit_fb(void *dst_ignored, void *src, int pitch_bytes, int h);


void i915_blit_fb_rect(void *dst_ignored, void *src, int pitch_bytes,
                        int dx, int dy, int w, int h);


void i915_blit_rect_phys(uint32_t dst_phys, uint32_t src_phys,
                         uint32_t dst_pitch, uint32_t src_pitch,
                         int w, int h);


void i915_fill_rect(void *dst_phys, uint32_t pitch, int x1, int y1, int x2, int y2, uint32_t color);


void gpu_hw_flush(void);


void i915_flush_batch(void);
void i915_batch_begin(void);
void i915_batch_end(void);


void gpu_sync(void);


void i915_hw_cursor_init(void *argb_pixels);
void i915_hw_cursor_move(int x, int y);


void i915_fbc_init(uint32_t fb_phys, uint32_t fb_size);


void i915_gpu_wake(void);
void i915_gpu_turbo(int on);
void i915_page_flip(uint32_t phys_addr);







