# DFRobot 规范化工作流一键脚本

这个文件夹把《github Auction 规范化工作流》“新版 .github / 如何用起来”中的两段操作合并成一次执行：

1. 下载 `pre-commit-local`，对库中所有文件运行本地格式化和静态检查；
2. 下载 `DFRobot_CI`，把云端检查入口安装到库根目录的 `.github`；
3. 本地检查默认执行两轮，以便第一轮自动修改后立即复查。

脚本不会自动执行 `git add`、`git commit` 或 `git push`。执行时还会把本工具文件夹加入当前仓库的 `.git/info/exclude`，因此以后运行 `git add .` 不会把它加入提交；该排除配置只保存在本机，不会上传到远程仓库。

## 最简单的用法（Windows）

把整个 `DFRobot_Workflow_Setup` 文件夹复制到目标库的根目录：

```text
DFRobot_YourLibrary/
├── .git/
├── src/
├── examples/
└── DFRobot_Workflow_Setup/
```

然后双击 `run.cmd`，或在 VS Code / PowerShell 终端执行：

```powershell
.\DFRobot_Workflow_Setup\run.cmd
```

需要预先安装：

- Git，并已配置访问 GitHub 的 SSH Key；
- Python 3（需要 3.10 或更高版本）。

`pre-commit-local` 当前使用公司私有 SSH 地址。若出现 `Permission denied (publickey)`，需要先为 GitHub 配置 SSH Key 和该仓库的读取权限。

## Git Bash、Linux 或 macOS

```bash
sh ./DFRobot_Workflow_Setup/run.sh
```

## 执行完成后

```bash
git status
git diff
git add .
git commit -m "Apply code quality workflow"
git push
```

推送后，GitHub Actions 会运行 `.github/workflows/ci.yml`，调用 `cdjq/cloud-code-checks` 做云端检查。打 `v1.2.3` 或 `V1.2.3` 形式的 tag 时，还会检查 tag 与 `library.properties` 中版本是否一致。

## 常用参数

直接调用 Python 主脚本时可以传入参数：

```powershell
# 只安装云端 .github，不运行本地格式化
python .\DFRobot_Workflow_Setup\setup.py --skip-format

# 只运行本地格式化，不安装云端 .github
python .\DFRobot_Workflow_Setup\setup.py --skip-cloud

# 改为检查三轮
python .\DFRobot_Workflow_Setup\setup.py --passes 3

# 脚本不在仓库内时，明确指定目标库
python .\DFRobot_Workflow_Setup\setup.py --target D:\path\to\DFRobot_Library

# 已有缓存时不联网更新
python .\DFRobot_Workflow_Setup\setup.py --no-update
```

运行 `python .\DFRobot_Workflow_Setup\setup.py --help` 可查看全部参数。

## 安全和恢复

- 本地格式化工具会修改不符合规范的源码，因此执行后必须先看 `git diff`；
- 脚本不会修改 Git 暂存区，也不会提交或推送；
- 工具文件夹会写入本地 `.git/info/exclude`，不会被 `git add .` 加入提交；
- 如果运行脚本之前已经暂存过该文件夹，请先执行 `git rm -r --cached -- DFRobot_Workflow_Setup`；
- 若原来已有同名 `.github` 文件，脚本会将旧文件备份到 `DFRobot_Workflow_Setup/backups/时间戳/` 后再更新；
- 下载的仓库和独立 Python 环境保存在 `.runtime/`，已被本文件夹内的 `.gitignore` 忽略；
- 若静态检查仍然失败，云端 CI 仍会完成安装，脚本会返回退出码 `2` 并保留完整错误日志。
