diff --git a/runtime/compiler/runtime/IProfiler.cpp b/runtime/compiler/runtime/IProfiler.cpp
index 3e4f2cde14..e4e2834a68 100644
--- a/runtime/compiler/runtime/IProfiler.cpp
+++ b/runtime/compiler/runtime/IProfiler.cpp
@@ -562,8 +562,7 @@ void *
 TR_IProfiler::operator new (size_t size) throw()
    {
    memoryConsumed += (int32_t)size;
-   void *alloc = _allocator->allocate(size, std::nothrow);
-   return alloc;
+   return _allocator->allocate(size, std::nothrow);
    }
 
 TR::PersistentAllocator *
@@ -1128,7 +1127,7 @@ TR_IProfiler::findOrCreateEntry(int32_t bucket, uintptr_t pc, bool addIt)
    TR_IPBytecodeHashTableEntry *headEntry = _bcHashTable[bucket];
    if (headEntry && headEntry->getPC() == pc)
       {
-      // Note: We never delete IP entries
+      delete entry; // Newly allocated entry is not needed
       return headEntry;
       }
 
@@ -2623,33 +2622,17 @@ TR_IProfiler::outputStats()
    checkMethodHashTable();
    }
 
+
 void *
-TR_IPBytecodeHashTableEntry::alignedPersistentAlloc(size_t size)
+TR_IPBytecodeHashTableEntry::operator new (size_t size) throw()
    {
-#if defined(TR_HOST_64BIT)
-   size += 4;
-   memoryConsumed += (int32_t)size;
-   void *address = (void *) TR_IProfiler::allocator()->allocate(size, std::nothrow);
-
-   return (void *)(((uintptr_t)address + 4) & ~0x7);
-#else
    memoryConsumed += (int32_t)size;
    return TR_IProfiler::allocator()->allocate(size, std::nothrow);
-#endif
-   }
-
-
-
-void *
-TR_IPBCDataCallGraph::operator new (size_t size) throw()
-   {
-   return TR_IPBytecodeHashTableEntry::alignedPersistentAlloc(size);
    }
 
-void *
-TR_IPBCDataFourBytes::operator new (size_t size) throw()
+void TR_IPBytecodeHashTableEntry::operator delete(void *p) throw()
    {
-   return TR_IPBytecodeHashTableEntry::alignedPersistentAlloc(size);
+   TR_IProfiler::allocator()->deallocate(p);
    }
 
 #if defined(J9VM_OPT_JITSERVER)
@@ -2707,18 +2690,6 @@ TR_IPBCDataFourBytes::getSumBranchCount()
    return (fallThroughCount + branchToCount);
    }
 
-void *
-TR_IPBCDataAllocation::operator new (size_t size) throw()
-   {
-   return TR_IPBytecodeHashTableEntry::alignedPersistentAlloc(size);
-   }
-
-void *
-TR_IPBCDataEightWords::operator new (size_t size) throw()
-   {
-   return TR_IPBytecodeHashTableEntry::alignedPersistentAlloc(size);
-   }
-
 void
 TR_IPBCDataEightWords::createPersistentCopy(TR_J9SharedCache *sharedCache, TR_IPBCDataStorageHeader *storage, TR::PersistentInfo *info)
    {
@@ -2915,7 +2886,8 @@ TR_IPBCDataCallGraph::getData(TR::Compilation *comp)
 void *
 TR_IPMethodHashTableEntry::operator new (size_t size) throw()
    {
-   return TR_IPBytecodeHashTableEntry::alignedPersistentAlloc(size);
+   memoryConsumed += (int32_t)size;
+   return TR_IProfiler::allocator()->allocate(size, std::nothrow);
    }
 
 int32_t
@@ -4543,8 +4515,7 @@ void *
 TR_IPHashedCallSite::operator new (size_t size) throw()
    {
    memoryConsumed += (int32_t)size;
-   void *alloc = TR_IProfiler::allocator()->allocate(size, std::nothrow);
-   return alloc;
+   return TR_IProfiler::allocator()->allocate(size, std::nothrow);
    }
 
 inline
diff --git a/runtime/compiler/runtime/IProfiler.hpp b/runtime/compiler/runtime/IProfiler.hpp
index e00a95d956..050d4b2697 100644
--- a/runtime/compiler/runtime/IProfiler.hpp
+++ b/runtime/compiler/runtime/IProfiler.hpp
@@ -190,7 +190,11 @@ enum TR_EntryStatusInfo
 class TR_IPBytecodeHashTableEntry
    {
 public:
-   static void* alignedPersistentAlloc(size_t size);
+   void * operator new (size_t size) throw();
+   void operator delete(void *p) throw();
+   void * operator new (size_t size, void * placement) {return placement;}
+   void operator delete(void *p, void *) {}
+
    TR_IPBytecodeHashTableEntry(uintptr_t pc) : _next(NULL), _pc(pc), _lastSeenClassUnloadID(-1), _entryFlags(0), _persistFlags(IPBC_ENTRY_CAN_PERSIST_FLAG) {}
 
    uintptr_t getPC() const { return _pc; }
@@ -298,10 +302,6 @@ class TR_IPBCDataFourBytes : public TR_IPBytecodeHashTableEntry
    {
 public:
    TR_IPBCDataFourBytes(uintptr_t pc) : TR_IPBytecodeHashTableEntry(pc), data(0) {}
-   void * operator new (size_t size) throw();
-   void operator delete(void *p) throw() {}
-   void * operator new (size_t size, void * placement) {return placement;}
-   void operator delete(void *p, void *) {}
 
    static const uint32_t IPROFILING_INVALID = ~0;
    virtual uintptr_t getData(TR::Compilation *comp = NULL) { return (uint32_t)data; }
@@ -329,8 +329,6 @@ class TR_IPBCDataAllocation : public TR_IPBytecodeHashTableEntry
    {
 public:
    TR_IPBCDataAllocation(uintptr_t pc) : TR_IPBytecodeHashTableEntry(pc), clazz(0), method(0), data(0) {}
-   void * operator new (size_t size) throw();
-   void operator delete(void *p) throw() {}
    static const uint32_t IPROFILING_INVALID = ~0;
    virtual uintptr_t getData(TR::Compilation *comp = NULL) { return (uint32_t)data; }
    virtual uint32_t* getDataReference() { return &data; }
@@ -359,10 +357,6 @@ public:
       for (int i = 0; i < SWITCH_DATA_COUNT; i++)
          data[i] = 0;
       };
-   void * operator new (size_t size) throw();
-   void operator delete(void *p) throw() {}
-   void * operator new (size_t size, void * placement) {return placement;}
-   void operator delete(void *p, void *) {}
    static const uint64_t IPROFILING_INVALID = ~0;
    virtual uintptr_t getData(TR::Compilation *comp = NULL) { /*TR_ASSERT(0, "Don't call me, I'm empty"); */return 0;}
    virtual int32_t setData(uintptr_t value, uint32_t freq = 1) { /*TR_ASSERT(0, "Don't call me, I'm empty");*/ return 0;}
@@ -394,10 +388,6 @@ public:
       {
       _csInfo.initialize();
       }
-   void * operator new (size_t size) throw();
-   void operator delete(void *p) throw() {}
-   void * operator new (size_t size, void * placement) {return placement;}
-   void operator delete(void *p, void *) {}
 
    // Set the higher 32 bits to zero under compressedref to avoid assertion in
    // CallSiteProfileInfo::setClazz, which is called by setInvalid with IPROFILING_INVALID
