












void cmd_femboygl_test(int wx, int wy, int ww, int wh) {
    int cx = wx + 2, cy = wy + 34, cw = ww - 4, ch = wh - 36;
    if (cw < 64 || ch < 64) return;

    uint32_t *color_buf = (uint32_t *)kmalloc((size_t)cw * ch * 4);
    float    *depth_buf = (float    *)kmalloc((size_t)cw * ch * sizeof(float));
    if (!color_buf || !depth_buf) return;


    gl_context_t gl;
    gl_init(&gl, cw, ch, color_buf, depth_buf);

    int mode = 0;
    float time = 0.0f;
    int running = 1;

    while (running) {

        if (keyboard_keyready()) {
            char k = keyboard_poll();
            if (k == 'q' || k == 'Q' || k == 27) running = 0;
            else if (k >= '0' && k <= '7') mode = k - '0';
        }


        time += 0.03f;
        gl_clear(&gl, 0xFF1A1A2E);


        switch(mode) {
            case 0:

                break;
            case 1:

                break;

            default:
                break;
        }


        vbe_bb_blit_rect(cx, cy, cw, ch, color_buf);
        vbe_flip_rect(wx, wy, ww, wh);

        for(volatile int i=0; i<100000; i++);
    }

    kfree(color_buf);
    kfree(depth_buf);
}




