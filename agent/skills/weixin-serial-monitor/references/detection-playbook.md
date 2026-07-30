# 工程内诊断流程

## 1. 确认范围

Record the project root, selected runtime, expected port/baud (if documented), firmware protocol (if documented), initial connection state, and whether the user requested observation or connection control. Never treat "diagnose" as permission to modify the device.

## 2. Prove bridge health

1. Run `discover` and record the executable/runtime candidate it reports.
2. Confirm that candidate matches the GUI-selected runtime. Pass an exact `--runtime` when it does not.
3. Start with `launch --wait` only if the GUI is not already using that runtime.
4. Run `status`; on timeout, stop and report the runtime mismatch or inactive GUI.
5. Run `refresh-ports` only after the master bridge is enabled.

## 3. Observe without changing the device

1. If status says connected, read up to 100 RX log entries.
2. Filter a second read only for a specific symptom, for example `error`.
3. Read `summary --max-age 5`; distinguish absent, invalid, stale, and fresh empty-channel data.
4. Do not clear logs/plots or export files as routine diagnosis.

## 4. Inspect a known compatible protocol

1. Require a documented compatible firmware protocol and **Read device data** permission.
2. Require connected status before protocol actions.
3. Run `--timeout 15 protocol-check --wait-completion`; do not treat the initial acknowledgement as completion.
4. Run `scan-params --wait-completion`; treat a settled count as discovery evidence, not full semantic validation.
5. Run `param-get` only for documented parameter names.

If this workflow established a connection that did not exist initially, run `disconnect` and confirm the disconnected state with `status`. Preserve an existing connection unless the user requested a change.

## 5. Report

Return the app/runtime, connection state, ports found, bounded observations, completed protocol checks, denied permissions, and the next safe action. Do not paste full raw logs unless explicitly requested.
