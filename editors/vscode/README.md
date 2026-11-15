# Candy Language for VS Code

Candy 语言基础插件，适用于 VS Code 1.85 及以上版本。

## 功能

- 自动识别 `.candy` 文件。
- 高亮关键字、函数、内置函数、数值、`#` 注释、单双引号字符串及三引号多行字符串。
- 注释切换、括号/引号自动补全、块缩进及基于缩进的折叠。
- 可用 `#region 名称` / `#endregion` 显式分组折叠；普通 `def`、`if` 等语句不会显示为小地图分区标题。
- 代码片段：`def`、`if`、`ifelse`、`elif`、`while`、`for`、`print`。
- 从编辑器标题的播放按钮、右键菜单或命令面板执行 **Candy: Run Current File**。
  快捷键：macOS `Cmd+Alt+R`，其他平台 `Ctrl+Alt+R`。

本版本不包含语言服务器、语义补全、错误诊断、格式化器或调试器。

## 安装

如果已有打包好的 VSIX，在仓库的 `editors/vscode` 目录中直接安装，
**不需要运行 npm，也不需要升级本机 Node.js**：

```sh
code --install-extension candy-language-0.1.1.vsix
```

也可在 VS Code 扩展视图的 `…` 菜单中选择 **Install from VSIX...**。
插件使用 VS Code 内置的 Node.js 运行，不包含 Candy 解释器或运行时依赖。

### 从源码打包

需要 Node.js 22+ 和 npm，在仓库的 `editors/vscode` 目录中运行：

```sh
npm ci && npm run package && code --install-extension candy-language-0.1.1.vsix
```

使用 `&&` 确保依赖安装或构建失败时停止，避免误安装上次生成的 VSIX。
打包时会从仓库根目录复制 Apache-2.0 许可证。

### Node 版本或 npm 缓存权限问题

- `EBADENGINE`：本机 Node 版本低于打包工具要求的 22。
- npm 缓存中的 `EACCES` / `EEXIST`：默认缓存目录没有写权限。
- `Cannot find module 'vscode-textmate'`：依赖安装失败后的连带错误，需要先成功完成 `npm ci`。

无需使用 `sudo`、`--force` 或修改全局 npm 设置。可以在以下子 shell 中使用独立缓存，
通过 npm 临时下载并使用 Node.js 22 打包，不替换本机 Node.js：

```sh
(
  export npm_config_cache="$HOME/.cache/candy-npm"
  npm exec --yes --package=node@22 -- npm ci &&
  npm exec --yes --package=node@22 -- npm run package &&
  code --install-extension candy-language-0.1.1.vsix
)
```

只有重新构建插件才需要这些步骤；已经安装成功时，可以直接配置解释器并使用插件。

## 配置解释器

先在仓库根目录编译 Candy：

```sh
cmake -S . -B build
cmake --build build --target candy
```

打开 Candy 仓库作为工作区，在工作区设置中添加：

```json
{
  "candy.executablePath": "build/shell/candy",
  "[candy]": {
    "editor.tabSize": 2,
    "editor.insertSpaces": true
  }
}
```

默认执行 `PATH` 中的 `candy`。也可以填写解释器的绝对路径；包含路径分隔符的相对路径
以当前文件所属的工作区文件夹为基准，没有工作区时以脚本目录为基准。多根工作区按文件
所属文件夹读取配置。此设置仅接受可执行文件路径，不接受 shell 命令或附加参数。

运行前自动保存已修改的文件；未命名文件需先保存到磁盘。脚本在其所在目录运行，
输出和解释器错误显示在 VS Code 任务终端中。路径含空格也可直接使用，无需添加引号。
解释器不存在或不能启动时，任务终端会显示启动错误。

仅受信任的工作区允许执行脚本；未信任和虚拟工作区仍可使用高亮和片段。
Remote SSH / Dev Containers 中需要在远程扩展宿主上安装 Candy。

## 开发与测试

以下命令同样需要 Node.js 22+，缓存权限问题的处理方式见上文。

```sh
npm ci && npm test &&
  code --extensionDevelopmentPath="$PWD" ../../examples/sudoku.cdy
```

测试覆盖真实 TextMate 分词、语言配置、片段和运行命令（保存失败、信任限制、
多根工作区、含特殊字符的路径及任务启动失败）。开发窗口可手动验证高亮、Tab 展开
代码片段、注释切换、缩进/折叠和运行按钮。
