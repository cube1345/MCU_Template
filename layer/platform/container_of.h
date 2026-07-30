/**
 * @file    container_of.h
 * @brief   OOP-in-C 基类与具体对象之间的安全布局转换辅助宏。
 * @author  weixin
 */
#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#include <stddef.h>

/*
 * 从嵌入式基类指针恢复具体对象指针。
 *
 * 约束：
 * - ptr 必须真实指向 type 对象中的 member 成员。
 * - member 通常是派生结构体的第一个成员，用于稳定向上转型。
 * - 不要把带副作用的表达式传给 ptr，避免调试困难。
 *
 * 该宏只用于同一对象生命周期内的合法基类地址；它不负责检查 NULL，
 * 也不能把任意类型的指针转换成具体对象。
 */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif /* CONTAINER_OF_H */
