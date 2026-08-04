⚡ 模式：锻造

已完成并提交 4 笔原子提交，`main` 干净、领先 `origin/main` 4 个提交，未推送。

核心成果：

- 修正 CMake/Keil 配置漂移，移除外部工程依赖与全量 SDK 递归收录。
- 新增项目内 [MSPM0 skill](E:\EmbeddedDevelopment\WorkSpace\MSPM0G3507\MCU_Template\skills\mspm0g3507-template\SKILL.md) 和 [构建 ADR](E:\EmbeddedDevelopment\WorkSpace\MSPM0G3507\MCU_Template\docs\decisions\0001-build-contract.md)。
- 重构 Motor：Device 仅保留 PI、死区与限幅；Board 经 HAL 注入 PWM port；旧 API 移至 [Facade](E:\EmbeddedDevelopment\WorkSpace\MSPM0G3507\MCU_Template\4_facade\motor_facade.c)。
- 新增 [Motor host test](E:\EmbeddedDevelopment\WorkSpace\MSPM0G3507\MCU_Template\tests\motor_device_test.c)，验证注入、限幅、死区、PI、停止与非法配置。
- 清理 Git 中已跟踪的 CMake 缓存、构建/烧录日志，本地文件仍保留并已忽略。
- 更新 [架构文档](E:\EmbeddedDevelopment\WorkSpace\MSPM0G3507\MCU_Template\docs\myused_five_layer_architecture.md) 与 [项目状态](E:\EmbeddedDevelopment\WorkSpace\MSPM0G3507\MCU_Template\docs\project_status.md)。

验证通过：

- `motor_device_test`：1/1 通过。
- ARMClang：Motor Device/Board/Facade 语法检查通过。
- CMake Ninja configure 通过。
- `git diff --check` 通过。
- 项目 skill 校验通过。

已知问题：UV4 CLI 当前会启动驻留 GUI 但不生成新日志，所以本轮不能诚实宣称 Keil clean rebuild 已通过；脚本现已先删除旧日志，避免误判历史成功日志为本次成功。

提交：

- `fb8d05b update: テンプレートの構築基盤を整備`
- `c5a03dd update: Motor のPWMポート境界を分離`
- `7b43c74 clean: 生成ログとCMakeキャッシュを追跡対象から除外`
- `64bb41f fix: Keilログの過去結果による誤判定を防止`

收工，cube