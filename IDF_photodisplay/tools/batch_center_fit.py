#!/usr/bin/env python3
"""
batch_center_crop.py
Usage:
    python IDF_photodisplay/tools/batch_center_fit.py  data/photodisplay/raw  data/photodisplay/prod
"""

import os
import sys
from glob import iglob
from PIL import Image

# ----------------- 参数 -----------------
SRC_EXT = ("*.jpg", "*.jpeg")          # 可扩展更多
TARGET_W, TARGET_H = 368, 448          # 目标画框
QUALITY = 95                           # 输出 JPEG 质量
META_NAME = "meta.txt"                 # 新增：meta 文件名
# ---------------------------------------

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


def process_folder(src_dir: str, dst_dir: str):
    os.makedirs(dst_dir, exist_ok=True)

    # 收集所有图片
    src_imgs = []
    for ext in SRC_EXT:
        src_imgs.extend(iglob(os.path.join(src_dir, "**", ext), recursive=True))
        src_imgs.extend(iglob(os.path.join(src_dir, "**", ext.upper()), recursive=True))

    if not src_imgs:
        print("未找到任何 jpg/jpeg 图片")
        return

    meta_list = []
    for idx, img_path in enumerate(src_imgs, 1):
        try:
            with Image.open(img_path) as im:
                im = best_fit_368_448(im)

                # 保存
                rel_path = os.path.relpath(img_path, src_dir)
                save_path = os.path.join(dst_dir, rel_path)
                os.makedirs(os.path.dirname(save_path), exist_ok=True)
                im.save(save_path, "JPEG", quality=QUALITY)

                meta_list.append(os.path.normpath(rel_path))
                print(f"[{idx:>4}/{len(src_imgs)}]  {img_path}  ->  {save_path}")

        except Exception as e:
            print(f"!! 处理失败：{img_path}  原因：{e}", file=sys.stderr)

    # 写 meta
    meta_path = os.path.join(dst_dir, META_NAME)
    with open(meta_path, "w", encoding="utf-8") as f:
        f.write("\n".join(meta_list))
    print(f"\nMeta 文件已生成：{meta_path}  共 {len(meta_list)} 条")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("用法: python batch_best_fit.py  <源文件夹>  <目标文件夹>")
        sys.exit(1)
    process_folder(sys.argv[1], sys.argv[2])
