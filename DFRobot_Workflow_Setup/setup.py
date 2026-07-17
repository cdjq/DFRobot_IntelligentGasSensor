#!/usr/bin/env python3
"""Install and run the DFRobot local/cloud code-quality workflow."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path


DEFAULT_PRECOMMIT_URL = "git@github.com:cdjq/pre-commit-local.git"
DEFAULT_CI_URL = "https://github.com/cdjq/DFRobot_CI.git"


class SetupError(RuntimeError):
  """An expected setup failure with a user-facing message."""


def configure_console() -> None:
  for stream in (sys.stdout, sys.stderr):
    reconfigure = getattr(stream, "reconfigure", None)
    if reconfigure is not None:
      reconfigure(errors="replace", line_buffering=True)


def run(
  command: list[str],
  *,
  cwd: Path | None = None,
  capture: bool = False,
  check: bool = True,
) -> subprocess.CompletedProcess[str]:
  result = subprocess.run(
    command,
    cwd=cwd,
    text=True,
    encoding="utf-8",
    errors="replace",
    stdout=subprocess.PIPE if capture else None,
    stderr=subprocess.PIPE if capture else None,
    check=False,
  )
  if check and result.returncode != 0:
    detail = ""
    if capture:
      detail = (result.stderr or result.stdout or "").strip()
    suffix = f"\n{detail}" if detail else ""
    raise SetupError(f"命令执行失败：{' '.join(command)}{suffix}")
  return result


def require_command(name: str) -> None:
  if shutil.which(name) is None:
    raise SetupError(f"未找到 {name}。请先安装并将其加入 PATH。")


def git_output(*args: str, cwd: Path) -> str:
  return run(["git", *args], cwd=cwd, capture=True).stdout.strip()


def resolve_target(script_dir: Path, requested: str | None) -> Path:
  candidate = Path(requested).expanduser().resolve() if requested else script_dir.parent
  if not candidate.is_dir():
    raise SetupError(f"目标目录不存在：{candidate}")

  try:
    repo_root = Path(git_output("rev-parse", "--show-toplevel", cwd=candidate)).resolve()
  except SetupError as exc:
    raise SetupError(f"目标目录不是 Git 仓库：{candidate}") from exc

  if requested is None and repo_root != candidate:
    raise SetupError(
      "请把整个 DFRobot_Workflow_Setup 文件夹放在仓库根目录后再运行；"
      "也可以使用 --target 指定仓库。"
    )
  return repo_root


def exclude_setup_folder(target: Path, script_dir: Path) -> None:
  try:
    relative = script_dir.resolve().relative_to(target.resolve()).as_posix()
  except ValueError:
    return
  if not relative or relative == ".":
    return

  exclude_value = git_output("rev-parse", "--git-path", "info/exclude", cwd=target)
  exclude_file = Path(exclude_value)
  if not exclude_file.is_absolute():
    exclude_file = target / exclude_file
  exclude_file = exclude_file.resolve()
  pattern = f"/{relative}/"

  existing = ""
  if exclude_file.is_file():
    existing = exclude_file.read_text(encoding="utf-8", errors="replace")
  if pattern not in existing.splitlines():
    exclude_file.parent.mkdir(parents=True, exist_ok=True)
    separator = "" if not existing or existing.endswith(("\n", "\r")) else "\n"
    with exclude_file.open("a", encoding="utf-8", newline="\n") as handle:
      handle.write(f"{separator}{pattern}\n")
    print(f"[排除] 工具文件夹不会被 git add：{pattern}")
  else:
    print(f"[排除] 工具文件夹已在 Git 本地排除列表中：{pattern}")

  indexed = git_output("ls-files", "--", relative, cwd=target)
  if indexed:
    print(
      "[提醒] 工具文件夹之前已被暂存或跟踪；如不希望提交，请执行：\n"
      f"       git rm -r --cached -- {relative}"
    )


def update_checkout(destination: Path, url: str, label: str, no_update: bool) -> None:
  if (destination / ".git").is_dir():
    if no_update:
      print(f"[复用] {label}：{destination}")
      return
    print(f"[更新] {label}")
    run(["git", "pull", "--ff-only"], cwd=destination)
    return

  if destination.exists():
    if any(destination.iterdir()):
      raise SetupError(f"缓存目录存在但不是有效 Git 仓库：{destination}")
    destination.rmdir()

  destination.parent.mkdir(parents=True, exist_ok=True)
  print(f"[下载] {label}\n       {url}")
  run(["git", "clone", "--depth", "1", url, str(destination)])


def venv_python(venv_dir: Path) -> Path:
  if os.name == "nt":
    return venv_dir / "Scripts" / "python.exe"
  return venv_dir / "bin" / "python"


def find_precommit_python(runtime_dir: Path) -> Path:
  current = Path(sys.executable).resolve()
  probe = run(
    [str(current), "-m", "pre_commit", "--version"], capture=True, check=False
  )
  if probe.returncode == 0:
    print(f"[环境] {probe.stdout.strip()}")
    return current

  venv_dir = runtime_dir / "venv"
  python_path = venv_python(venv_dir)
  if not python_path.is_file():
    print("[环境] 创建独立 Python 虚拟环境")
    run([str(current), "-m", "venv", str(venv_dir)])

  probe = run(
    [str(python_path), "-m", "pre_commit", "--version"],
    capture=True,
    check=False,
  )
  if probe.returncode != 0:
    print("[环境] 安装 pre-commit（仅安装在脚本的 .runtime 中）")
    run([str(python_path), "-m", "pip", "install", "pre-commit"])
  return python_path


def create_runtime_config(source_repo: Path, runtime_dir: Path) -> Path:
  source = source_repo / ".github" / "config" / ".pre-commit-config.yaml"
  if not source.is_file():
    raise SetupError(f"本地格式化仓库缺少配置文件：{source}")

  config_dir = source.parent.resolve().as_posix()
  content = source.read_text(encoding="utf-8")
  content = content.replace("../pre-commit-local/.github/config", config_dir)
  content = content.replace("..\\pre-commit-local\\.github\\config", config_dir)

  generated = runtime_dir / "generated.pre-commit-config.yaml"
  generated.write_text(content, encoding="utf-8", newline="\n")
  return generated


def install_cloud_workflow(ci_repo: Path, target: Path, script_dir: Path) -> None:
  source_root = ci_repo / ".github"
  if not source_root.is_dir():
    raise SetupError(f"DFRobot_CI 仓库中没有 .github：{ci_repo}")

  timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
  backup_root = script_dir / "backups" / timestamp
  changed = 0
  backed_up = 0

  for source in sorted(path for path in source_root.rglob("*") if path.is_file()):
    relative = source.relative_to(source_root)
    destination = target / ".github" / relative
    if destination.is_file() and destination.read_bytes() == source.read_bytes():
      continue
    if destination.exists() and not destination.is_file():
      raise SetupError(f"云端工作流目标不是普通文件：{destination}")
    if destination.is_file():
      backup = backup_root / ".github" / relative
      backup.parent.mkdir(parents=True, exist_ok=True)
      shutil.copy2(destination, backup)
      backed_up += 1
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    changed += 1

  if changed == 0:
    print("[云端] .github 工作流已是最新，无需修改")
  else:
    print(f"[云端] 已安装/更新 {changed} 个 .github 文件")
  if backed_up:
    print(f"[备份] 已备份 {backed_up} 个原文件到：{backup_root}")


def repository_files(target: Path, script_dir: Path) -> list[str]:
  tracked = run(
    ["git", "ls-files", "-z"], cwd=target, capture=True
  ).stdout.split("\0")
  untracked = run(
    ["git", "ls-files", "--others", "--exclude-standard", "-z"],
    cwd=target,
    capture=True,
  ).stdout.split("\0")

  try:
    setup_relative = script_dir.resolve().relative_to(target.resolve()).as_posix()
  except ValueError:
    setup_relative = ""
  setup_prefix = f"{setup_relative}/" if setup_relative else ""

  result: list[str] = []
  seen: set[str] = set()
  for name in [*tracked, *untracked]:
    normalized = name.replace("\\", "/")
    if not normalized or normalized in seen:
      continue
    if setup_prefix and (
      normalized == setup_relative or normalized.startswith(setup_prefix)
    ):
      continue
    if (target / Path(normalized)).is_file():
      seen.add(normalized)
      result.append(normalized)
  return result


def file_batches(files: list[str]) -> list[list[str]]:
  limit = 20_000 if os.name == "nt" else 100_000
  batches: list[list[str]] = []
  current: list[str] = []
  length = 0
  for name in files:
    cost = len(name) + 3
    if current and length + cost > limit:
      batches.append(current)
      current = []
      length = 0
    current.append(name)
    length += cost
  if current:
    batches.append(current)
  return batches


def run_local_checks(
  python_path: Path,
  config: Path,
  target: Path,
  script_dir: Path,
  passes: int,
) -> bool:
  files = repository_files(target, script_dir)
  if not files:
    print("[本地] 没有可检查的文件")
    return True

  batches = file_batches(files)
  print(f"[本地] 将检查 {len(files)} 个文件（不会自动 git add/commit/push）")
  for pass_number in range(1, passes + 1):
    print(f"\n===== pre-commit 第 {pass_number}/{passes} 轮 =====")
    success = True
    for batch_number, batch in enumerate(batches, start=1):
      if len(batches) > 1:
        print(f"--- 文件批次 {batch_number}/{len(batches)} ---")
      result = run(
        [
          str(python_path),
          "-m",
          "pre_commit",
          "run",
          "--config",
          str(config),
          "--files",
          *batch,
        ],
        cwd=target,
        check=False,
      )
      success = success and result.returncode == 0
    if success:
      print("[本地] 所有格式化和检查均已通过")
      return True
    if pass_number < passes:
      print("[本地] 本轮可能已自动修复文件，继续复查")

  print("[本地] 复查后仍有失败项，请根据上方日志手动处理")
  return False


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(
    description="安装 DFRobot 云端 CI，并运行本地 pre-commit 格式化/检查。"
  )
  parser.add_argument("--target", help="目标 Git 仓库；默认是脚本文件夹的上一级")
  parser.add_argument("--skip-format", action="store_true", help="跳过本地格式化检查")
  parser.add_argument("--skip-cloud", action="store_true", help="跳过云端 .github 安装")
  parser.add_argument(
    "--passes", type=int, default=2, help="本地检查轮数（默认：2）"
  )
  parser.add_argument(
    "--precommit-url", default=DEFAULT_PRECOMMIT_URL, help="pre-commit-local Git 地址"
  )
  parser.add_argument("--ci-url", default=DEFAULT_CI_URL, help="DFRobot_CI Git 地址")
  parser.add_argument(
    "--no-update", action="store_true", help="存在本地缓存时不执行 git pull"
  )
  return parser.parse_args()


def main() -> int:
  configure_console()
  args = parse_args()
  if args.passes < 1:
    raise SetupError("--passes 必须大于等于 1")
  if args.skip_format and args.skip_cloud:
    raise SetupError("不能同时使用 --skip-format 和 --skip-cloud")

  require_command("git")
  script_dir = Path(__file__).resolve().parent
  target = resolve_target(script_dir, args.target)
  runtime_dir = script_dir / ".runtime"
  print(f"[目标] {target}")
  exclude_setup_folder(target, script_dir)

  ci_repo: Path | None = None
  if not args.skip_cloud:
    ci_repo = runtime_dir / "DFRobot_CI"
    update_checkout(ci_repo, args.ci_url, "DFRobot_CI", args.no_update)

  python_path: Path | None = None
  config: Path | None = None
  if not args.skip_format:
    precommit_repo = runtime_dir / "pre-commit-local"
    update_checkout(
      precommit_repo, args.precommit_url, "pre-commit-local", args.no_update
    )
    python_path = find_precommit_python(runtime_dir)
    config = create_runtime_config(precommit_repo, runtime_dir)

  if ci_repo is not None:
    install_cloud_workflow(ci_repo, target, script_dir)

  local_success = True
  if python_path is not None and config is not None:
    local_success = run_local_checks(
      python_path, config, target, script_dir, args.passes
    )

  print("\n===== 执行结果 =====")
  if not args.skip_cloud:
    print("[完成] 云端 CI 已准备：.github/workflows/ci.yml")
  if args.skip_format:
    print("- 已按参数跳过本地格式化检查")
  elif local_success:
    print("[完成] 本地格式化检查已通过")
  else:
    print("! 本地格式化已执行，但仍有需要人工处理的检查项")
  print("下一步：检查 git diff，确认后自行 git add、commit 和 push。")
  return 0 if local_success else 2


if __name__ == "__main__":
  try:
    raise SystemExit(main())
  except KeyboardInterrupt:
    print("\n已取消。", file=sys.stderr)
    raise SystemExit(130)
  except SetupError as exc:
    print(f"\n错误：{exc}", file=sys.stderr)
    raise SystemExit(1)
