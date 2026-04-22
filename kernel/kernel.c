




































extern void shell_init(void);
extern void __attribute__((noreturn)) yaoigui_run(void);





typedef struct {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t syms[4];
  uint32_t mmap_length;
  uint32_t mmap_addr;
  uint32_t drives_length;
  uint32_t drives_addr;
  uint32_t config_table;
  uint32_t boot_loader_name;
  uint32_t apm_table;
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint16_t vbe_mode;
  uint16_t vbe_interface_seg;
  uint16_t vbe_interface_off;
  uint16_t vbe_interface_len;
  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
  uint8_t framebuffer_type;
  uint8_t color_info[6];
} __attribute__((packed)) multiboot_info_t;

static volatile uint32_t ticks = 0;
static void timer_handler(registers_t *regs) {
  (void)regs;
  ticks++;
}
uint32_t get_ticks(void) { return ticks; }
void sleep_ms(uint32_t ms) {
  uint32_t start = ticks;

  while ((uint32_t)(ticks - start) < ms)
    __asm__ volatile("sti; hlt");
}
static void timer_init(uint32_t freq) {
  register_interrupt_handler(32, timer_handler);
  uint32_t divisor = 1193180 / freq;
  uint8_t lo = (uint8_t)(divisor & 0xFF);
  uint8_t hi = (uint8_t)((divisor >> 8) & 0xFF);
  __asm__ volatile("outb %0, $0x43" : : "a"((uint8_t)0x36));
  __asm__ volatile("outb %0, $0x40" : : "a"(lo));
  __asm__ volatile("outb %0, $0x40" : : "a"(hi));
}


static int boot_mode_prompt(void) {
  if (!vbe_active()) {

    terminal_writestring("[stinger] No VBE framebuffer — starting CLI\n");
    return 0;
  }

  terminal_writestring("\n");
  terminal_writestring("  +------------------------------------------+\n");
  terminal_writestring("  |   stinger v4.0  --  Boot Mode          |\n");
  terminal_writestring("  |                                            |\n");
  terminal_writestring("  |   Start GUI compositor?  [Y/n]            |\n");
  terminal_writestring("  |   (defaults to GUI after 5s timeout)      |\n");
  terminal_writestring("  +------------------------------------------+\n");
  terminal_writestring("\n  > ");


  char k = keyboard_getchar_timeout(5000);

  if (k == 'n' || k == 'N') {
    terminal_writestring("n — starting CLI\n");
    return 0;
  }

  if (k == 0)
    terminal_writestring("(timeout) — starting GUI\n");
  else
    terminal_writestring("y — starting GUI\n");
  return 1;
}

void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
  (void)magic;
  terminal_init();


  if (mbi && (mbi->flags & (1 << 11)) && mbi->vbe_mode_info) {
    if (vbe_init(mbi->vbe_mode_info) == 0) {
      vbe_terminal_init();
    }
  }

  if (vbe_active()) {
    vbe_terminal_setcolor(0x0A);
    vbe_terminal_writestring("[stinger] VBE framebuffer active\n");
    vbe_terminal_setcolor(0x07);
  } else {
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("[stinger] kernel v4.0 starting...\n");
    terminal_writestring("[stinger] nevermind vro ts too hard for me\n");
    terminal_setcolor(
        terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring(
        "[stinger] destroying hackers... all hackers destroyed!\n");
    terminal_writestring("[stinger] publicly executing stemboy.... done!\n");
  }

  terminal_printf("[stinger] GDT/IDT...\n");
  gdt_init();
  idt_init();
  extern void hda_isr(registers_t *regs);
  register_interrupt_handler(43, hda_isr);
  __asm__ volatile("sti");
  terminal_printf("[stinger] interrupts enabled\n");
  terminal_printf(
      "[evil malware wanting to hack you] psst bro im not dangerous but uh "
      "whats your SSN and birthday? i need it for my school project\n");

  terminal_printf("[stinger] PIT @ 1000Hz...\n");
  timer_init(1000);


  terminal_printf("[stinger] Paging...\n");
  terminal_printf("[evil malware wanting to hack you] sowwy i depaged it :3 "
                  "now gimme bitcoin so i can fix it\n");
  paging_init(mbi && (mbi->flags & 1) ? mbi->mem_upper + 1024u : 256u * 1024u);
  paging_enable();


  if (vbe_active()) {
    extern void vbe_rebase_framebuffer(void);
    vbe_rebase_framebuffer();
  }


  {
    uint8_t *virt_lapic = NULL;
    if (paging_map_mmio(0xFEE00000, 4096, &virt_lapic) == 0) {
      extern volatile uint32_t *lapic;
      lapic = (volatile uint32_t *)virt_lapic;
      terminal_printf("[stinger] LAPIC mapped to virtual 0x%08x\n",
                      (uint32_t)virt_lapic);
    }
  }

  if (mbi && (mbi->flags & 1))
    terminal_printf("[stinger] RAM: %dKB lower, %dKB upper (%dMB)\n",
                    mbi->mem_lower, mbi->mem_upper, mbi->mem_upper / 1024);

  terminal_printf("[stinger] stingerFS...\n");
  sfs_init();
  terminal_printf("[stinger] stingerFS ready\n");


  {
    terminal_printf("[stinger] extracting filesystem objects...\n");

    for (int i = 0; overlay_files[i].path != NULL; i++) {
      const char *p = overlay_files[i].path;
      if (*p == '/')
        p++;


      char path_buf[256];
      size_t pi = 0;
      while (p[pi] && pi < 255) {
        path_buf[pi] = p[pi];
        pi++;
      }
      path_buf[pi] = '\0';

      sfs_node_t *cur_dir = sfs_root();
      char *start = path_buf;
      char *slash = NULL;


      for (char *scan = start; *scan; scan++) {
        if (*scan == '/') {
          slash = scan;
          break;
        }
      }

      while (slash) {
        *slash = '\0';
        sfs_node_t *next = sfs_find_child(cur_dir, start);
        if (!next)
          next = sfs_mkdir(cur_dir, start);
        if (next)
          cur_dir = next;

        start = slash + 1;
        slash = NULL;
        for (char *scan = start; *scan; scan++) {
          if (*scan == '/') {
            slash = scan;
            break;
          }
        }
      }


      if (cur_dir && *start) {
        sfs_write_file(cur_dir, start, overlay_files[i].data,
                       overlay_files[i].size);
      }
    }
    terminal_printf("[stinger] overlay embedded\n");


    if (mbi && (mbi->flags & (1 << 3)) && mbi->mods_count > 0) {
      terminal_printf("[stinger] found %d multiboot modules\n",
                      mbi->mods_count);
      typedef struct {
        uint32_t start, end, str, rsv;
      } mb_mod_t;
      mb_mod_t *mods = (mb_mod_t *)mbi->mods_addr;
      for (uint32_t i = 0; i < mbi->mods_count; i++) {
        const char *cmd = (const char *)mods[i].str;
        uint8_t *data = (uint8_t *)mods[i].start;
        uint32_t size = mods[i].end - mods[i].start;
        char pb[256];
        int pi = 0;

        for (int ci = 0; cmd[ci] && pi < 255; ci++) {
          if (cmd[ci] == ' ' || cmd[ci] == '\t' || cmd[ci] == '\n' ||
              cmd[ci] == '\r')
            break;
          if (cmd[ci] != '"')
            pb[pi++] = cmd[ci];
        }
        pb[pi] = 0;

        terminal_printf(
            "[stinger] embedding module: '%s' (%u bytes at 0x%08x)\n", pb,
            size, (uint32_t)data);

        const char *p = (pb[0] == '/') ? pb + 1 : pb;
        sfs_node_t *cur = sfs_root();
        char *st = (char *)p;
        for (char *c = (char *)p; *c; c++) {
          if (*c == '/') {
            *c = 0;
            sfs_node_t *nx = sfs_find_child(cur, st);
            if (!nx)
              nx = sfs_mkdir(cur, st);
            if (nx)
              cur = nx;
            st = c + 1;
          }
        }
        if (cur && *st) {
          sfs_node_t *node = sfs_write_file(cur, st, data, size);
          if (node) {
            terminal_printf(
                "[stinger] embedded '%s' as node 0x%08x (data: 0x%08x)\n", st,
                (uint32_t)node, (uint32_t)node->data);
          }
        }
      }
    }
  }


  {
    terminal_printf("[stinger] embedding installer...\n");

    sfs_node_t *etc = sfs_resolve(sfs_root(), "/etc");
    if (!etc)
      etc = sfs_mkdir(sfs_root(), "etc");
    if (etc)
      sfs_write_file(etc, "installer.mbf", installer_mbf, installer_mbf_len);
    terminal_printf("[stinger] installer embedded\n");
  }


  terminal_printf("[stinger] ACPI...\n");
  if (acpi_init() == 0) {
    terminal_printf("[stinger] ACPI enabling...\n");
    acpi_enable();
    terminal_printf("[stinger] ACPI ready\n");
  } else
    terminal_printf("[stinger] ACPI unavailable (QEMU fallback)\n");

  terminal_printf("[stinger] PCI scanning...\n");
  pci_scan();
  terminal_printf("[stinger] PCI scan complete\n");

  terminal_printf("[stinger] Intel GPU (Gen 11 i915)...\n");
  gpu_init_i915();
  terminal_printf("[stinger] GPU 'made in china' check completed.\n");

  if (i915_active()) {

    terminal_printf("[stinger] i915 using using CPU back-buffer path.\n");
  }


  extern volatile int g_smp_go;
  g_smp_go = 1;

  terminal_printf("[stinger] SMP/APIC...\n");
  lapic_init();
  int ncpus = smp_detect();
  terminal_printf("[stinger] %d CPU(s)\n", ncpus);


  for (int i = 1; i < ncpus && i < SMP_MAX_CPUS; i++) {
    smp_boot_ap_start(g_cpus[i].apic_id, 8);
  }


  sleep_ms(20);


  for (int i = 1; i < ncpus && i < SMP_MAX_CPUS; i++) {
    if (smp_boot_ap_finish(g_cpus[i].apic_id) == 0)
      terminal_printf("[smp] AP %d online\n", i);
    else
      terminal_printf("[smp] AP %d failed\n", i);
  }

  for (int i = 0; i < NSIG; i++)
    g_sigstate.handlers[i] = SIG_DFL;
  terminal_printf("[stinger] signals/IPC ready\n");

  dynlink_init();
  kapi_init();
  sched_init();


  terminal_printf("[stinger] keyboard...\n");
  keyboard_init();

  terminal_printf("[stinger] ATA scan...\n");
  ata_init();
  ahci_init();


  pci_list();


  pci_device_t *hda_dev = pci_find_class(0x04, 0x03);
  if (hda_dev) {
    uint32_t bar = hda_dev->bar[0] & ~0xF;
    if (paging_map_mmio(bar, 4096, (uint8_t **)&g_hda_base) == 0) {

      pci_enable_busmaster(hda_dev->bus, hda_dev->dev, hda_dev->fn);
      terminal_printf("[stinger] HDA controller mapped to 0x%08x\n",
                      (uint32_t)g_hda_base);
    }
  }


  {
    static part_entry_t _parts[16];
    int _nparts = partition_detect_all(_parts, 16);
    int _drive;
    uint32_t _lba;
    int _pi = sfs_find_sync_target(_parts, _nparts, &_drive, &_lba);
    if (_pi >= 0) {
      terminal_printf("[stinger] SFS partition found, mounting...\n");
      if (sfs_load_from_disk(_drive, _lba) == 0) {
        sfs_set_persist_target(_drive, _lba);
        terminal_printf("[stinger] SFS mounted and auto-persist enabled\n");
      } else {
        terminal_printf("[stinger] SFS mount failed, starting fresh\n");
      }
    } else {
      terminal_printf(
          "[stinger] no SFS partition found, filesystem is RAM-only\n");
    }
  }

  terminal_printf("[stinger] USB...\n");
  if (usb_init() == 0) {
    xhci_init();
    usb_scan_devices();
  }

  terminal_printf("[stinger] mouse...\n");
  mouse_init();

  terminal_printf("[stinger] WiFi...\n");
  ax201_probe();
  wifi_rtl8188_probe();
  wifi_rtl8192_probe();
  wifi_ath9k_probe();
  wifi_mt7601_probe();
  wifi_iwlwifi_probe();
  wifi_rt2800_probe();
  wifi_brcmfmac_probe();
  if (!wifi_active)
    terminal_printf("[stinger] no WiFi adapter\n");


  terminal_printf("[stinger] Ethernet...\n");
  int net_up = 0;
  {
    uint8_t _mac[6];
    if (!net_up && rndis_init() == 0) {
      rndis_get_mac(_mac);
      rndis_set_receive_cb(net_receive_frame);
      net_set_poll_fn(rndis_poll);
      net_set_send_fn(rndis_send);
      net_init();
      net_set_mac(_mac);
      terminal_printf("[stinger] RTL8139 ready\n");
      net_up = 1;
    }
    if (!net_up && e1000_init() == 0) {
      e1000_get_mac(_mac);
      e1000_set_rx_cb(net_receive_frame);
      net_set_poll_fn(e1000_poll);
      net_set_send_fn(e1000_send);
      net_init();
      net_set_mac(_mac);
      terminal_printf("[stinger] e1000 ready\n");
      net_up = 1;
    }
    if (!net_up && virtio_net_init() == 0) {
      virtio_net_get_mac(_mac);
      virtio_net_set_rx_cb(net_receive_frame);
      net_set_poll_fn(virtio_net_poll);
      net_set_send_fn(virtio_net_send);
      net_init();
      net_set_mac(_mac);
      terminal_printf("[stinger] virtio-net ready\n");
      net_up = 1;
    }
    if (!net_up && ne2000_init() == 0) {
      ne2000_get_mac(_mac);
      ne2000_set_rx_cb(net_receive_frame);
      net_set_poll_fn(ne2000_poll);
      net_set_send_fn(ne2000_send);
      net_init();
      net_set_mac(_mac);
      terminal_printf("[stinger] ne2000 ready\n");
      net_up = 1;
    }
    if (!net_up && usbnet_probe() == 0) {
      usbnet_get_mac(_mac);
      usbnet_set_rx_cb(net_receive_frame);
      net_set_poll_fn(usbnet_poll);
      net_set_send_fn(usbnet_send);
      net_init();
      net_set_mac(_mac);
      terminal_printf("[stinger] USB-NET ready\n");
      net_up = 1;
    }
    if (!net_up) {
      net_init();
      terminal_printf("[stinger] WARNING: no network adapter found\n");
    }
  }
  net_dhcp();
  terminal_printf("[stinger] Network configured (DHCP finished/timed out)\n");
  dns_init();

  make_init();

  sleep_ms(100);
  sound_init();
  sleep_ms(100);

  terminal_printf("[stinger] v4.0 init complete\n\n");
  auth_init_etc();
  login_prompt();
  vbe_flip();

  shell_init();


  {
    int start_gui = 1;
    sfs_node_t *etc2 = sfs_resolve(sfs_root(), "/etc");
    if (etc2) {
      sfs_node_t *f = sfs_find_child(etc2, "gui_autostart");
      if (f && f->data && f->size > 0)
        start_gui = ((char *)f->data)[0] != '0';
    }
    if (start_gui)
      yaoigui_run();
  }
  shell_run();

  kernel_panic("shell returned unexpectedly", NULL);
  __asm__ volatile("cli");
  for (;;)
    __asm__ volatile("hlt");
}



