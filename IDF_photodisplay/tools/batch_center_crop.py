#!/usr/bin/env python3
"""
batch_center_crop.py
Usage:
    python IDF_photodisplay/tools/batch_center_crop.py  data/photodisplay/raw  data/photodisplay/prod
"""

import os
import sys
from glob import iglob
from PIL import Image

# ----------------- 参数 -----------------
SRC_EXT = ("*.jpg", "*.jpeg")          # 可扩展更多
CROP_W, CROP_H = 368, 448              # 目标尺寸
QUALITY = 95                           # 输出 JPEG 质量
META_NAME = "meta.txt"                 # 新增：meta 文件名
# ---------------------------------------

def center_crop_368_448(src_dir: str, dst_dir: str):
    """保持比例缩放 + 中心裁剪并保存"""
    os.makedirs(dst_dir, exist_ok=True)

    # 生成所有图片路径（不区分大小写）
    src_imgs = []
    for ext in SRC_EXT:
        src_imgs.extend(iglob(os.path.join(src_dir, "**", ext), recursive=True))
        src_imgs.extend(iglob(os.path.join(src_dir, "**", ext.upper()), recursive=True))

    if not src_imgs:
        print("未找到任何 jpg/jpeg 图片，请检查路径或扩展名")
        return


    meta_list = []                       # 新增：收集相对路径

    for idx, img_path in enumerate(src_imgs, 1):
        try:
            with Image.open(img_path) as im:
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

                # 保存
                rel_path = os.path.relpath(img_path, src_dir)
                save_path = os.path.join(dst_dir, rel_path)
                os.makedirs(os.path.dirname(save_path), exist_ok=True)
                im.save(save_path, "JPEG", quality=QUALITY)

                # 收集相对路径（去掉最前面的 ./ 等）
                rel_path_for_meta = os.path.normpath(rel_path)
                meta_list.append(rel_path_for_meta)

                print(f"[{idx:>4}/{len(src_imgs)}]  {img_path}  ->  {save_path}")

        except Exception as e:
            print(f"!! 处理失败：{img_path}  原因：{e}", file=sys.stderr)

    # 新增：写入 meta.txt
    meta_path = os.path.join(dst_dir, META_NAME)
    with open(meta_path, "w", encoding="utf-8") as f:
        f.write("\n".join(meta_list))
    print(f"\nMeta 文件已生成：{meta_path}  共 {len(meta_list)} 条")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("用法: python batch_center_crop.py  <源文件夹>  <目标文件夹>")
        sys.exit(1)

    src_folder, dst_folder = sys.argv[1], sys.argv[2]
    center_crop_368_448(src_folder, dst_folder)
