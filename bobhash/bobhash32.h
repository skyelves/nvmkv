#ifndef _BOBHASH32_H
#define _BOBHASH32_H
#include <stdio.h>
using namespace std;

typedef unsigned int uint;
typedef unsigned long long int uint64;


#define MAX_PRIME32 1229
#define MAX_BIG_PRIME32 50
#define MAX_PRIME64 1229
#define MAX_BIG_PRIME64 50

class BOBHash32
{
public:
    BOBHash32();
    ~BOBHash32();
    BOBHash32(uint prime32Num);
    void initialize(uint prime32Num);
    uint run(const char * str, uint len);
private:
    uint prime32Num;
};

class BOBHash64
{
public:
    BOBHash64();
    ~BOBHash64();
    BOBHash64(uint prime64Num);
    void initialize(uint prime64Num);
    uint64 run(const char * str, uint len);
private:
    uint prime64Num;
};

#endif //_BOBHASH32_H