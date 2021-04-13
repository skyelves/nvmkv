//
// Created by 王柯 on 4/13/21.
//

#ifndef NVMKV_UTIL_H
#define NVMKV_UTIL_H

#include <stdio.h>
#include <iostream>
#include <sys/time.h>

#define PROFILE 1

#if (PROFILE == 1)
timeval start_time, end_time;
uint64_t t1 = 0, t2 = 0, t3 = 0;
#endif

class util {

};


#endif //NVMKV_UTIL_H
