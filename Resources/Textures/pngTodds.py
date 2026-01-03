import os
import numpy as np
import cv2
import imageio.v2 as imageio

# 启用 DDS 写入（需要 FreeImage 插件，首次会自动下载）
try:
    imageio.plugins.freeimage.download()
except Exception:
    pass

def to_three_channels(img):
    # 灰度 -> 3 通道
    if img.ndim == 2:
        return cv2.merge([img, img, img])
    # RGBA -> RGB
    if img.ndim == 3 and img.shape[-1] == 4:
        return img[..., :3]
    # 已是 RGB
    return img

def ensure_uint8(img):
    if img.dtype == np.uint8:
        return img
    if img.dtype == np.float32 or img.dtype == np.float64:
        img = np.clip(img, 0.0, 1.0)
        return (img * 255).astype(np.uint8)
    if img.dtype == np.uint16:
        # 映射到 8-bit
        return (img / 257).astype(np.uint8)
    return img.astype(np.uint8)

def main():
    base_dir = os.path.dirname(__file__)
    in_path = os.path.join(base_dir, "default_depth.png")
    out_path = os.path.join(base_dir, "default_depth.dds")

    if not os.path.exists(in_path):
        print(f"未找到文件: {in_path}")
        return

    # 读取 PNG（优先 imageio，失败则用 OpenCV）
    img = None
    try:
        img = imageio.imread(in_path)
    except Exception:
        img = cv2.imread(in_path, cv2.IMREAD_UNCHANGED)

    if img is None:
        print(f"无法读取: {in_path}")
        return

    img = to_three_channels(img)
    img = ensure_uint8(img)

    # 写入 DDS
    try:
        imageio.imwrite(out_path, img, format="DDS")
        print(f"已保存: {out_path}")
    except Exception as exc:
        print(f"DDS 保存失败: {exc}")

if __name__ == "__main__":
    main()