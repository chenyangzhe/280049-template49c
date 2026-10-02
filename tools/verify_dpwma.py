"""
DPWMA 算法离线验证脚本
1:1 复制 DPWMA.c 中的 C 代码逻辑，Python 复现
生成 400 点三相正弦 → DPWMA → CSV + 统计
"""

import math
import csv

# ============ 参数（与 C2000 完全一致） ============
POINTS    = 400        # 载波比 20kHz/50Hz
TBPRD     = 2500       # ePWM 周期
M         = 0.8        # 调制度
DEAD_MIN  = 5
DEAD_MAX  = 2495

# ============ 1:1 移植 C 代码 ============

def dpwma_z(ua, ub, uc):
    """DPWMA_Z — 零序分量计算（与 DPWMA.c 第21行完全一致）"""
    # A 相
    if ua > 0.0:
        a1 = 1.0001 - ua
        a2 = ua
    else:
        a1 = -ua
        a2 = 1.0001 + ua

    # B 相
    if ub > 0.0:
        b1 = 1.0001 - ub
        b2 = ub
    else:
        b1 = -ub
        b2 = 1.0001 + ub

    # C 相
    if uc > 0.0:
        c1 = 1.0001 - uc
        c2 = uc
    else:
        c1 = -uc
        c2 = 1.0001 + uc

    # z1 = min(a1, b1, c1)
    z1 = a1
    if b1 < z1: z1 = b1
    if c1 < z1: z1 = c1

    # z2 = min(a2, b2, c2)
    z2 = a2
    if b2 < z2: z2 = b2
    if c2 < z2: z2 = c2

    # 扇区判断
    if z1 > z2:
        z = -z2
    else:
        z = z1

    return z

def dpwma_control(inp, zs, tbprd):
    """DPWMA_Control — 占空比生成（与 DPWMA.c 第91行完全一致）"""
    upper = tbprd + 1.0
    lower = 0.0

    duty = (inp + 1.0 + zs) * tbprd * 0.5

    if duty >= upper:
        duty = upper + 1.0
    elif duty <= lower:
        duty = lower

    return int(duty + 0.5)  # 四舍五入截断


# ============ 生成三相正弦 + DPWMA 计算 ============

results = []
clamp_count = {"A_up": 0, "A_dn": 0, "B_up": 0, "B_dn": 0, "C_up": 0, "C_dn": 0}

for i in range(POINTS):
    pu = i / POINTS  # 归一化相位 [0, 1)
    angle = pu * 2.0 * math.pi

    # 三相正弦（与 C2000 SIN_COS.c ISR 一致）
    sinA = math.sin(angle)
    sinB = math.sin(angle + 2.0/3.0 * math.pi)   # 滞后 120°
    sinC = math.sin(angle + 4.0/3.0 * math.pi)   # 滞后 240°

    # 调制波
    UA = M * sinA
    UB = M * sinB
    UC = M * sinC

    # DPWMA
    z  = dpwma_z(UA, UB, UC)
    dA = dpwma_control(UA, z, TBPRD)
    dB = dpwma_control(UB, z, TBPRD)
    dC = dpwma_control(UC, z, TBPRD)

    # 注入零序后的调制波（归一化为 [-1,1] 用于观察马鞍波）
    UA_dpwma = UA + z
    UB_dpwma = UB + z
    UC_dpwma = UC + z

    # 统计钳位
    if dA >= TBPRD + 1:  clamp_count["A_up"] += 1
    if dA <= 0:          clamp_count["A_dn"] += 1
    if dB >= TBPRD + 1:  clamp_count["B_up"] += 1
    if dB <= 0:          clamp_count["B_dn"] += 1
    if dC >= TBPRD + 1:  clamp_count["C_up"] += 1
    if dC <= 0:          clamp_count["C_dn"] += 1

    results.append({
        "idx": i,
        "deg": round(angle * 180.0 / math.pi, 1),
        "UA":  UA,  "UB":  UB,  "UC":  UC,
        "z":   z,
        "UA_z": UA_dpwma, "UB_z": UB_dpwma, "UC_z": UC_dpwma,
        "dA": dA, "dB": dB, "dC": dC,
    })


# ============ CSV 输出 ============

csv_path = "d:\\Ti\\code\\workspace\\template49c\\tools\\dpwma_result.csv"
with open(csv_path, "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["idx","deg","UA","UB","UC","z","UA+z","UB+z","UC+z","dA","dB","dC"])
    for r in results:
        w.writerow([
            r["idx"], r["deg"],
            round(r["UA"],6), round(r["UB"],6), round(r["UC"],6),
            round(r["z"],6),
            round(r["UA_z"],6), round(r["UB_z"],6), round(r["UC_z"],6),
            r["dA"], r["dB"], r["dC"]
        ])


# ============ 统计输出 ============

print("=" * 60)
print("  DPWMA 算法离线验证结果")
print("=" * 60)
print(f"  点数: {POINTS}  |  调制度 M: {M}  |  TBPRD: {TBPRD}")
print()

print("  [钳位统计] (duty >= TBPRD+1 = 上钳位, duty <= 0 = 下钳位)")
print(f"    A 相:  上钳 {clamp_count['A_up']:3d} 点  |  下钳 {clamp_count['A_dn']:3d} 点")
print(f"    B 相:  上钳 {clamp_count['B_up']:3d} 点  |  下钳 {clamp_count['B_dn']:3d} 点")
print(f"    C 相:  上钳 {clamp_count['C_up']:3d} 点  |  下钳 {clamp_count['C_dn']:3d} 点")
total_clamp = sum(clamp_count.values())
print(f"    总计钳位: {total_clamp} / {POINTS} 点  ({100*total_clamp/POINTS:.1f}%)")
print(f"    理论值:   理论上 DPWMA 每 60° 钳位一相 → 钳位占比约 33%~66%")
print()

# 检查过调制
overmod = [r for r in results if abs(r["UA_z"]) > 1.0 or abs(r["UB_z"]) > 1.0 or abs(r["UC_z"]) > 1.0]
if overmod:
    print(f"  ⚠ 过调制: {len(overmod)} 个点 (注入零序后幅值 > 1.0)")
    for r in overmod[:5]:
        print(f"     idx={r['idx']:3d} deg={r['deg']:6.1f}  UA+z={r['UA_z']:+.4f} UB+z={r['UB_z']:+.4f} UC+z={r['UC_z']:+.4f}")
else:
    print(f"  ✓ 无过调制 (注入零序后仍在 [-1, 1] 内)")

# 线电压检查
UA_UB = [r["UA_z"] - r["UB_z"] for r in results]
UA_UB_max = max(UA_UB)
UA_UB_min = min(UA_UB)
print()
print(f"  [线电压 Uab = UA+z - UB+z]")
print(f"    峰值: {UA_UB_max:.4f}  谷值: {UA_UB_min:.4f}  峰峰值: {UA_UB_max-UA_UB_min:.4f}")
print(f"    SPWM 线电压峰峰值理论: {M*math.sqrt(3):.4f}")
print(f"    DPWMA 线电压应 ≥ SPWM（利用率高约 15%）")

print()
print(f"  ✓ CSV 已保存: {csv_path}")
print("  用 Excel / VOFA / Python matplotlib 打开查看波形")
