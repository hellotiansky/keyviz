# [Keyviz](https://keyviz.org)

<div>
   <img src="https://img.shields.io/github/v/release/mulaRahul/keyviz?style=flat-square" alt="Releases">
   <img src="https://img.shields.io/github/downloads/mulaRahul/keyviz/total?style=flat-square" alt="Downloads">
   <img src="https://img.shields.io/github/stars/mulaRahul/keyviz?style=flat-square" alt="Stars">
   <img src="https://img.shields.io/github/license/mulaRahul/keyviz?style=flat-square" alt="License">
   <img src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS-lightgrey?style=flat-square" alt="Platform Support">
</div>

Keyviz 是一款**免费且开源**的工具，可以实时可视化你的按键和鼠标操作。无论是录制教程、做演示、协作办公，还是其他需要展示操作的场景，都能让观众清楚地看到你按下了哪些快捷键。

## fork改进：增加中文翻译

## ⌨️ 按键与 🖱️ 鼠标操作
除了普通按键，Keyviz 还可以可视化鼠标操作，例如 <kbd>Cmd</kbd> + <kbd>点击</kbd>、<kbd>Alt</kbd> + <kbd>拖拽</kbd> 等。

<img src="previews/visualization.png" alt="按键可视化" width="450">

在光标旁显示鼠标点击和滚轮滚动。

<img src="previews/mouse-indicator.gif" alt="鼠标指示器" width="450">

</br>

## ⚙️ 全面自定义
不必局限于默认样式，可视化的每个细节都可以调整：

- **样式**：修改颜色（修饰键与普通键可分别设置）、大小、布局、边框和背景。
- **过滤**：通过热键或自定义过滤器控制显示哪些按键。
- **历史记录**：保留最近输入的可视化轨迹。
- **位置**：把可视化区域移动到屏幕的任意位置。
- **动画**：使用预设的进入和退出动画，自定义按键的出现与消失方式。

</br>

<img src="previews/settings.png" alt="设置面板" width="600">

</br>

## 📥 安装

### Windows 与 macOS
你可以从 **[GitHub Releases](https://github.com/mulaRahul/keyviz/releases)** 页面下载最新版本的 Keyviz。

- **Windows**：下载 `.msi` 安装包，运行并按照提示完成安装。
- **macOS**：下载 `.dmg`。
  **注意**：Keyviz 需要**输入监控**和**辅助功能**权限，请在以下位置启用：
  `系统设置 > 隐私与安全性 > 输入监控 / 辅助功能`

### Linux（X11）
Keyviz 兼容使用 X11 协议的 Linux。目前可以按照下方的构建说明自行编译体验。

</br>

## 🛠️ 从源码构建

如果你想参与贡献或体验最新功能，请确保系统中已安装 [Node.js](https://nodejs.org/) 和 [Tauri](https://v2.tauri.app/start)。

1. **克隆仓库：**
   ```bash
   git clone https://github.com/mulaRahul/keyviz.git
   cd keyviz
   ```

2. **安装依赖：**
   ```bash
   npm install
   ```

3. **构建可执行文件：**
   ```bash
   npx tauri build
   ```

<br/>

## 🤖 自动构建 Windows 发布版

仓库内的 [`.github/workflows/build-windows.yml`](.github/workflows/build-windows.yml) 会在 GitHub Actions 中自动构建适用于 **Windows 10 / 11（64 位）** 的安装包：

- **手动触发**：进入 `Actions > Build Windows Release > Run workflow`，构建完成后可在本次运行的 Artifacts 中下载 `.msi` 和 `.exe` 安装包。
- **发布版本**：推送形如 `v2.1.1` 的标签（tag），工作流会自动构建并创建 GitHub Release 草稿，确认无误后发布即可。发布后会同时触发 Winget 发布工作流。

安装包包含 `.msi`（Windows Installer）和 `.exe`（NSIS）两种格式。安装程序会自动检测并安装 WebView2 运行时，因此可以在未预装 WebView2 的 Windows 10 系统上正常使用。

<br/>

## 💖 支持项目

- **收藏仓库**：帮助更多人发现这个项目！
- **GitHub Sponsors**：[赞助 @mularahul](https://github.com/sponsors/mulaRahul)
- **Keyviz Pro**：在支持开源项目发展的同时，解锁专属功能。

👉 **[在 keyviz.org/pro 升级到 Pro](https://keyviz.org/pro)**

</br>

---

  使用 🦀 和 ❤️ 构建，基于 <a href="https://v2.tauri.app/">Tauri</a>。
