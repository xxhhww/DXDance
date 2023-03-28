#include <Windows.h>
#include "Pool.h"
#include "SegregatedPool.h"
#include "BuddyAllocator.h"
#include "BuddyAllocatorPool.h"

using namespace Tool;

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Pool pool;
    SegregatedPoolBucket<void, int> bucket(1, 2);
    SegregatedPool<int64_t, int64_t> segPool(2);

    auto alloc1 = segPool.Allocate(1);
    alloc1.bucket->userData = 32;
    auto alloc2 = segPool.Allocate(1);
    auto alloc3 = segPool.Allocate(7);
    auto alloc4 = segPool.Allocate(7);
    auto alloc5 = segPool.Allocate(7);

    BuddyAllocator<> buddyAllocator(1, 4);
    auto buddy1 = buddyAllocator.Allocate(1);
    auto buddy2 = buddyAllocator.Allocate(2);
    auto buddy3 = buddyAllocator.Allocate(1);
    buddyAllocator.Deallocate(buddy1);
    buddyAllocator.Deallocate(buddy3);
    buddyAllocator.Deallocate(buddy2);


    BuddyAllocatorPool<> buddyAllocatorPool(1, 4);
    auto buddyPool1 = buddyAllocatorPool.Allocate(1);
    auto buddyPool2 = buddyAllocatorPool.Allocate(2);
    auto buddyPool3 = buddyAllocatorPool.Allocate(4);
    auto buddyPool4 = buddyAllocatorPool.Allocate(1);
    auto buddyPool5 = buddyAllocatorPool.Allocate(2);
    auto buddyPool6 = buddyAllocatorPool.Allocate(4);

    buddyAllocatorPool.Deallocate(buddyPool3);
    buddyAllocatorPool.Deallocate(buddyPool6);
    buddyAllocatorPool.Deallocate(buddyPool2);
    buddyAllocatorPool.Deallocate(buddyPool4);
    buddyAllocatorPool.Deallocate(buddyPool1);
    buddyAllocatorPool.Deallocate(buddyPool5);

    return 0;
}