# IndustryLibDemo — 工业视觉软件分层框架最小 Demo

一个可运行的架构演示：**分层、库化、条件编译、UI 可替换**。
不引入 OpenCV 等重依赖，算法用假数据模拟；Linux / Windows 同一套源码。

---

## 一、架构

```
                        ┌──────────────────────────────┐
                        │   UI 层（可替换 / 可缺席）    │
                        │  ui_python.py    web/index   │
                        └──────────────┬───────────────┘
                                       │  TCP + JSON  127.0.0.1:9500
                                       │  （UI 随时接入/断开，核心不受影响）
╔══════════════════════════════════════╪═══════════════════════════════════╗
║  core（C）                           │                                   ║
║                          ┌───────────┴────────────┐                      ║
║                          │   main.c  主程序 exe   │                      ║
║                          │  装配 / 主循环 / 命令  │                      ║
║                          └───────────┬────────────┘                      ║
║                                      │ 直接编入（条件编译）              ║
║   ┌──────────────────────────────────┴───────────────────────────────┐   ║
║   │ flow/  业务流程层        每个流程 = {name, run} → 一行结果 JSON  │   ║
║   │   flow_common.c  [always]   score>=140 判 OK                     │   ║
║   │   flow_customer_a.c [-DFLOW_A=ON]  阈值>=150，多输出 A/B/C 等级  │   ║
║   │   flow_customer_b.c [-DFLOW_B=ON]  区间 130~170，字段结构不同    │   ║
║   └──────────────────────────────────┬───────────────────────────────┘   ║
║                                      │ 只能向下调用                      ║
║   ┌──────────────────────────────────┴───────────────────────────────┐   ║
║   │ algo/  算法模块层  →  libalgo.so / algo.dll                      │   ║
║   │   detect.c   自研检测：组合 base 原语算出 score                  │   ║
║   └──────────────────────────────────┬───────────────────────────────┘   ║
║                                      │ 只能向下调用                      ║
║   ┌──────────────────────────────────┴───────────────────────────────┐   ║
║   │ base/  基础库层  →  libbase.so / base.dll                        │   ║
║   │   imgproc.c  基础算法原语(mean/min/max/stddev) ← OpenCV 的位置   │   ║
║   │   plc_sim.c  模拟数据源 ← 相机 SDK / PLC 的位置                  │   ║
║   │   net.c      TCP 服务端      log.c  日志                         │   ║
║   └──────────────────────────────────────────────────────────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════╝

依赖方向：flow ──▶ algo ──▶ base        反向调用一律禁止
```

## 二、每层职责与库产物

| 层 | 职责 | 产物 | 为什么这样切 |
|---|---|---|---|
| `base/` | 通用能力：日志、TCP、数据源、图像算法原语。**不含任何业务概念** | **共享库** `libbase.so` / `base.dll` | 与项目无关，最稳定。所有产品线共用一份，升级不需要动上层 |
| `algo/` | 自研检测算法：组合 base 原语算出 score。**不知道什么是"良品"** | **共享库** `libalgo.so` / `algo.dll` | 算法迭代最频繁，独立成库后可单独替换 / 灰度 / 保密交付 |
| `flow/` | 客户业务判定：阈值、等级、输出字段 | **直接编入 `core` exe** | 见下 |
| `main.c` | 装配三层、主循环、命令处理 | `core` exe | 唯一知道全貌的地方 |

**业务流程为什么编进 exe，而不做成库？**

1. **交付隔离**：给客户A的包里，客户B的判定逻辑（阈值、等级规则）连二进制都不存在 —— 这是商业机密边界，做成动态库反而能被拷走、被反编译、被误加载。
2. **无运行时开销与不确定性**：流程数量在编译期就确定，没有插件扫描、没有加载失败的分支。
3. **流程本来就"薄"**：它只是几行判定 + 拼 JSON，真正的重量在 algo/base 里 —— 那两层才值得付动态库的成本（独立升级、多产品共用）。

CI 里有一步专门验证这点：A 版二进制中 `grep customer_b` 必须查不到。

## 三、编译

```bash
cmake -B build-full -DFLOW_A=ON -DFLOW_B=ON
cmake --build build-full --config Release
```

产物都在 `build-*/bin/`（MSVC 下为 `build-*/bin/Release/`）：

| | Linux | Windows |
|---|---|---|
| 基础库 | `libbase.so` | `base.dll` (+ `base.lib`) |
| 算法库 | `libalgo.so` | `algo.dll` (+ `algo.lib`) |
| 主程序 | `core` | `core.exe` |

同一套源码，产物分别是 `.so` 和 `.dll`：跨平台差异全部收敛在 `base/net.c` 的几个
`#ifdef _WIN32` 里，`algo` / `flow` / `main` 无一处平台判断。
Windows 下靠 `CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON` 自动导出符号，源码里不需要
`__declspec(dllexport)` 宏。

## 四、本地演示（本机不装编译器）

编译全部在 GitHub Actions runner 上完成，本地只负责"跑"和"看"。

```powershell
# 第一次 / 想更新产物：-Fetch 会从最近一次成功的 CI 下载 Windows 产物到 dist\
powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Fetch

# 之后想演示哪个版本就切哪个（不必重新下载）
powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow full   # 3 个流程
powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow a      # 2 个流程

powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Stop        # 收摊
```

脚本做三件事：起 `core.exe`（独立窗口，能看到日志刷）、起 `bridge.py`、开浏览器
到 `http://127.0.0.1:8080`。想再加一个 UI 就另开一个终端 `python ui\ui_python.py`，
两个 UI 会同时连同一个 core。

`dist\` 已在 `.gitignore` 里，不进仓库。

### 演示"库是真的库"

```powershell
# core.exe 的导入表里有我们自己的两个库
powershell -File tools\show_deps.ps1
```
```
A版        依赖: algo.dll, base.dll     含 customer_b: False
全功能版   依赖: algo.dll, base.dll     含 customer_b: True
```
把 `base.dll` 改个名再双击 `core.exe`，Windows 会直接报 **STATUS_DLL_NOT_FOUND
(0xC0000135)** 起不来 —— 说明 base 是运行期真的被加载的动态库，不是编译期塞进 exe 的。
而 `customer_b` 那一行则说明：关掉的客户流程连字符串都不在二进制里。

## 五、演示步骤（源码 / 编译角度）

### 1. 只开客户A：编入 2 个流程

```bash
cmake -B build-a -DFLOW_A=ON && cmake --build build-a
./build-a/bin/core
```
```
[10:22:31][main] IndustryLibDemo core   build=common+A     ← BUILD_TAG
[10:22:31][main] flows compiled in: 2
[10:22:31][main]   [1] common
[10:22:31][main]   [2] customer_a
```

### 2. 全功能：编入 3 个流程

```bash
cmake -B build-full -DFLOW_A=ON -DFLOW_B=ON && cmake --build build-full
./build-full/bin/core
```
```
[10:23:02][main] IndustryLibDemo core   build=common+A+B
[10:23:02][main] flows compiled in: 3
```
同一帧数据，三个流程各判各的（注意 B 的输出字段和 A 完全不同）：
```
[10:23:03][flow] {"type":"result","flow":"common","frame":1,"score":151,"verdict":"OK","threshold":140}
[10:23:03][flow] {"type":"result","flow":"customer_a","frame":1,"score":151,"verdict":"OK","threshold":150,"grade":"B"}
[10:23:03][flow] {"type":"result","flow":"customer_b","frame":1,"score":151,"verdict":"OK","band":"in_range","low":130,"high":170,"noise":11}
```

### 3. core 独立运行（不开任何 UI）

直接运行 `core` 即可 —— 它自己产帧、自己判定、自己打印，**一个 UI 都不需要**。
UI 只是 TCP 上的一个可选观察者。这是"UI 可替换"的前提：核心根本不知道 UI 存在。

### 4. 两个 UI 轮流 / 同时连同一个 core

```bash
# 终端1：核心（先跑起来，一直不动）
./build-full/bin/core

# 终端2：tkinter UI
python ui/ui_python.py

# 终端3：Web UI，浏览器打开 http://127.0.0.1:8080
python ui/web/bridge.py
```

两个 UI 可以**同时**连着同一个 core：同一份结果流，两边同步刷新；
在任意一边点"停止"，另一边的数据流也停 —— 因为状态在核心里，不在 UI 里。
随便关掉哪个 UI，core 照常运行；再打开又能接回来。

`ui/web/bridge.py` 用标准库 `http.server` 把核心的 TCP 转成 HTTP+SSE，
顺带伺服 `index.html`，`POST /cmd` 转发指令 —— 不用 websocket，无需装任何依赖。

## 六、流程：现有哪些、怎么切、怎么加

### 现有 3 个流程

| 流程 | 开关 | 判定规则 | 输出字段 |
|---|---|---|---|
| `common` | 无（**始终编入**） | `score >= 140` → OK | `threshold` |
| `customer_a` | `-DFLOW_A=ON` | `score >= 150` → OK（更严） | `threshold` + `grade` (A/B/C) |
| `customer_b` | `-DFLOW_B=ON` | `130 <= score <= 170` → OK（区间） | `band`(under/in_range/over) + `low`/`high`/`noise` |

三个流程吃**同一帧、同一个 score**，各判各的。所以会出现同一帧结论不同——这正是要演示的：
```
frame 1  score=130   common: NG    customer_a: NG    customer_b: OK(in_range)
frame 2  score=175   common: OK    customer_a: OK    customer_b: NG(over)
```
注意 `customer_b` 的输出字段和 A 完全不是一套：**流程之间不必共享输出结构**，
因为它们本来就是不同客户的验收标准。

### 切流程 = 改 cmake 开关，重新编

```bash
cmake -B build-a    -DFLOW_A=ON              # common+A    2 个流程
cmake -B build-full -DFLOW_A=ON -DFLOW_B=ON  # common+A+B  3 个流程
cmake -B build-min                           # common      1 个流程
```
不是配置文件、不是命令行参数、不是插件目录 —— **是编译期决定的**。
交付给客户A的 exe 里，客户B的规则根本不存在（`tools\show_deps.ps1` 可当场验证）。

本地演示切换不用重编，两个版本 CI 都编好了：`tools\demo.ps1 -Flow a` / `-Flow full`。

### 加一个新流程（以客户C为例）—— 4 处改动

1. **写流程**：新建 `core/flow/flow_customer_c.c`，照抄现有那 20 行的结构
   ```c
   static void run(int frame_id, const detect_result_t *r, char *json, size_t n)
   { snprintf(json, n, "{...你的判定...}"); }
   const flow_t FLOW_CUSTOMER_C = {"customer_c", run};
   ```
2. **声明**：`core/flow/flow.h` 里加
   ```c
   #ifdef FLOW_C
   extern const flow_t FLOW_CUSTOMER_C;
   #endif
   ```
3. **注册**：`core/main.c` 的 `g_flows[]` 数组里加
   ```c
   #ifdef FLOW_C
       &FLOW_CUSTOMER_C,
   #endif
   ```
4. **开关**：根 `CMakeLists.txt` 里加 `option(FLOW_C ...)`，以及跟 A/B 一样的
   `list(APPEND FLOW_SOURCES ...)` / `BUILD_TAG` / `target_compile_definitions` 三行。

**base 和 algo 一行都不用动**，也不用重新编库 —— 新客户的规则只是在既有算法之上
换个判定口径。这就是分层的收益：改动被摁在最上面那一层里。

代价是加流程要碰 4 个文件（而不是丢一个插件 .so 进目录）。这是刻意的取舍：
换来的是"关掉的客户代码物理上不在交付物里"，对多客户分版交付的场景，这个保证比少改
两行有价值。真到了流程几十个的规模，再把第 2、3 步用一张 X-macro 表收敛成一处即可。

## 七、TCP 协议

连接 `127.0.0.1:9500`，**行分隔 JSON**。

发送（纯文本行）：`start` / `stop` / `status`

接收：
```jsonc
{"type":"result","flow":"common","frame":12,"score":151,"verdict":"OK","threshold":140}
{"type":"ack","cmd":"start","running":true}
{"type":"status","running":true,"flow_count":3,"build":"common+A+B"}
```

## 八、CI

`.github/workflows/build.yml` 在 Linux 与 Windows runner 上各构建 A 版和全功能版，
并跑 `tools/verify_tcp.py` 验证：结果流、`start`/`stop`/`status`、BUILD_TAG、
流程数、UI 断开重连、以及"关掉的流程不进二进制"。
