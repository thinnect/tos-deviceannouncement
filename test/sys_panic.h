/*
 * Mock sys_panic.
 *
 * Copyright Thinnect Inc. 2021
 * @license MIT
 */
#ifndef SYS_PANIC_H
#define SYS_PANIC_H

#define sys_panic(str) \
({                     \
    while(1);          \
})

#endif//SYS_PANIC_H
