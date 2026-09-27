import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageTk
import os
import time
import re
import warnings

# 抑制 Pillow 对损坏 EXIF 数据的警告
warnings.filterwarnings("ignore", category=UserWarning, module="PIL")


# =========================================================
# 索引色打包函数（LVGL 8.3 兼容）
# =========================================================
def image_to_indexed(img, bits):
    """
    将 PIL RGB 图像转为 LVGL 索引色格式。
    返回: (data_bytes, n_colors)
    LVGL 8.3 索引色数据布局:
        [调色板: N*4 字节, BGRA 顺序, A=0xFF]
        [像素索引数据: 按位深打包, 高位在前]
    """
    n_colors = 1 << bits  # 2, 4, 16, 256

    if img.mode != "RGB":
        img = img.convert("RGB")

    # 1) 量化到 n_colors 色
    quantized = img.quantize(colors=n_colors, method=Image.MEDIANCUT)
    palette_data = quantized.getpalette()

    # 2) 生成调色板（BGRA，A=0xFF）
    palette_bytes = bytearray()
    for i in range(n_colors):
        if palette_data and i * 3 + 2 < len(palette_data):
            r = palette_data[i * 3]
            g = palette_data[i * 3 + 1]
            b = palette_data[i * 3 + 2]
        else:
            r = g = b = 0
        palette_bytes += bytes([b, g, r, 0xFF])

    # 3) 生成像素索引数据
    w, h = quantized.size
    indices = list(quantized.getdata())

    packed = bytearray()
    if bits == 8:
        packed.extend(indices)

    elif bits == 4:
        for i in range(0, len(indices), 2):
            hi = indices[i] & 0x0F
            lo = (indices[i + 1] & 0x0F) if (i + 1) < len(indices) else 0
            packed.append((hi << 4) | lo)

    elif bits == 2:
        for i in range(0, len(indices), 4):
            b_byte = 0
            for j in range(4):
                idx = indices[i + j] & 0x03 if (i + j) < len(indices) else 0
                b_byte |= idx << (6 - 2 * j)
            packed.append(b_byte)

    elif bits == 1:
        for i in range(0, len(indices), 8):
            b_byte = 0
            for j in range(8):
                idx = indices[i + j] & 0x01 if (i + j) < len(indices) else 0
                b_byte |= idx << (7 - j)
            packed.append(b_byte)

    return bytes(palette_bytes) + bytes(packed), n_colors


class ImageCropIndexedApp:
    def __init__(self, root):
        self.root = root
        self.root.title("图片预处理缩放 + 裁剪 + LVGL 索引色/ RGB565 生成器")
        self.root.geometry("1360x880")

        self.original_image = None
        self.working_image = None
        self.preview_image = None
        self.display_image = None
        self.preview_photo = None
        self.scale_factor = 1.0
        self.offset_x = 0
        self.offset_y = 0

        self.crop_w = 240
        self.crop_h = 240
        self.crop_x = 0
        self.crop_y = 0
        self.drag_offset_x = 0
        self.drag_offset_y = 0
        self.is_dragging = False
        self.active_handle = None
        self.handles = {}
        self.last_move_time = 0

        self._build_ui()

    def _build_ui(self):
        top = ttk.Frame(self.root)
        top.pack(side=tk.TOP, fill=tk.X, padx=5, pady=(5, 2))
        ttk.Button(top, text="打开图片", command=self.open_image).pack(side=tk.LEFT, padx=2)
        self.file_label = ttk.Label(top, text="未加载", foreground="#666")
        self.file_label.pack(side=tk.LEFT, padx=10)

        # 缩放区
        scale_frame = ttk.LabelFrame(self.root, text="① 预处理缩放（先降分辨率）")
        scale_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=2)
        ttk.Label(scale_frame, text="缩放模式:").pack(side=tk.LEFT, padx=(5, 2))
        self.scale_mode = tk.StringVar(value="percent")
        ttk.Radiobutton(scale_frame, text="按比例(%)", variable=self.scale_mode,
                        value="percent", command=self._on_scale_mode_change).pack(side=tk.LEFT)
        ttk.Radiobutton(scale_frame, text="按目标宽度", variable=self.scale_mode,
                        value="width", command=self._on_scale_mode_change).pack(side=tk.LEFT)
        self.scale_percent_var = tk.StringVar(value="50")
        ttk.Label(scale_frame, text="比例:").pack(side=tk.LEFT, padx=(15, 2))
        self.percent_entry = ttk.Entry(scale_frame, textvariable=self.scale_percent_var, width=6)
        self.percent_entry.pack(side=tk.LEFT)
        ttk.Label(scale_frame, text="%").pack(side=tk.LEFT)
        ttk.Label(scale_frame, text="目标宽度:").pack(side=tk.LEFT, padx=(15, 2))
        self.target_width_var = tk.StringVar(value="480")
        self.width_entry = ttk.Entry(scale_frame, textvariable=self.target_width_var, width=6)
        self.width_entry.pack(side=tk.LEFT)
        ttk.Label(scale_frame, text="px").pack(side=tk.LEFT)
        ttk.Button(scale_frame, text="应用缩放", command=self.apply_prescale).pack(side=tk.LEFT, padx=10)
        ttk.Button(scale_frame, text="恢复原图", command=self.reset_prescale).pack(side=tk.LEFT, padx=2)
        self.working_size_label = ttk.Label(scale_frame, text="工作图: -", foreground="#0066CC")
        self.working_size_label.pack(side=tk.LEFT, padx=15)
        self._on_scale_mode_change()

        # 裁剪区
        crop_frame = ttk.LabelFrame(self.root, text="② 裁剪设置")
        crop_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=2)
        ttk.Label(crop_frame, text="裁剪宽度:").pack(side=tk.LEFT, padx=(5, 2))
        self.crop_w_var = tk.StringVar(value="240")
        ttk.Entry(crop_frame, textvariable=self.crop_w_var, width=6).pack(side=tk.LEFT)
        ttk.Label(crop_frame, text="高度:").pack(side=tk.LEFT, padx=(10, 2))
        self.crop_h_var = tk.StringVar(value="240")
        ttk.Entry(crop_frame, textvariable=self.crop_h_var, width=6).pack(side=tk.LEFT)
        ttk.Button(crop_frame, text="应用尺寸", command=self.apply_crop_size).pack(side=tk.LEFT, padx=5)
        ttk.Button(crop_frame, text="居中裁剪框", command=self.center_crop).pack(side=tk.LEFT, padx=2)

        # 输出区
        out_frame = ttk.LabelFrame(self.root, text="③ 输出")
        out_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=2)

        ttk.Label(out_frame, text="输出格式:").pack(side=tk.LEFT, padx=(5, 2))
        self.output_format = tk.StringVar(value="RGB565 uint8_t 字节数组")
        self.format_combo = ttk.Combobox(
            out_frame, textvariable=self.output_format,
            values=[
                "RGB565 uint8_t 字节数组",
                "RGB565 uint16_t 数组",
                "RGB565 LVGL lv_img_dsc_t",
                "INDEXED_8BIT (256色) LVGL",
                "INDEXED_4BIT (16色)  LVGL",
                "INDEXED_2BIT (4色)   LVGL",
                "INDEXED_1BIT (2色)   LVGL",
            ],
            state="readonly", width=30)
        self.format_combo.pack(side=tk.LEFT, padx=2)

        ttk.Label(out_frame, text="字节序:").pack(side=tk.LEFT, padx=(15, 2))
        self.byte_order = tk.StringVar(value="big")
        ttk.Radiobutton(out_frame, text="大端", variable=self.byte_order, value="big").pack(side=tk.LEFT)
        ttk.Radiobutton(out_frame, text="小端", variable=self.byte_order, value="little").pack(side=tk.LEFT)

        ttk.Button(out_frame, text="生成代码", command=self.generate_array).pack(side=tk.LEFT, padx=20)

        # 主区域
        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        left_frame = ttk.LabelFrame(paned, text="工作图（拖动绿色框 / 拖动 8 个手柄缩放）")
        paned.add(left_frame, weight=3)
        self.canvas = tk.Canvas(left_frame, bg="#333", cursor="fleur")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.canvas.bind("<Configure>", self.on_canvas_resize)
        self.canvas.bind("<Button-1>", self.on_mouse_down)
        self.canvas.bind("<B1-Motion>", self.on_mouse_move)
        self.canvas.bind("<ButtonRelease-1>", self.on_mouse_up)

        right_frame = ttk.Frame(paned)
        paned.add(right_frame, weight=2)
        preview_frame = ttk.LabelFrame(right_frame, text="裁剪预览（最终输出效果）")
        preview_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=False, pady=(0, 5))
        self.preview_canvas = tk.Canvas(preview_frame, bg="#222", width=260, height=260)
        self.preview_canvas.pack(padx=5, pady=5)

        text_frame = ttk.LabelFrame(right_frame, text="生成的 C 代码")
        text_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.text_output = tk.Text(text_frame, wrap=tk.NONE, font=("Courier", 9))
        self.text_output.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        sb = ttk.Scrollbar(text_frame, orient=tk.VERTICAL, command=self.text_output.yview)
        sb.pack(side=tk.RIGHT, fill=tk.Y)
        self.text_output.config(yscrollcommand=sb.set)

        self.status_var = tk.StringVar(value="就绪")
        ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN,
                  anchor=tk.W).pack(side=tk.BOTTOM, fill=tk.X)

    # ============ 缩放 ============
    def _on_scale_mode_change(self):
        if self.scale_mode.get() == "percent":
            self.percent_entry.config(state="normal")
            self.width_entry.config(state="disabled")
        else:
            self.percent_entry.config(state="disabled")
            self.width_entry.config(state="normal")

    def apply_prescale(self):
        if self.original_image is None:
            messagebox.showwarning("提示", "请先打开图片")
            return
        img = self.original_image
        try:
            if self.scale_mode.get() == "percent":
                pct = float(self.scale_percent_var.get())
                if pct <= 0:
                    raise ValueError
                new_w = max(1, int(img.width * pct / 100))
                new_h = max(1, int(img.height * pct / 100))
            else:
                tw = int(self.target_width_var.get())
                if tw <= 0:
                    raise ValueError
                ratio = tw / img.width
                new_w = tw
                new_h = max(1, int(img.height * ratio))
        except ValueError:
            messagebox.showerror("错误", "请输入有效的数值")
            return
        self.working_image = img.resize((new_w, new_h), Image.LANCZOS)
        self.working_size_label.config(text=f"工作图: {new_w}x{new_h}")
        self.status_var.set(f"已缩放到 {new_w}x{new_h}")
        self.crop_x = max(0, (new_w - self.crop_w) // 2)
        self.crop_y = max(0, (new_h - self.crop_h) // 2)
        self.clamp_crop()
        self.redraw_canvas()
        self.update_preview()

    def reset_prescale(self):
        if self.original_image is None:
            return
        self.working_image = self.original_image.copy()
        self.working_size_label.config(text=f"工作图: {self.working_image.width}x{self.working_image.height}")
        self.crop_x = max(0, (self.working_image.width - self.crop_w) // 2)
        self.crop_y = max(0, (self.working_image.height - self.crop_h) // 2)
        self.clamp_crop()
        self.redraw_canvas()
        self.update_preview()
        self.status_var.set("已恢复为原图")

    # ============ 打开图片 ============
    def open_image(self):
        path = filedialog.askopenfilename(
            filetypes=[("图片文件", "*.png;*.jpg;*.jpeg;*.bmp;*.gif")])
        if not path:
            return
        try:
            img = Image.open(path).convert("RGB")
            self.original_image = img
            self.working_image = img.copy()
            self.file_label.config(text=f"{os.path.basename(path)}  ({img.width}x{img.height})")
            self.working_size_label.config(text=f"工作图: {img.width}x{img.height}")
            self.crop_x = max(0, (img.width - self.crop_w) // 2)
            self.crop_y = max(0, (img.height - self.crop_h) // 2)
            self.clamp_crop()
            self.redraw_canvas()
            self.update_preview()
            self.status_var.set(f"已加载: {os.path.basename(path)}")
        except Exception as e:
            messagebox.showerror("错误", f"无法打开图片: {e}")

    # ============ 画布 ============
    def on_canvas_resize(self, event=None):
        self.redraw_canvas()

    def redraw_canvas(self):
        if self.working_image is None:
            return
        img = self.working_image
        cw = self.canvas.winfo_width()
        ch = self.canvas.winfo_height()
        if cw <= 1 or ch <= 1:
            self.root.after(50, self.redraw_canvas)
            return
        scale_w = cw / img.width
        scale_h = ch / img.height
        self.scale_factor = min(scale_w, scale_h, 4.0)
        disp_w = int(img.width * self.scale_factor)
        disp_h = int(img.height * self.scale_factor)
        self.offset_x = (cw - disp_w) // 2
        self.offset_y = (ch - disp_h) // 2
        disp_img = img.resize((disp_w, disp_h),
                              Image.NEAREST if self.scale_factor >= 2 else Image.LANCZOS)
        self.display_image = ImageTk.PhotoImage(disp_img)
        self.canvas.delete("all")
        self.canvas.create_image(self.offset_x, self.offset_y, anchor=tk.NW, image=self.display_image)
        self.draw_crop_box()

    def draw_crop_box(self):
        if self.working_image is None:
            return
        self.canvas.delete("overlay")
        self.canvas.delete("cropbox")
        self.canvas.delete("handle")
        cx1 = self.offset_x + self.crop_x * self.scale_factor
        cy1 = self.offset_y + self.crop_y * self.scale_factor
        cx2 = self.offset_x + (self.crop_x + self.crop_w) * self.scale_factor
        cy2 = self.offset_y + (self.crop_y + self.crop_h) * self.scale_factor
        self.canvas.create_rectangle(cx1, cy1, cx2, cy2, outline="#00FF00", width=2, tags="cropbox")
        hs = 6
        positions = {
            "nw": (cx1, cy1), "n": ((cx1+cx2)//2, cy1), "ne": (cx2, cy1),
            "w":  (cx1, (cy1+cy2)//2), "e": (cx2, (cy1+cy2)//2),
            "sw": (cx1, cy2), "s": ((cx1+cx2)//2, cy2), "se": (cx2, cy2),
        }
        self.handles.clear()
        for name, (px, py) in positions.items():
            hid = self.canvas.create_rectangle(
                px-hs, py-hs, px+hs, py+hs, fill="#00FF00", outline="#000", tags="handle")
            self.handles[hid] = name

    def canvas_to_image_coords(self, cx, cy):
        if self.working_image is None:
            return None, None
        return (cx - self.offset_x) / self.scale_factor, (cy - self.offset_y) / self.scale_factor

    def on_mouse_down(self, event):
        if self.working_image is None:
            return
        items = self.canvas.find_overlapping(event.x-3, event.y-3, event.x+3, event.y+3)
        handle_name = None
        for item in items:
            if item in self.handles:
                handle_name = self.handles[item]
                break
        if handle_name:
            self.is_dragging = "resize"
            self.active_handle = handle_name
            self.start_crop = (self.crop_x, self.crop_y, self.crop_w, self.crop_h)
            return
        ix, iy = self.canvas_to_image_coords(event.x, event.y)
        if self.crop_x <= ix <= self.crop_x + self.crop_w and self.crop_y <= iy <= self.crop_y + self.crop_h:
            self.is_dragging = "move"
            self.drag_offset_x = ix - self.crop_x
            self.drag_offset_y = iy - self.crop_y
            return
        self.is_dragging = "new"
        self.update_crop_position(ix - self.crop_w/2, iy - self.crop_h/2)

    def on_mouse_move(self, event):
        now = time.time()
        if now - self.last_move_time < 0.02:
            return
        self.last_move_time = now
        if not self.is_dragging or self.working_image is None:
            return
        ix, iy = self.canvas_to_image_coords(event.x, event.y)
        if ix is None or iy is None:
            return
        if self.is_dragging == "move":
            self.update_crop_position(ix - self.drag_offset_x, iy - self.drag_offset_y)
        elif self.is_dragging == "new":
            self.update_crop_position(ix - self.crop_w/2, iy - self.crop_h/2)
        elif self.is_dragging == "resize":
            x, y, w, h = self.start_crop
            left, top, right, bottom = x, y, x + w, y + h
            hname = self.active_handle
            if "w" in hname: left = ix
            if "e" in hname: right = ix
            if "n" in hname: top = iy
            if "s" in hname: bottom = iy
            self.crop_w = max(8, int(right - left))
            self.crop_h = max(8, int(bottom - top))
            self.crop_x = int(left)
            self.crop_y = int(top)
            self.crop_w_var.set(str(self.crop_w))
            self.crop_h_var.set(str(self.crop_h))
            self.clamp_crop()
            self.draw_crop_box()
            self.update_preview()

    def on_mouse_up(self, event):
        self.is_dragging = False

    def update_crop_position(self, new_x, new_y):
        self.crop_x = int(new_x)
        self.crop_y = int(new_y)
        self.clamp_crop()
        self.draw_crop_box()
        self.update_preview()

    def clamp_crop(self):
        if self.working_image is None:
            return
        img = self.working_image
        self.crop_w = min(self.crop_w, img.width)
        self.crop_h = min(self.crop_h, img.height)
        self.crop_x = max(0, min(self.crop_x, img.width - self.crop_w))
        self.crop_y = max(0, min(self.crop_y, img.height - self.crop_h))

    def apply_crop_size(self):
        if self.working_image is None:
            messagebox.showwarning("提示", "请先打开图片")
            return
        try:
            w = int(self.crop_w_var.get())
            h = int(self.crop_h_var.get())
            if w <= 0 or h <= 0:
                raise ValueError
        except ValueError:
            messagebox.showerror("错误", "宽高必须是正整数")
            return
        self.crop_w, self.crop_h = w, h
        cx = self.crop_x + self.crop_w / 2
        cy = self.crop_y + self.crop_h / 2
        self.crop_x = int(cx - w / 2)
        self.crop_y = int(cy - h / 2)
        self.clamp_crop()
        self.draw_crop_box()
        self.update_preview()

    def center_crop(self):
        if self.working_image is None:
            return
        self.crop_x = max(0, (self.working_image.width - self.crop_w) // 2)
        self.crop_y = max(0, (self.working_image.height - self.crop_h) // 2)
        self.clamp_crop()
        self.draw_crop_box()
        self.update_preview()

    # ============ 预览 ============
    def update_preview(self):
        if self.working_image is None:
            return
        self.clamp_crop()
        cropped = self.working_image.crop(
            (self.crop_x, self.crop_y,
             self.crop_x + self.crop_w, self.crop_y + self.crop_h))
        self.preview_image = cropped
        self.status_var.set(
            f"工作图 {self.working_image.width}x{self.working_image.height} | "
            f"裁剪 ({self.crop_x},{self.crop_y}) {self.crop_w}x{self.crop_h}")
        pc = self.preview_canvas
        cw = pc.winfo_width() or 260
        ch = pc.winfo_height() or 260
        scale = min(cw / cropped.width, ch / cropped.height, 1.0)
        disp_w = max(1, int(cropped.width * scale))
        disp_h = max(1, int(cropped.height * scale))
        disp = cropped.resize((disp_w, disp_h), Image.LANCZOS)
        self.preview_photo = ImageTk.PhotoImage(disp)
        pc.delete("all")
        pc.create_image(cw//2, ch//2, anchor=tk.CENTER, image=self.preview_photo)

    # ============ 生成代码 ============
    def generate_array(self):
        if self.preview_image is None:
            messagebox.showwarning("提示", "请先打开图片并选择裁剪区域")
            return

        img = self.preview_image
        w, h = img.size
        byte_order = self.byte_order.get()
        big_endian = (byte_order == "big")
        fmt = self.output_format.get()

        lines = []
        lines.append(f"// 原始图片: {self.original_image.width}x{self.original_image.height}")
        lines.append(f"// 预处理缩放后: {self.working_image.width}x{self.working_image.height}")
        lines.append(f"// 裁剪输出: {w}x{h}")
        lines.append("")

        # =====================================================
        # 分支 A：索引色格式
        # =====================================================
        if fmt.startswith("INDEXED"):
            match = re.search(r'INDEXED_(\d+)BIT', fmt)
            if not match:
                messagebox.showerror("错误", f"无法解析索引色格式: {fmt}")
                return
            bits = int(match.group(1))

            data_bytes, n_colors = image_to_indexed(img, bits)
            palette_size = n_colors * 4
            raw_size = w * h * 2
            ratio = (1 - len(data_bytes) / raw_size) * 100 if raw_size else 0

            lines.append(f"// 格式: INDEXED_{bits}BIT ({n_colors} 色)")
            lines.append(f"// 调色板: {palette_size} 字节 ({n_colors} 项 × 4 字节 BGRA)")
            lines.append(f"// 像素数据: {len(data_bytes) - palette_size} 字节")
            lines.append(f"// 数据总大小: {len(data_bytes)} 字节")
            lines.append(f"// 对比 RGB565 未压缩 ({raw_size} 字节) 节省: {ratio:.1f}%")
            lines.append("")
            lines.append('#include "lvgl.h"')
            lines.append("")
            lines.append(f"static const uint8_t indexed_map[{len(data_bytes)}] = {{")
            for i in range(0, len(data_bytes), 16):
                chunk = data_bytes[i:i+16]
                lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
            lines.append("};")
            lines.append("")
            lines.append("const lv_img_dsc_t my_image = {")
            lines.append(f"    .header.cf = LV_IMG_CF_INDEXED_{bits}BIT,")
            lines.append("    .header.always_zero = 0,")
            lines.append(f"    .header.w = {w},")
            lines.append(f"    .header.h = {h},")
            lines.append(f"    .data_size = {len(data_bytes)},")
            lines.append("    .data = indexed_map,")
            lines.append("};")

        # =====================================================
        # 分支 B：RGB565 格式
        # =====================================================
        else:
            pixels_rgb = img.load()
            vals = []
            for y in range(h):
                for x in range(w):
                    r, g, b = pixels_rgb[x, y]
                    r5 = (r >> 3) & 0x1F
                    g6 = (g >> 2) & 0x3F
                    b5 = (b >> 3) & 0x1F
                    vals.append((r5 << 11) | (g6 << 5) | b5)

            raw_size = len(vals) * 2
            lines.append(f"// 格式: RGB565, {byte_order}端字节序")
            lines.append(f"// 数据大小: {raw_size} 字节")
            lines.append("")

            if fmt == "RGB565 uint8_t 字节数组":
                bytes_list = []
                for v in vals:
                    if big_endian:
                        bytes_list += [(v >> 8) & 0xFF, v & 0xFF]
                    else:
                        bytes_list += [v & 0xFF, (v >> 8) & 0xFF]
                lines.append(f"const unsigned char image_data[{len(bytes_list)}] = {{")
                for i in range(0, len(bytes_list), 16):
                    chunk = bytes_list[i:i+16]
                    lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
                lines.append("};")

            elif fmt == "RGB565 uint16_t 数组":
                vals_out = vals
                if big_endian:
                    vals_out = [((v & 0xFF) << 8) | ((v >> 8) & 0xFF) for v in vals]
                lines.append(f"const uint16_t image_data[{len(vals_out)}] = {{")
                for i in range(0, len(vals_out), 8):
                    chunk = vals_out[i:i+8]
                    lines.append("    " + ", ".join(f"0x{v:04X}" for v in chunk) + ",")
                lines.append("};")

            elif fmt == "RGB565 LVGL lv_img_dsc_t":
                lvgl_vals = vals
                if big_endian:
                    lvgl_vals = [((v & 0xFF) << 8) | ((v >> 8) & 0xFF) for v in vals]
                bytes_list = []
                for v in lvgl_vals:
                    bytes_list += [v & 0xFF, (v >> 8) & 0xFF]
                lines.append('#include "lvgl.h"')
                lines.append("")
                lines.append(f"static const uint8_t image_map[{len(bytes_list)}] = {{")
                for i in range(0, len(bytes_list), 16):
                    chunk = bytes_list[i:i+16]
                    lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
                lines.append("};")
                lines.append("")
                lines.append("const lv_img_dsc_t my_image = {")
                lines.append("    .header.cf = LV_IMG_CF_TRUE_COLOR,")
                lines.append("    .header.always_zero = 0,")
                lines.append(f"    .header.w = {w},")
                lines.append(f"    .header.h = {h},")
                lines.append(f"    .data_size = {len(bytes_list)},")
                lines.append("    .data = image_map,")
                lines.append("};")

        full_text = "\n".join(lines)

        display_text = full_text
        if len(full_text) > 100000:
            display_text = full_text[:100000] + "\n\n... (输出过长，已截断，完整内容请保存到文件) ..."
        self.text_output.delete(1.0, tk.END)
        self.text_output.insert(tk.END, display_text)

        # 更新状态栏
        if fmt.startswith("INDEXED"):
            self.status_var.set(
                f"索引色输出 | 总大小约 {len(data_bytes)} 字节 "
                f"| 相比 RGB565 节省 {ratio:.1f}%"
            )
        else:
            self.status_var.set(f"RGB565 输出 | 大小 {raw_size} 字节")

        if messagebox.askyesno("保存", "是否保存完整代码到文件？"):
            path = filedialog.asksaveasfilename(
                defaultextension=".h",
                filetypes=[("C 头文件", "*.h"), ("C 源文件", "*.c"), ("所有文件", "*.*")])
            if path:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(full_text)
                self.status_var.set(f"已保存到 {path}")
                messagebox.showinfo("完成", f"代码已保存到:\n{path}")


if __name__ == "__main__":
    root = tk.Tk()
    app = ImageCropIndexedApp(root)
    root.mainloop()