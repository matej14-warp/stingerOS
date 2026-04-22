



















EV_NONE   = 0
EV_KEY    = 1
EV_CLICK  = 2
EV_CLOSE  = 3
EV_RESIZE = 4


BLACK   = 0x000000
WHITE   = 0xFFFFFF
RED     = 0xFF3030
GREEN   = 0x00DC50
BLUE    = 0x3078FF
CYAN    = 0x00C8DC
YELLOW  = 0xFFC800
MAGENTA = 0xC800C8
GREY    = 0x888888
DARK    = 0x181818
ACCENT  = 0x0078D7

class Window:
    """Custom GUI window. All drawing is relative to the client area."""

    def __init__(self, title="Window", x=100, y=100, w=400, h=300):
        """Create a new window. Returns a Window handle."""

        pass

    def clear(self, colour=BLACK):
        """Fill the entire client area with colour (0xRRGGBB)."""
        pass

    def fill_rect(self, x, y, w, h, colour):
        """Draw a filled rectangle."""
        pass

    def text(self, x, y, string, colour=WHITE):
        """Draw text at (x, y) in the given colour."""
        pass

    def pixel(self, x, y, colour):
        """Set a single pixel."""
        pass

    def flush(self):
        """Blit the title bar and flip the window region to screen."""
        pass

    def width(self):
        """Client area width in pixels."""
        return 0

    def height(self):
        """Client area height in pixels."""
        return 0

    def alive(self):
        """Return True if the window has not been closed."""
        return True

    def poll(self):
        """
        Return the next pending event as [type, a, b], or None if no events.
          EV_KEY:   [EV_KEY, key_char_code, 0]
          EV_CLICK: [EV_CLICK, mx, my]
          EV_CLOSE: [EV_CLOSE, 0, 0]
        """
        return None

    def destroy(self):
        """Close and destroy the window."""
        pass


def sleep(seconds):
    """Sleep for the given number of seconds (float supported)."""
    pass

def print_ok():
    """Print an OK message from the stinger runtime."""
    pass


if __name__ == "__main__":
    w = Window("Demo", 80, 80, 500, 350)
    w.clear(DARK)
    w.text(20, 20, "Hello from stinger!", WHITE)
    w.text(20, 44, "Press any key to quit.", GREY)
    w.flush()
    while w.alive():
        ev = w.poll()
        if ev:
            if ev[0] == EV_CLOSE or ev[0] == EV_KEY:
                break
        sleep(0.016)
    w.destroy()




