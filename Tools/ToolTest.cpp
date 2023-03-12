#include <Windows.h>
#include "Pool.h"
#include "SegregatedPool.h"

using namespace Tool;

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Pool pool;
    SegregatedPoolBucket<void, int> bucket(1, 2);
    SegregatedPool<int64_t, int64_t> segPool(2);

    auto alloc1 = segPool.Allocate(1);
    alloc1.bucket.userData = 32;
    auto alloc2 = segPool.Allocate(1);
    auto alloc3 = segPool.Allocate(7);
    auto alloc4 = segPool.Allocate(7);
    auto alloc5 = segPool.Allocate(7);

    return 0;
}