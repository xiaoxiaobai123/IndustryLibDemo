# IndustryLibDemo — 工业视觉软件分层框架最小 Demo

一个可运行的架构演示：**分层、库化、条件编译、UI 可替换**。
不引入 OpenCV 等重依赖，算法用假数据模拟；Linux / Windows 同一套源码。

**核心命题**：客户A和客户B是**两个不同的项目**——不同硬件、不同视觉需求、
不同输出。但它们**共用同一套底层**，靠编译开关裁出各自的交付物。

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
║  core（C）               ┌───────────┴────────────┐                      ║
║                          │  main.c   宿主(exe)    │                      ║
║                          │ 起网络/主循环/转发命令 │                      ║
║                          │ 不认识算法，不认识相机 │                      ║
║                          └───────────┬────────────┘                      ║
║                                      │ 只调 flow->run()                  ║
║   ┌──────────────────────────────────┴───────────────────────────────┐   ║
║   │ flow/  业务流程层 = 一个流程就是一个客户项目（条件编译进 exe）   │   ║
║   │                                                                  │   ║
║   │   flow_customer_a.c  [-DFLOW_A=ON]  客户A：苹果外观检测项目      │   ║
║   │        取 CAM_A 的图 ──▶ 调 algo.detect_surface ──▶ 分 A/B/C 级  │   ║
║   │                                                                  │   ║
║   │   flow_customer_b.c  [-DFLOW_B=ON]  客户B：工件角度测量项目      │   ║
║   │        取 CAM_B 的图 ──▶ 调 algo.angle_measure  ──▶ ±15° 判 OK   │   ║
║   │                                                                  │   ║
║   │   flow_common.c      [always]       通用表面检测                 │   ║
║   │                                                                  │   ║
║   │   每个流程自己决定：用谁的硬件 / 调哪个算法 / 怎么判 / 输出什么  │   ║
║   └──────────────────────────────────┬───────────────────────────────┘   ║
║                                      │ 各取所需，只能向下调用            ║
║   ┌──────────────────────────────────┴───────────────────────────────┐   ║
║   │ algo/  算法模块层  →  libalgo.so / algo.dll                      │   ║
║   │   detect.c  表面质量评分 detect_surface()   ← 客户A用            │   ║
║   │   angle.c   角度测量     angle_measure()    ← 客户B用            │   ║
║   │   通用能力，不绑定任何客户。自研 = 在 base 原语上做二次开发      │   ║
║   └──────────────────────────────────┬───────────────────────────────┘   ║
║                                      │ 只能向下调用                      ║
║   ┌──────────────────────────────────┴───────────────────────────────┐   ║
║   │ base/  基础库层  →  libbase.so / base.dll                        │   ║
║   │   imgproc.c  基础算法原语(mean/min/max/stddev) ← OpenCV 的位置   │   ║
║   │   plc_sim.c  数据源：CAM_A 面阵 / CAM_B 线扫 ← 相机 SDK 的位置   │   ║
║   │   net.c      TCP 服务端      log.c  日志(含调用链 trace)         │   ║
║   └──────────────────────────────────────────────────────────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════╝

依赖方向：main ──▶ flow ──▶ algo ──▶ base        反向调用一律禁止
```

**为什么 main 只是"宿主"而不是流程**：如果让 main 去取图、调算法、再把结果喂给
各个流程，那 main 就成了唯一真正的流程，flow 只剩拼 JSON——而且客户A的机器上会
白算一遍客户B才需要的角度。所以取图和选算法必须下放到 flow：**谁的项目，谁做主**。

CI 里有一条断言守着这个：`main.c` 一旦 include 了 `detect.h` / `angle.h` /
`plc_sim.h`，构建直接失败。

## 二、每层职责与库产物

| 层 | 职责 | 产物 | 为什么这样切 |
|---|---|---|---|
| `base/` | 通用能力：日志、TCP、相机数据源、图像算法原语。**不含任何业务概念** | **共享库** `libbase.so` / `base.dll` | 与项目无关，最稳定。所有客户项目共用一份，升级不需要动上层 |
| `algo/` | 自研算法能力：表面评分、角度测量……**通用，不绑定任何客户**，也不知道什么叫"良品" | **共享库** `libalgo.so` / `algo.dll` | 算法迭代最频繁，独立成库后可单独替换 / 灰度 / 保密交付；新增能力不影响老客户 |
| `flow/` | **一个流程 = 一个客户项目**：选硬件、选算法、定判定、定输出 | **直接编入 `core` exe** | 见下 |
| `main.c` | 宿主：起网络、跑主循环、转发命令 | `core` exe | **不认识算法也不认识相机**——它只是个壳 |

**业务流程为什么编进 exe，而不做成库？**

1. **交付隔离**：给客户A的包里，客户B的项目代码连二进制都不存在 —— 这是商业机密边界，做成动态库反而能被拷走、被反编译、被误加载。
2. **无运行时开销与不确定性**：流程数量在编译期就确定，没有插件扫描、没有加载失败的分支。
3. **该重的地方才付库的成本**：算法和基础能力值得独立成库（多客户共用、可单独升级）；而客户项目是一客一份、互不复用的，做成库只有坏处。

CI 里有断言验证这点：客户A的二进制中 `grep customer_b` 必须查不到，反之亦然。

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

# 之后想演示哪个交付物就切哪个（不必重新下载，三个版本 CI 都编好了）
powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow a      # 客户A：苹果外观检测项目
powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow b      # 客户B：角度测量项目
powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow full   # 内部回归版：两个项目都有

powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Stop        # 收摊
```

> `-Fetch` 和 `-Flow` 是**两个独立参数**，不是三种模式：`-Fetch` 负责"下载"，
> `-Flow` 负责"跑哪个"，可以组合（`-Fetch -Flow b`）。不写 `-Flow` 默认 `full`。
> 第一次必须带 `-Fetch`，之后就不用了。

脚本做三件事：起 `core.exe`（独立窗口，能看到日志刷）、起 `bridge.py`、开浏览器
到 `http://127.0.0.1:8080`。想再加一个 UI 就另开一个终端 `python ui\ui_python.py`，
两个 UI 会同时连同一个 core。

`dist\` 已在 `.gitignore` 里，不进仓库。

### 演示"库是真的库 + 客户之间真的隔离"

```powershell
powershell -File tools\show_deps.ps1
```
```
交付物     运行时依赖的库           含客户A项目    含客户B项目
--------------------------------------------------------------------
客户A版    base.dll, algo.dll       True           False
客户B版    base.dll, algo.dll       False          True
全功能版   base.dll, algo.dll       True           True

库本身的依赖方向（反向依赖不存在才叫分层）：
  base.dll   -> （不依赖我们任何库）
  algo.dll   -> base.dll
```

三件事一次说清：

- **库是真的**：把 `base.dll` 改个名再双击 `core.exe`，Windows 直接报
  **STATUS_DLL_NOT_FOUND (0xC0000135)** 起不来 —— base 是运行期真加载的动态库，
  不是编译期塞进 exe 的。
- **客户是真隔离的**：客户A的 exe 里搜不到 `customer_b`，客户B的里搜不到
  `customer_a`。不是"运行时不调用"，是二进制里没有这些字节。
- **依赖是单向的**：`base.dll` 里搜不到 `algo.dll` —— 反向依赖不是靠约定，是物理上不存在。
  base 可以单独拿去别的项目用，algo 离了 base 跑不起来。

## 五、演示步骤（源码 / 编译角度）

### 1. 客户A的交付物：苹果外观检测项目

```bash
cmake -B build-a -DFLOW_A=ON && cmake --build build-a
./build-a/bin/core
```
```
[10:22:31][main] IndustryLibDemo core   build=common+A     ← BUILD_TAG
[10:22:31][main] flows compiled in: 2
[10:22:31][main]   [1] common
[10:22:31][main]   [2] customer_a
[10:22:32][flow] {"type":"result","flow":"customer_a","frame":2,"score":155,"verdict":"OK","threshold":150,"grade":"B"}
```

### 2. 客户B的交付物：角度测量项目（另一个项目）

```bash
cmake -B build-b -DFLOW_B=ON && cmake --build build-b
./build-b/bin/core
```
```
[10:23:02][main] IndustryLibDemo core   build=common+B
[10:23:02][main] flows compiled in: 2
[10:23:02][main]   [1] common
[10:23:02][main]   [2] customer_b
[10:23:03][flow] {"type":"result","flow":"customer_b","frame":1,"angle":-24,"verdict":"NG","tolerance":15,"direction":"left","confidence":83}
```
**注意主指标都换了**：客户A看 `score`，客户B看 `angle`——不同硬件、不同算法、
不同输出。同一个 `main.c`、同一个 `libbase`、同一个 `libalgo`，只是编译开关不同。

### 3. 内部全功能版：两个项目一起跑（回归测试用）

```bash
cmake -B build-full -DFLOW_A=ON -DFLOW_B=ON && cmake --build build-full
./build-full/bin/core --trace     # 加 --trace 能看到调用链
```
```
[10:23:02][main] IndustryLibDemo core   build=common+A+B
[10:23:02][main] flows compiled in: 3
```

### 4. core 独立运行（不开任何 UI）

直接运行 `core` 即可 —— 它自己产帧、自己判定、自己打印，**一个 UI 都不需要**。
UI 只是 TCP 上的一个可选观察者。这是"UI 可替换"的前提：核心根本不知道 UI 存在。

### 5. 两个 UI 轮流 / 同时连同一个 core

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

## 六、流程 = 客户项目：现有哪些、怎么切、怎么加

### 现有 3 个流程 = 3 个项目

| 流程 | 开关 | 项目 | 硬件(base) | 算法(algo) | 判定 | 输出字段 |
|---|---|---|---|---|---|---|
| `common` | 无（**始终编入**） | 通用表面检测 | CAM_A | `detect_surface` | `score>=140` | `score`/`threshold` |
| `customer_a` | `-DFLOW_A=ON` | **苹果外观检测** | CAM_A 面阵 | `detect_surface` | `score>=150` | `score`/`threshold`/`grade` |
| `customer_b` | `-DFLOW_B=ON` | **工件角度测量** | CAM_B 线扫 | `angle_measure` | `\|angle\|<=15°` | `angle`/`direction`/`tolerance`/`confidence` |

看清楚 A 和 B 那两行：**不同硬件、不同算法、不同判定、不同输出字段，连主指标都不是
一个东西**（一个是评分，一个是角度）。它们不是"同一个产品的两套阈值"，是两个项目。
`flow_customer_b.c` 里一个字都没提 `detect_surface`——客户B的项目根本不需要那个能力。

共用的是下面两层：都靠 `base/imgproc` 的 mean/min/max/stddev 原语，都用 `base` 的
相机接口、日志、TCP。**这就是"底层类似、上层是不同项目"**。

### 切项目 = 改 cmake 开关，重新编

```bash
cmake -B build-a    -DFLOW_A=ON              # common+A    客户A的交付物
cmake -B build-b    -DFLOW_B=ON              # common+B    客户B的交付物
cmake -B build-full -DFLOW_A=ON -DFLOW_B=ON  # common+A+B  内部回归测试版
```

不是配置文件、不是命令行参数、不是插件目录 —— **是编译期决定的**。
客户A的设备上装的就是 `build-a` 那个 exe，里面没有客户B的项目；反之亦然。
`tools\show_deps.ps1` 可当场验证，CI 里也有断言守着。

（现实里 A 和 B 是两台不同设备，不会同时跑。`build-full` 把两个项目编在一起是给
我们自己做回归测试用的——一次跑完所有客户的流程。）

本地演示切换不用重编，三个版本 CI 都编好了：`tools\demo.ps1 -Flow a|b|full`。

### 看调用关系：`--trace`

```bash
./core --trace          # 或运行中通过 TCP 发 trace 命令切换
```
每一行都是**那一层自己打的**，不是上层替它narrate的：

```
[main] --- tick: 调 flow[1] customer_a.run() ---     宿主只喊一声
[flow.customer_a] 苹果外观项目：取图 CAM_A -> algo.detect_surface()
[base.plc_sim] grab CAM_A -> frame=2                 flow 自己去 base 取图
[algo.detect]  detect_surface() -> 调 base.imgproc 取特征
[base.imgproc]   mean(n=64) = 153.7                  algo 自己调 base 原语
[base.imgproc]   stddev(n=64) = 11.8
[algo.detect]  score = mean 153.7 + contrast 40*0.2 - noise 11.8*0.6 = 155
[flow.customer_a] 判定 score=155 vs 150 -> OK 等级=B  flow 自己判

[main] --- tick: 调 flow[2] customer_b.run() ---     同一个宿主，另一个项目
[flow.customer_b] 角度测量项目：取图 CAM_B -> algo.angle_measure()
[base.plc_sim] grab CAM_B -> frame=1                 不同的相机
[algo.angle]  angle_measure() -> 调 base.imgproc 取左右半区亮度
[base.imgproc]   mean(n=32) = 146.4                  不同的算法，连 n 都不一样
[base.imgproc]   mean(n=32) = 154.8
```

`main → flow → algo → base` 一层层下去，每层只调下一层。这是**运行时**的证据；
再加上源码的 `#include` 图、CMake 的 `target_link_libraries`、和二进制导入表
（`show_deps.ps1`），四个层次都能验证同一个方向。

### 来了个新客户C，怎么落地？

**情况一：现有算法够用**（比如客户C也是看表面，只是阈值和分级规则不同）
—— 只动 flow 层，4 处改动，`base` / `algo` 一行不碰，库都不用重编：

1. **写项目**：新建 `core/flow/flow_customer_c.c`
   ```c
   #include "flow.h"
   #include "detect.h"   /* 挑一个现成算法 */
   #include "plc_sim.h"  /* 挑一个现成硬件 */

   static void run(char *json, size_t n)
   {
       frame_t f; detect_result_t r;
       plc_sim_grab(PLC_CAM_A, &f);                  /* 自己取图  */
       detect_surface(f.px, PLC_FRAME_PIXELS, &r);   /* 自己调算法 */
       snprintf(json, n, "{...自己判、自己定输出字段...}");
   }
   const flow_t FLOW_CUSTOMER_C = {"customer_c", run};
   ```
2. **声明**：`core/flow/flow.h` 加 `#ifdef FLOW_C extern const flow_t FLOW_CUSTOMER_C; #endif`
3. **注册**：`core/main.c` 的 `g_flows[]` 数组加 `#ifdef FLOW_C &FLOW_CUSTOMER_C, #endif`
4. **开关**：根 `CMakeLists.txt` 加 `option(FLOW_C ...)` 和跟 A/B 一样的三行

**情况二：客户C要一个全新能力**（比如"数个数"，现有算法都没有）
—— 在 `algo/` 加一个模块（如 `count.c`，照 `angle.c` 抄，只调 `base/imgproc` 的原语），
在 `algo/CMakeLists.txt` 里加一行，然后照情况一写 flow。**`base/` 依然一行不动。**

新硬件同理：在 `base/plc_sim.c` 加一个通道，algo 和别的客户项目都不受影响。

这就是分层的收益：**改动被摁在它该在的那一层，越往下越不动**。客户C的项目上线，
客户A、B的交付物字节不变——不需要回归测试它们。

> 情况一的 4 步实测过：临时分支上做了个"客户C"，Linux / Windows 双平台一次编过，
> `BUILD_TAG` 变成 `common+A+C`。当时 `flow_customer_b.c` 就在目录里，但没开
> `FLOW_B`，它就是不在二进制里。任意组合，互不干扰。

代价是加流程要碰 4 个文件（而不是丢一个插件 .so 进目录）。这是刻意的取舍：
换来的是"别的客户代码物理上不在交付物里"。真到流程几十个的规模，第 2、3 步可以用
一张 X-macro 表收敛成一处。

## 七、TCP 协议

连接 `127.0.0.1:9500`，**行分隔 JSON**。

发送（纯文本行）：`start` / `stop` / `status` / `trace`（切换调用链日志）

各客户项目的输出字段本就不同（A 有 `score`/`grade`，B 有 `angle`/`direction`），
UI 只依赖公共骨架 `flow`/`frame`/`verdict`，其余字段原样展示。

接收：
```jsonc
{"type":"result","flow":"customer_a","frame":12,"score":151,"verdict":"OK","threshold":150,"grade":"B"}
{"type":"result","flow":"customer_b","frame":12,"angle":-9,"verdict":"OK","tolerance":15,"direction":"left","confidence":83}
{"type":"ack","cmd":"start","running":true}
{"type":"status","running":true,"flow_count":3,"build":"common+A+B","trace":false}
```

## 八、CI

`.github/workflows/build.yml` 在 Linux 与 Windows runner 上各构建 A 版和全功能版，
并跑 `tools/verify_tcp.py` 验证：结果流、`start`/`stop`/`status`、BUILD_TAG、
流程数、UI 断开重连、以及"关掉的流程不进二进制"。
