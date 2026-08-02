/**
 * @file    my_freetros_lib.c
 * @brief   FreeRTOS 任务、CPU 占用率和堆内存监控实现。
 * @author  weixin
 */
#include "my_freetros_lib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx.h"

#include <stdint.h>
#include <stdio.h>

#include "app_log.h"

/* 单次快照支持的最大任务数量，超过上限时明确报告，不输出不完整数据。 */
#define FREERTOS_MONITOR_MAX_TASKS UINT32_C(24)

/* 百分比以 0.01% 为一个单位，避免在监控路径中引入浮点运算。 */
#define FREERTOS_MONITOR_PERCENT_SCALE UINT64_C(10000)

/* 堆历史最小剩余量低于该值时输出告警，单位为字节。 */
#define FREERTOS_MONITOR_HEAP_WARNING_BYTES ((size_t)128U)

/**
 * @brief 保存一个任务在上次监控快照中的累计运行时间。
 */
struct freertos_monitor_task_snapshot {
  TaskHandle_t handle;             /* 任务句柄，用于跨快照匹配同一个任务。 */
  uint64_t run_time_cycles;        /* 上次快照时累计占用的 CPU 周期数。 */
};

/* DWT 低 32 位计数器上次读数，用于识别一次硬件回绕。 */
static uint32_t s_last_dwt_cycles;

/* 将 DWT 的多次 32 位回绕累计成长期单调递增的 64 位周期数。 */
static uint64_t s_total_dwt_cycles;

/* 当前任务状态快照，由静态存储提供，监控过程不改变堆内存水位。 */
static TaskStatus_t s_task_status[FREERTOS_MONITOR_MAX_TASKS];

/* 上一次报告的逐任务快照，用于计算窗口 CPU 占用率。 */
static struct freertos_monitor_task_snapshot
    s_last_task_snapshot[FREERTOS_MONITOR_MAX_TASKS];

/* 上一次快照中有效的任务记录数量。 */
static UBaseType_t s_last_task_count;

/* 上一次报告时的系统总运行周期数，用于确定统计窗口长度。 */
static uint64_t s_last_total_run_time_cycles;

/**
 * @brief  初始化 FreeRTOS 运行时间统计使用的 DWT 周期计数器。
 * @retval None
 * @note   由 FreeRTOS 在启动调度器时调用，应用层不应重复调用。
 */
void freertos_run_time_stats_init(void)
{
  uint32_t primask;

  /* 初始化过程必须与运行时间读取互斥，避免产生不一致的扩展计数。 */
  primask = __get_PRIMASK();
  __disable_irq();

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  s_last_dwt_cycles = 0U;
  s_total_dwt_cycles = UINT64_C(0);

  __set_PRIMASK(primask);
}

/**
 * @brief  获取从调度器启动以来累计经过的 CPU 周期数。
 * @return 单调递增的 64 位 CPU 周期数。
 * @note   可在任务切换中断和任务上下文调用；相邻调用间隔必须小于一次
 *         DWT 回绕时间。在 168 MHz 下，一次回绕约为 25.6 秒。
 */
uint64_t freertos_run_time_stats_get_cycles(void)
{
  uint32_t primask;
  uint32_t current_cycles;
  uint32_t elapsed_cycles;
  uint64_t total_cycles;

  /*
   * DWT 只有 32 位。通过无符号减法处理一次自然回绕，并在短临界区内
   * 更新 64 位软件累计值，防止任务查询与 PendSV 切换同时修改状态。
   */
  primask = __get_PRIMASK();
  __disable_irq();

  current_cycles = DWT->CYCCNT;
  elapsed_cycles = current_cycles - s_last_dwt_cycles;
  s_total_dwt_cycles += (uint64_t)elapsed_cycles;
  s_last_dwt_cycles = current_cycles;
  total_cycles = s_total_dwt_cycles;

  __set_PRIMASK(primask);
  return total_cycles;
}

/**
 * @brief  将 FreeRTOS 任务状态转换为简短文本。
 * @param  state：uxTaskGetSystemState() 返回的任务状态。
 * @return 永久有效的只读状态文本。
 */
static const char *freertos_monitor_state_name_get(eTaskState state)
{
  switch (state) {
  case eRunning:
    return "RUN";
  case eReady:
    return "RDY";
  case eBlocked:
    return "BLK";
  case eSuspended:
    return "SUS";
  case eDeleted:
    return "DEL";
  default:
    return "UNK";
  }
}

/**
 * @brief  查找指定任务在上一次快照中的累计运行周期数。
 * @param  handle：需要查询的任务句柄。
 * @return 找到时返回上次累计周期数；新任务或无效句柄返回 0。
 */
static uint64_t freertos_monitor_last_run_time_get(TaskHandle_t handle)
{
  UBaseType_t index;

  for (index = 0U; index < s_last_task_count; index++) {
    if (s_last_task_snapshot[index].handle == handle) {
      return s_last_task_snapshot[index].run_time_cycles;
    }
  }

  return UINT64_C(0);
}

/**
 * @brief  计算运行时间相对于总时间的百分比，结果保留两位小数。
 * @param  run_time：任务运行周期数。
 * @param  total_time：对应统计区间的总周期数。
 * @return 以 0.01% 为单位的百分比；总时间为 0 时返回 0。
 */
static uint32_t freertos_monitor_percent_x100_get(uint64_t run_time,
                                                   uint64_t total_time)
{
  uint64_t scaled_percent;

  if (total_time == UINT64_C(0)) {
    return UINT32_C(0);
  }

  /* 正常快照中任务时间不会超过系统总时间，异常输入按 100% 截断。 */
  if (run_time >= total_time) {
    return (uint32_t)FREERTOS_MONITOR_PERCENT_SCALE;
  }

  if (run_time <= (UINT64_MAX / FREERTOS_MONITOR_PERCENT_SCALE)) {
    scaled_percent = (run_time * FREERTOS_MONITOR_PERCENT_SCALE) /
                     total_time;
  } else {
    /*
     * 系统长期运行后直接乘以 10000 可能溢出。此时先缩小分母，最多
     * 损失末尾一个 0.01% 计数，并通过下方截断保证结果不超过 100%。
     */
    scaled_percent = run_time /
                     (total_time / FREERTOS_MONITOR_PERCENT_SCALE);
  }

  if (scaled_percent > FREERTOS_MONITOR_PERCENT_SCALE) {
    scaled_percent = FREERTOS_MONITOR_PERCENT_SCALE;
  }
  return (uint32_t)scaled_percent;
}

/**
 * @brief  将 CPU 周期数转换为微秒。
 * @param  cycles：待转换的 CPU 周期数。
 * @return 按当前 SystemCoreClock 换算的微秒数；时钟无效时返回 0。
 */
static uint64_t freertos_monitor_cycles_to_us(uint64_t cycles)
{
  if (SystemCoreClock == 0U) {
    return UINT64_C(0);
  }

  /* 分段计算可避免 cycles * 1000000 在长期运行后溢出。 */
  return ((cycles / SystemCoreClock) * UINT64_C(1000000)) +
         (((cycles % SystemCoreClock) * UINT64_C(1000000)) /
          SystemCoreClock);
}

/**
 * @brief  保存当前任务运行时间，供下一次报告计算窗口占用率。
 * @param  task_status：当前任务状态数组。
 * @param  task_count：数组中的有效任务数量。
 * @retval None
 */
static void freertos_monitor_snapshot_save(const TaskStatus_t *task_status,
                                            UBaseType_t task_count)
{
  UBaseType_t index;

  for (index = 0U; index < task_count; index++) {
    s_last_task_snapshot[index].handle = task_status[index].xHandle;
    s_last_task_snapshot[index].run_time_cycles =
        task_status[index].ulRunTimeCounter;
  }

  /* 清除已经删除任务留下的尾部记录，避免句柄重用后错误匹配。 */
  for (index = task_count; index < s_last_task_count; index++) {
    s_last_task_snapshot[index].handle = NULL;
    s_last_task_snapshot[index].run_time_cycles = UINT64_C(0);
  }

  s_last_task_count = task_count;
}

/**
 * @brief  打印 FreeRTOS 任务运行状态、CPU 占用率和堆内存统计信息。
 * @retval None
 * @note   Avg% 为系统启动以来的累计平均 CPU 占用率。
 * @note   Win% 为本次调用与上次调用之间的窗口 CPU 占用率。
 * @note   只能从任务上下文调用，本函数不可重入，获取快照时会短暂挂起调度器。
 */
void Show_System_Stats(void)
{
  UBaseType_t task_count;
  UBaseType_t index;
  uint64_t total_run_time_cycles;
  uint64_t window_total_cycles;
  uint64_t total_run_time_us;
  size_t free_heap;
  size_t min_free_heap;

  task_count = uxTaskGetNumberOfTasks();
  if (task_count > FREERTOS_MONITOR_MAX_TASKS) {
    printf("\r\n[freertos] monitor capacity exceeded: task=%lu max=%lu\r\n",
           (unsigned long)task_count,
           (unsigned long)FREERTOS_MONITOR_MAX_TASKS);
    return;
  }

  total_run_time_cycles = UINT64_C(0);
  task_count = uxTaskGetSystemState(s_task_status,
                                    FREERTOS_MONITOR_MAX_TASKS,
                                    &total_run_time_cycles);
  if (task_count == 0U) {
    printf("\r\n[freertos] failed to capture task state\r\n");
    return;
  }

  if (total_run_time_cycles >= s_last_total_run_time_cycles) {
    window_total_cycles = total_run_time_cycles -
                          s_last_total_run_time_cycles;
  } else {
    /* 该分支仅用于防御计时源被外部重新初始化的异常情况。 */
    window_total_cycles = total_run_time_cycles;
  }
  total_run_time_us = freertos_monitor_cycles_to_us(total_run_time_cycles);
  (void)total_run_time_us;

  printf("\r\n====================== FreeRTOS Monitor ======================\r\n");
  printf("System Total RunTime: %llu us\r\n",
         (unsigned long long)total_run_time_us);
  printf("--------------------------------------------------------------\r\n");
  printf("%-12s %-6s %-5s %-8s %-12s %-8s %-8s\r\n",
         "Name", "State", "Prio", "Stack", "AbsTime", "Avg%", "Win%");
  printf("--------------------------------------------------------------\r\n");

  for (index = 0U; index < task_count; index++) {
    uint64_t last_task_cycles;
    uint64_t window_task_cycles;
    uint64_t task_run_time_us;
    uint32_t avg_percent_x100;
    uint32_t win_percent_x100;

    last_task_cycles =
        freertos_monitor_last_run_time_get(s_task_status[index].xHandle);
    if (s_task_status[index].ulRunTimeCounter >= last_task_cycles) {
      window_task_cycles = s_task_status[index].ulRunTimeCounter -
                           last_task_cycles;
    } else {
      /* 任务句柄被重用时，将当前任务视作本窗口内新创建的任务。 */
      window_task_cycles = s_task_status[index].ulRunTimeCounter;
    }

    task_run_time_us = freertos_monitor_cycles_to_us(
        s_task_status[index].ulRunTimeCounter);
    avg_percent_x100 = freertos_monitor_percent_x100_get(
        s_task_status[index].ulRunTimeCounter, total_run_time_cycles);
    win_percent_x100 = freertos_monitor_percent_x100_get(
        window_task_cycles, window_total_cycles);
    (void)task_run_time_us;
    (void)avg_percent_x100;
    (void)win_percent_x100;

    printf("%-12s %-6s %-5lu %-8lu %-12llu %3lu.%02lu%% %3lu.%02lu%%\r\n",
           s_task_status[index].pcTaskName,
           freertos_monitor_state_name_get(
               s_task_status[index].eCurrentState),
           (unsigned long)s_task_status[index].uxCurrentPriority,
           (unsigned long)s_task_status[index].usStackHighWaterMark,
           (unsigned long long)task_run_time_us,
           (unsigned long)(avg_percent_x100 / UINT32_C(100)),
           (unsigned long)(avg_percent_x100 % UINT32_C(100)),
           (unsigned long)(win_percent_x100 / UINT32_C(100)),
           (unsigned long)(win_percent_x100 % UINT32_C(100)));
  }

  freertos_monitor_snapshot_save(s_task_status, task_count);
  s_last_total_run_time_cycles = total_run_time_cycles;

  free_heap = xPortGetFreeHeapSize();
  min_free_heap = xPortGetMinimumEverFreeHeapSize();
  (void)free_heap;

  printf("--------------------------------------------------------------\r\n");
  printf("Memory Summary:\r\n");
  printf("   > Current Free : %lu Bytes\r\n", (unsigned long)free_heap);
  printf("   > Historic Min : %lu Bytes (MinFree)\r\n",
         (unsigned long)min_free_heap);

  if (min_free_heap < FREERTOS_MONITOR_HEAP_WARNING_BYTES) {
    printf("WARNING: System memory is critically low!\r\n");
  }

  printf("==============================================================\r\n");
}
