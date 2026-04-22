import tkinter as tk
from PIL import Image, ImageGrab
import pyperclip

grid_size = 16
pixel_size = 25
c_desk = "
c_accent = "

class iconcreatorsys:
    def __init__(self, root):
        self.root = root
        self.root.title("fbg-icon-to-clipboard")
        self.root.configure(bg=c_desk)

        self.current_color_idx = 1

        self.palette = [None, "
        self.grid_data = [[0 for _ in range(grid_size)] for _ in range(grid_size)]

        self.canvas = tk.Canvas(root, width=grid_size*pixel_size, height=grid_size*pixel_size, bg="
        self.canvas.pack(padx=20, pady=20)

        self.rect_ids = [[None for _ in range(grid_size)] for _ in range(grid_size)]
        self.draw_checkerboard()

        for y in range(grid_size):
            for x in range(grid_size):
                self.rect_ids[y][x] = self.canvas.create_rectangle(
                    x*pixel_size, y*pixel_size, (x+1)*pixel_size, (y+1)*pixel_size,
                    fill="", outline="
                )

        pal_frame = tk.Frame(root, bg=c_desk)
        pal_frame.pack(fill=tk.X, padx=20, pady=5)

        for i, color in enumerate(self.palette):
            btn_color = color if color else "
            btn_text = "X" if not color else ""
            btn = tk.Button(pal_frame, bg=btn_color, text=btn_text, width=3, command=lambda idx=i: self.set_color(idx))
            btn.pack(side=tk.LEFT, padx=2)

        ctrl_frame = tk.Frame(root, bg=c_desk)
        ctrl_frame.pack(fill=tk.X, padx=20, pady=20)

        tk.Button(ctrl_frame, text="copy struct", command=self.export_to_clipboard, bg=c_accent, fg='white', font=("monospace", 10, "bold")).pack(side=tk.RIGHT)
        tk.Button(ctrl_frame, text="paste image", command=self.import_from_clipboard, bg="
        tk.Button(ctrl_frame, text="clear", command=self.clear_grid, bg="

        self.canvas.bind("<B1-Motion>", self.paint)
        self.canvas.bind("<Button-1>", self.paint)
        self.canvas.bind("<Button-3>", self.erase)

    def draw_checkerboard(self):
        for y in range(grid_size):
            for x in range(grid_size):
                color = "
                self.canvas.create_rectangle(x*pixel_size, y*pixel_size, (x+1)*pixel_size, (y+1)*pixel_size, fill=color, outline="")

    def set_color(self, idx):
        self.current_color_idx = idx

    def paint(self, event):
        x, y = event.x // pixel_size, event.y // pixel_size
        if 0 <= x < grid_size and 0 <= y < grid_size:
            color = self.palette[self.current_color_idx]
            self.canvas.itemconfig(self.rect_ids[y][x], fill=color if color else "")
            self.grid_data[y][x] = self.current_color_idx

    def erase(self, event):
        x, y = event.x // pixel_size, event.y // pixel_size
        if 0 <= x < grid_size and 0 <= y < grid_size:
            self.canvas.itemconfig(self.rect_ids[y][x], fill="")
            self.grid_data[y][x] = 0

    def clear_grid(self):
        for y in range(grid_size):
            for x in range(grid_size):
                self.canvas.itemconfig(self.rect_ids[y][x], fill="")
                self.grid_data[y][x] = 0

    def import_from_clipboard(self):
        try:
            img = ImageGrab.grabclipboard()
            if isinstance(img, Image.Image):
                img = img.convert("RGB").resize((grid_size, grid_size), Image.NEAREST)
                for y in range(grid_size):
                    for x in range(grid_size):
                        r, g, b = img.getpixel((x, y))
                        best_idx = self.get_closest_color(r, g, b)
                        self.grid_data[y][x] = best_idx
                        self.canvas.itemconfig(self.rect_ids[y][x], fill=self.palette[best_idx] if best_idx != 0 else "")
            else:
                print("no image in clipboard.")
        except Exception as e:
            print(f"error pasting: {e}")

    def get_closest_color(self, r, g, b):

        min_dist = float('inf')
        best_idx = 1
        for i in range(1, len(self.palette)):
            hex_c = self.palette[i].lstrip('
            pr, pg, pb = tuple(int(hex_c[j:j+2], 16) for j in (0, 2, 4))
            dist = (r - pr)**2 + (g - pg)**2 + (b - pb)**2
            if dist < min_dist:
                min_dist = dist
                best_idx = i
        return best_idx

    def export_to_clipboard(self):
        lines = ["static const icon_t ICO_CUSTOM = {", "    .px={"]
        for row in self.grid_data:
            lines.append("        {" + ",".join(map(str, row)) + "},")
        lines.append("    },")
        lines.append("    .pal={C(0,0,0), C(0,160,20), C(0,220,60), C(255,230,0), C(255,80,80), C(80,160,255), C(255,255,255), C(128,128,128)}")
        lines.append("};")

        final_code = "\n".join(lines)
        pyperclip.copy(final_code)
        print("copied. go nuts.")

if __name__ == "__main__":
    root = tk.Tk()
    app = iconcreatorsys(root)
    root.mainloop()



