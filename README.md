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
- 工程不写死本机路径：SDK / 工具链位置一律走 CCS 变量（`${COM_TI_C2000WARE_*}`、`${CG_TOOL_ROOT}`、
  `${workspace_loc:${ProjName}}`），跨机器或队友克隆后 `.cproject` / `makefile.targets` 都无需手改。
  前提是本机已装好下列组件（版本需与工程一致），CCS 会自动解析这些变量：
  C2000Ware `26.01.00.00` + SysConfig `1.28.1` + TI C2000 CGT `22.6.3.LTS`
