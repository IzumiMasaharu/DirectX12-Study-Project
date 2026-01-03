import cv2
import numpy as np
import imageio.v2 as imageio
from scipy.sparse.linalg import cg
from scipy.sparse import diags
import os
 
def normal_to_depth(normal_map):
    """
    从法线图恢复深度图
    :param normal_map: (H, W, 3) 的 numpy 数组，范围在 [-1, 1] 之间
    :return: 归一化的深度图
    """
    h, w, _ = normal_map.shape
    nx, ny, nz = normal_map[..., 0], normal_map[..., 1], normal_map[..., 2]
 
    # 避免 nz 过小（防止除零）
    nz[nz == 0] = 1e-8  
 
    # 计算梯度
    dzdx = -nx / nz
    dzdy = -ny / nz

    # 计算梯度的散度到二维网格，避免展平后形状不一致
    div = np.zeros((h, w), dtype=np.float64)
    # 水平分量：右减左
    div[:, :-1] += dzdx[:, :-1]
    div[:, 1:] -= dzdx[:, :-1]
    # 垂直分量：下减上
    div[:-1, :] += dzdy[:-1, :]
    div[1:, :] -= dzdy[:-1, :]

    # 设定泊松方程的系数矩阵
    A = diags([-1, -1, 4, -1, -1], [-w, -1, 0, 1, w], shape=(h*w, h*w))
    b = div.flatten()
 
    # 解泊松方程
    depth, _ = cg(A, b)
 
    # 归一化到 [0, 1] 方便可视化
    depth = depth.reshape((h, w))
    depth = (depth - depth.min()) / (depth.max() - depth.min())
 
    return depth

def load_normal_map(filepath):
    """
    加载法线贴图（支持 DDS 和 PNG 格式）
    :param filepath: 文件路径
    :return: 归一化到 [-1, 1] 的法线图
    """
    img = cv2.imread(filepath, cv2.IMREAD_COLOR)

    if img is None:
        # OpenCV 的轮子通常不带 DDS，使用 imageio + freeimage 插件兜底
        try:
            img = imageio.imread(filepath)
        except Exception as exc:  # noqa: BLE001
            print(f"无法加载文件: {filepath}, 错误: {exc}")
            return None
    
    # BGR 转 RGB 并转换为 float32
    if img.shape[-1] == 4:
        img = img[..., :3]

    if img.shape[-1] == 3:
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB).astype(np.float32)
    else:
        print(f"不支持的通道数: {img.shape[-1]} in {filepath}")
        return None
    # 归一化到 [-1, 1]
    normal_map = img / 127.5 - 1
    return normal_map

# 处理两张法线贴图
normal_files = ["brick_normal.dds", "floor_normal.dds"]

for normal_file in normal_files:
    if os.path.exists(normal_file):
        print(f"处理: {normal_file}")
        normal_map = load_normal_map(normal_file)
        
        if normal_map is not None:
            depth_map = normal_to_depth(normal_map)
            
            # 生成输出文件名
            output_file = normal_file.replace("_normal", "_depth").replace(".dds", ".png")
            
            # 保存深度图
            cv2.imwrite(output_file, (depth_map * 255).astype(np.uint8))
            print(f"已保存: {output_file}")
            
            # 显示结果
            cv2.imshow(f"Depth Map - {normal_file}", (depth_map * 255).astype(np.uint8))
    else:
        print(f"文件不存在: {normal_file}")

cv2.waitKey(0)
cv2.destroyAllWindows()