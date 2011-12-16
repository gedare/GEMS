#ifndef __GABDEBUG_H_
#define __GABDEBUG_H_

//#define GAB_DEBUG
//#undef GAB_DEBUG

#if defined(GAB_DEBUG)
#include <stdio.h>
#include <rtems/bspIo.h>
#endif

#if defined(GAB_DEBUG)
#define DPRINTF(fmt,...) printf(fmt, ##__VA_ARGS__)
#define DPRINTK(fmt,...) printk(fmt, ##__VA_ARGS__)
#define DPUTS(str) puts((str))
#else
#define DPRINTF(...)
#define DPRINTK(...)
#define DPUTS(str)
#endif

#endif
