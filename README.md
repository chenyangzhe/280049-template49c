# 280049-template49c

基于 **TI TMS320F280049C**（C2000 / C28x）的逆变器控制工程，包含 VSG（虚拟同步发电机）控制、CLA 并行任务、OLED 显示与 VOFA+ 上位机波形输出。

## 开发环境

| 项目 | 内容 |
|---|---|
| 器件 | TMS320F280049C（C2000，小端，COFF） |
| IDE | Code Composer Studio 20.4.1（工程为 Eclipse-based 变体，工程文件格式 CCS 12.8） |
| 编译器 | TI C2000 CodeGen 22.6.3.LTS |
| 仿真器 | XDS110（`targetConfigs/TMS320F280049C.ccxml`） |
| 构建配置 | `RAM`（RAM 调试运行）、`FLASH_RUN`（Flash 运行） |

## 目录结构

| 目录 | 说明 |
|---|---|
| `App/src`、`App/inc` | 控制算法与应用层：VSG、SOGI、PR、PI、Hilbert、Notch_Filter、Modulator、RMS_algorithm、AdcCapture、CLA 任务（`cla_control.cla`）、OLED、VOFA+ |
| `Bsp` | 板级支持与 SysConfig 配置（`untitled.syscfg`、`board.c/h`） |
| `Cmd` | 链接命令文件（RAM / Flash / CLA 版本） |
| `Device` | 器件启动代码与寄存器头文件 |
| `DCL` | TI Digital Controller Library（PI / PID / DF 等） |
| `Lib/FPU32` | C28x FPU 快速数学库（sin、cos、sqrt、atan…） |
| `C2000Ware` | C2000Ware 头文件副本 |
| `tools`、`targetConfigs` | 辅助工具与目标板配置 |
| `RAM`、`FLASH_RUN` | 编译输出目录（已在 `.gitignore` 中忽略，不入库） |

## 编译与烧录

1. CCS 中 `File → Open Folder` 打开本目录
2. 选择 `RAM` 或 `FLASH_RUN` 构建配置后 Build
3. 连接 XDS110，下载调试或烧写 Flash

## 说明

- 编译产物、`.trae` / `.mcp` / `.claude` 等本地缓存与日志均不入库（见 `.gitignore`）
- `App/src/OLED.c`、`App/src/vofa*.c` 分别负责 OLED 显示与 VOFA+ 波形上传
- **`board.c` / `board.h` 在哪、怎么改**：唯一真源是 `Bsp/untitled.syscfg`（在 CCS 里双击打开 GUI
  配置引脚 / 外设）。CCS 是"构建步骤"式的，每次构建把它生成到构建目录 —— `RAM/syscfg/`、
  `FLASH_RUN/syscfg/`，**编译用的就是那一份**。`makefile.targets` 里加了一条镜像规则，每次构建会把
  `syscfg/board.c|h` 同步一份回 `Bsp/`，所以改完 `.syscfg` 重新构建后 `Bsp/board.c` 就是最新的，
  可以直接看，也能进 git diff。镜像文件已在 `.cproject` 中排除出构建，只是"镜子"、不参与编译，
  不会重复符号。**要改配置请改 `Bsp/untitled.syscfg`，手改 `Bsp/board.c` 会在下次构建被覆盖。**
- **跨机器 / 队友克隆时需要留意的唯一一处**：`.cproject` 里有 9 条写死的安装路径 —— 7 处指向
  `C2000Ware_26_01_00_00/.metadata/sdk.json`（SysConfig 的产品清单，RAM / FLASH_RUN / Debug 各配置合计 7 处），
  2 处指向 fastRTS / CGT 的库文件。C2000Ware 或编译器装在别的位置时，把这 9 条一并替换。
  **不要**把它们改成 `${COM_TI_C2000WARE_SYSCONFIG_MANIFEST}` 之类的写法：本工程实测该变量展开为空，
  会让 SysConfig 调用丢掉 `-s` 参数、`board.c/h` 不再生成。
- 其余路径都走 CCS 变量（`${workspace_loc:${ProjName}}`、`${CG_TOOL_ROOT}`），无需手改；
  `makefile.targets` 也已改为不写死路径。
- 需要安装的组件（版本需与工程一致）：C2000Ware `26.01.00.00` + SysConfig `1.28.1` + TI C2000 CGT `22.6.3.LTS`
