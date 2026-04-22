






            if(_hit&&_pdist>0){
                int _proj_h=(render_h*256)/_pdist;
                if(_proj_h<1)_proj_h=1;
                if(_proj_h>render_h*4)_proj_h=render_h*4;

                int _ry_base = py_f>>8;



                int _y_offset = (int)((_hy - _ry_base) * (_proj_h / 16));

                int _dtop = half - _proj_h/2 + _y_offset - (pitch * 3);
                int _dbot = _dtop + _proj_h;

                int _vtop = _dtop < 0 ? 0 : (_dtop > render_h ? render_h : _dtop);
                int _vbot = _dbot < 0 ? 0 : (_dbot > render_h ? render_h : _dbot);

                uint32_t _base = (!_hface) ? BLK_TOP[_hblk] : BLK_SIDE[_hblk];
                if(_hface==0){
                    int _r2=(int)((_base>>16)&0xFF)*3/4;
                    int _g2=(int)((_base>>8)&0xFF)*3/4;
                    int _b2=(int)(_base&0xFF)*3/4;
                    _base=_GG_COL(_r2,_g2,_b2);
                }

                int _ds=_pdist>>8;
                if(_ds>10)_ds=10;
                int _fade=10-_ds;

                int _r2=(int)((_base>>16)&0xFF)*(_fade+4)/14;
                int _g2=(int)((_base>>8)&0xFF)*(_fade+4)/14;
                int _b2=(int)(_base&0xFF)*(_fade+4)/14;

                if(_r2>255)_r2=255; if(_g2>255)_g2=255; if(_b2>255)_b2=255;
                uint32_t _wc=_GG_COL(_r2,_g2,_b2);

                for(int _sy3=0;_sy3<render_h;_sy3++){
                    if(_sy3<_vtop) vbe_put_pixel(cx+_sx3,cy+_sy3,MC_SKY(_sy3));
                    else if(_sy3<_vbot) vbe_put_pixel(cx+_sx3,cy+_sy3,_wc);
                    else {
                        int _d=_sy3-(half-pitch*3);
                        uint32_t _fc=_GG_COL(65-_d/6>0?65-_d/6:0,55,35);
                        vbe_put_pixel(cx+_sx3,cy+_sy3,_fc);
                    }
                }
            }




