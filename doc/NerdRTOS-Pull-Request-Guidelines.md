# NerdRTOS Pull Request 规范

## 规范目的
本规范用于约束 NerdRTOS 的 Pull Request 提交流程，目标是保证每个 PR 具备清晰的问题边界、可审查的提交粒度、可验证的测试结果和可维护的提交历史。

## PR 范围

一个 PR 应当只解决一个问题，如：
- 修复一个 bug
- 增加一个可用且边界清晰的功能
- 重构一个模块
- 创建、更新文档

不建议一个 PR 中同时混入不同的主题，比如**同时**修改 IPC 和添加中间件。

## 分支命名规范

分支名应该描述**改动类型**和**影响范围**。

当前 CI 并不检查分支名，但 `.github/workflows/lint.yml` 会在 PR 中检查提交信息和 PR 标题是否符he Conventional Commits。因此，分支命名应与 PR 标题、提交信息保持同一语义体系。

推荐格式：
``` text
<type>/<scope>-<summary>
```
其中：
- `type` 表示改动类型，应该与 Conventional Commits 的 type 保持一致。
- `scope` 表示影响范围，例如 thread、timer、scheduler、ipc、docs、ci
- `summary` 用简短的英文描述改动内容，单词之间用 `-` 进行连接。

分支类型建议包括：
- fix：修复缺陷
- feat：新增功能
- refactor：重构已有实现，不改变外部语义
- docs：仅修改文档
- ci：修改 GitHub Actions、检查脚本或构建流程
- test：新增或调整测试
- build：修改构建系统、工具链或 BSP 构建配置

## 提交信息
NerdRTOS 使用 Conventional Commits。

提交标题格式：
``` text
<type>(<scope>): <subject>
```
允许的 type：
```text
feat, fix, docs, style, refactor, perf, test, build, ci, chore, revert
```

提交正文应包含：
- Summary
- Design Rationale
- Impact
- Documentation Impact
- Verification
- PR Checklist

## 本地检查
提交 PR 前应该至少检查：
``` shell
python3 .github/scripts/check_style.py
```

## PR 自检清单
PR 自检清单用于辅助作者在提交前确认范围、提交信息、检查命令和文档影响。

模板内容如下：
- [ ] 本 PR 只解决一个明确问题，没有混入无关改动
- [ ] PR 标题符合 Conventional Commits
- [ ] commit message 符合 Conventional Commits
- [ ] 已检查暂存内容，没有提交无关文件
- [ ] 已执行代码风格检查：`python3 .github/scripts/check_style.py`
- [ ] 已执行必要的构建或测试
- [ ] 涉及 API、行为、构建方式或使用方式变化时，已同步更新文档
- [ ] 若没有文档影响，已在 PR 描述中说明原因

**清单中的项目应根据 PR 的实际改动范围进行判断，不适用于当前 PR 的项目可以不执行。**

自检清单同时也在 `.github/pull_request_template.md` 中，由 GitHub 在创建 PR 时自动插入到 PR 描述中。
