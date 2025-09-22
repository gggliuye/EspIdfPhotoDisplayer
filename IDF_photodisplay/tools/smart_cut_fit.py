#!/usr/bin/env python3
"""
smart_crop_fit.py
Usage:
    python IDF_photodisplay/tools/smart_cut_fit.py  data/photodisplay/raw  data/photodisplay/prod
"""

import os
import sys
from glob import iglob
from PIL import Image, ImageDraw
from ultralytics import YOLO
import pyzipper
from pillow_heif import HeifImagePlugin   # 注册 HEIC 解码
import tempfile
import shutil
import numpy as np

# ----------------- 参数 -----------------
SRC_EXT = ("*.jpg", "*.jpeg", "*.png")
TARGET_W, TARGET_H = 368, 448
CROP_W, CROP_H = 368, 448
QUALITY = 95
META_NAME = "meta.txt"
DRAW_DEBUG = False


def livp_to_pil(livp_path: str) -> Image.Image:
    """
    把 .livp 文件 -> PIL.Image（取 HEIC 关键帧）
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        with pyzipper.AESZipFile(livp_path) as zf:
            zf.extractall(tmpdir)
        # 找 HEIC
        heic_name = [f for f in os.listdir(tmpdir) if f.lower().endswith('.heic')]
        if not heic_name:
            raise RuntimeError("livp 内未找到 HEIC")
        heic_path = os.path.join(tmpdir, heic_name[0])
        return Image.open(heic_path)


def draw_debug(pil_im, boxes_norm, win_xyxy, src_path, debug_dir):
    """
    画 debug 图：红框=人物安全区，绿框=最终裁剪窗口
    boxes_norm: list[(x1,y1,x2,y2)] 归一化 0~1
    win_xyxy: (x0,y0,x1,y1) 像素坐标
    """
    os.makedirs(debug_dir, exist_ok=True)
    debug_im = pil_im.copy().convert("RGB")
    draw = ImageDraw.Draw(debug_im)
    w, h = pil_im.size

    # 画人物 bbox（红）
    for x1n, y1n, x2n, y2n in boxes_norm:
        draw.rectangle([x1n*w, y1n*h, x2n*w, y2n*h], outline="red", width=3)

    # 画裁剪窗口（绿）
    draw.rectangle(win_xyxy, outline="lime", width=3)

    # 保存
    rel_name = os.path.relpath(src_path, SRC_DIR_ROOT)  # 后面主函数里给这个变量
    save_path = os.path.join(debug_dir, rel_name)
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    debug_im.save(save_path)
    print(f"[debug] 已画框：{save_path}")


# ----- 全局加载一次模型 -----
yolo = YOLO("yolov8n.pt")      # 最小最快；要精度可换 yolov8m.pt / yolov8x.pt

def detect_person_bboxes(pil_im: Image.Image):
    """
    用 YOLOv8 检测所有人，返回 list[(xmin,ymin,xmax,ymax)] **归一化 0~1**
    只保留 person 类别（COCO id=0）
    """
    # PIL -> numpy RGB
    img_rgb = np.array(pil_im.convert("RGB"))
    h, w = img_rgb.shape[:2]

    results = yolo(img_rgb, verbose=False)          # 返回 Results 列表
    boxes = []
    for r in results:
        for *xyxy, conf, cls in r.boxes.data.cpu().numpy():
            if conf > 0.6 and int(cls) == 0:                       # 0 就是 person
                x1, y1, x2, y2 = xyxy
                # 转归一化
                boxes.append((x1/w, y1/h, x2/w, y2/h))
    return boxes


def best_fit_368_448(im: Image.Image) -> Image.Image:
    """
    保持比例，整张图无裁剪地缩放到“最大且能完全放进 TARGET_W × TARGET_H”。
    不足区域留白（默认黑色，可改）。
    """
    im = im.convert("RGB")
    w, h = im.size

    # 计算“让整张图刚好放进目标框”的缩放系数
    scale = min(TARGET_W / w, TARGET_H / h)
    new_w, new_h = int(w * scale), int(h * scale)
    im = im.resize((new_w, new_h), Image.LANCZOS)

    # 居中粘贴到纯色画布上（默认黑底，可改）
    canvas = Image.new("RGB", (TARGET_W, TARGET_H), (0, 0, 0))
    paste_x = (TARGET_W - new_w) // 2
    paste_y = (TARGET_H - new_h) // 2
    canvas.paste(im, (paste_x, paste_y))
    return canvas



def safe_crop_and_fit(pil_im: Image.Image, src_path: str = ""):
    w, h = pil_im.size
    boxes_norm = detect_person_bboxes(pil_im)          # YOLO 返回归一化
    has_person = bool(boxes_norm)

    # 如果没人 → 中心裁剪
    if not has_person:
        return center_crop_368_448(pil_im)

    # ----- 1. 计算“必须保留”的像素级安全区 -----
    safe_x1 = min(int(box[0] * w) for box in boxes_norm)
    safe_y1 = min(int(box[1] * h) for box in boxes_norm)
    safe_x2 = max(int(box[2] * w) for box in boxes_norm)
    safe_y2 = max(int(box[3] * h) for box in boxes_norm)
    safe_cx = (safe_x1 + safe_x2) / 2
    safe_cy = (safe_y1 + safe_y2) / 2
    safe_w = safe_x2 - safe_x1
    safe_h = safe_y2 - safe_y1

    # ----- 2. 保持 368:448 比例，向外扩展“最小窗口” -----
    crop_ratio = TARGET_W / TARGET_H
    # 最小窗口：必须包住 safe 区域
    win_w = max(safe_w, safe_h * crop_ratio)
    win_h = win_w / crop_ratio
    # 留 5% 边距（可改）
    win_w *= 1.05
    win_h *= 1.05

    # 如果算出来比 368×448 还小，就保持该尺寸（后续会 upscale+pad）
    # 否则就按算出来的尺寸裁剪
    win_w = min(win_w, w)
    win_h = min(win_h, h)
    if win_h > h:
        win_h = h
        win_w = win_h * crop_ratio
    if win_w > w:
        win_w = w
        win_h = win_w / crop_ratio

    # 窗口居中（以安全区中心为锚点）
    x0 = max(0, min(safe_cx - win_w / 2, w - win_w))
    y0 = max(0, min(safe_cy - win_h / 2, h - win_h))
    x1, y1 = x0 + win_w, y0 + win_h

    # ----- 3. debug 图 -----
    if DRAW_DEBUG:
        debug_root = DST_DIR_ROOT + "_debug"
        draw_debug(pil_im, boxes_norm, (x0, y0, x1, y1), src_path, debug_root)

    # ----- 4. 裁剪 + 直接 resize 到 368×448（人物完整，可能出现黑边） -----
    im = pil_im.crop((int(x0), int(y0), int(x1), int(y1)))
    # im = im.resize((TARGET_W, TARGET_H), Image.LANCZOS)
    # return im
    return best_fit_368_448(im)


def center_crop_368_448(im: Image.Image) -> Image.Image:
    """原中心裁剪逻辑"""
    im = im.convert("RGB")  # 去掉透明通道等
    w, h = im.size

    # 计算缩放比例：让最短边恰好覆盖目标尺寸
    scale = max(CROP_W / w, CROP_H / h)
    new_w, new_h = int(w * scale), int(h * scale)
    im = im.resize((new_w, new_h), Image.LANCZOS)

    # 中心裁剪
    left = (new_w - CROP_W) // 2
    top = (new_h - CROP_H) // 2
    im = im.crop((left, top, left + CROP_W, top + CROP_H))

    return im


def open_any(path: str) -> Image.Image:
    if path.lower().endswith('.livp'):
        return livp_to_pil(path)
    return Image.open(path)



def process_folder(src_dir: str, dst_dir: str):
    os.makedirs(dst_dir, exist_ok=True)


    global SRC_DIR_ROOT, DST_DIR_ROOT
    SRC_DIR_ROOT = src_dir
    DST_DIR_ROOT = dst_dir

    src_imgs = []
    for ext in SRC_EXT:
        src_imgs.extend(iglob(os.path.join(src_dir, "**", ext), recursive=True))
        src_imgs.extend(iglob(os.path.join(src_dir, "**", ext.upper()), recursive=True))

    src_imgs.extend(iglob(os.path.join(src_dir, "**", "*.livp"), recursive=True))
    src_imgs.extend(iglob(os.path.join(src_dir, "**", "*.LIVP"), recursive=True))

    if not src_imgs:
        print("未找到任何 jpg/jpeg 图片")
        return

    meta_list = []
    for idx, img_path in enumerate(src_imgs, 1):
        try:
            with open_any(img_path) as im:
                im = safe_crop_and_fit(im, img_path.replace(".livp", ".jpg"))

                rel_path = os.path.relpath(img_path, src_dir).replace(" ", "_")
                save_path = os.path.join(dst_dir, rel_path)
                os.makedirs(os.path.dirname(save_path), exist_ok=True)
                im.save(save_path, "JPEG", quality=QUALITY)

                meta_list.append(os.path.normpath(rel_path))
                print(f"[{idx:>4}/{len(src_imgs)}]  {img_path}  ->  {save_path}")

        except Exception as e:
            print(f"!! 处理失败：{img_path}  原因：{e}", file=sys.stderr)

    meta_path = os.path.join(dst_dir, META_NAME)
    with open(meta_path, "w", encoding="utf-8") as f:
        f.write("\n".join(meta_list))
    print(f"\nMeta 文件已生成：{meta_path}  共 {len(meta_list)} 条")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("用法: python smart_crop_fit.py  <源文件夹>  <目标文件夹>")
        sys.exit(1)
    process_folder(sys.argv[1], sys.argv[2])
