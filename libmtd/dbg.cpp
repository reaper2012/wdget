#include "matrix.h"

#undef new
#undef delete

namespace matrix {

void dbgout( const char* fmt, ... )
{
    char str[4096];
    va_list v;
    va_start( v, fmt );
    vsprintf( str, fmt, v );
    strcat( str, "\n" );
    fputs( str, stderr );
}

#ifndef NDEBUG
/*
 * Heap management using operator new/delete.  These routines use 
 * unbalanced binary trees to keep track of allocations in an attempt 
 * to make them fast yet simple.
 *
 * The trees for scalar and vector allocations are completely separate so
 * that trying to deallocate memory with the wrong version of operator
 * delete will always fail.
 *
 * Global operator new is redefined to pass the file and line where the
 * allocation occurred to facilitate memory leak debugging.  Unfortunately,
 * it doesn't seem to be possible to redefine operator delete.
 *
 * Each block of memory consists of an alloc_node header, the requested
 * memory block, and two-byte guards before and after the requested memory
 * block.  The requested memory block is filled with a semi-random byte
 * value to ensure that the caller does not rely on any particular initial
 * bit pattern (eg. a block of zeros or NULLs).  It is refilled with a
 * (possibly different) byte value after deallocation to ensure that the
 * caller doesn't attempt to use the freed memory.
 *
 * NOTES:
 * It is perfectly valid to allocate a block of length zero.  The returned
 * block should have a length of 1.  This simplifies algorithms, especially
 * those that do vector allocation based on an unsigned input parameter.  It
 * is also valid to delete a NULL block.  This simplifies object destruction
 * by alleviating the need for if() guards.  These rules have been around
 * since C++ was born but most people don't seem to know or care.
 */

struct alloc_node
{
    alloc_node* lptr;
    alloc_node* rptr;
    size_t      len;
    CPCHAR      file;
    UINT        line;
};

static alloc_node* g_heap = NULL;
static alloc_node* g_vector_heap = NULL;

// Our magic guard bytes
static BYTE g_guard[] =
{
    0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
    0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF
};

void* operator new( size_t n, CPCHAR file, UINT line )
{
    BYTE* pmem = NULL;
    if( !n ) n = 1;
    alloc_node* pnode = (alloc_node*)malloc( n + 2*sizeof(g_guard) + sizeof(alloc_node) );
    if( pnode )
    {
        pmem = (BYTE*)pnode + sizeof(alloc_node) + sizeof(g_guard);
        memcpy( pmem - sizeof(g_guard), g_guard, sizeof(g_guard) );
        memset( pmem, time(NULL), n );
        memcpy( pmem + n, g_guard, sizeof(g_guard) );

        pnode->lptr = pnode->rptr = NULL;
        pnode->len = n;
        pnode->file = file;
        pnode->line = line;
        alloc_node** ppuplink = &g_heap;
        alloc_node* pcur = g_heap;
        while( pcur )
        {
            if( pnode == pcur )
            {
                dbgout( "*** FATAL: duplicate memory allocated ***" );
                assert( false );
                exit( -1 );
            }
            if( pnode < pcur )
            {
                ppuplink = &pcur->lptr;
                pcur = pcur->lptr;
            }
            else
            {
                ppuplink = &pcur->rptr;
                pcur = pcur->rptr;
            }
        }
        *ppuplink = pnode;
    }

    return pmem;
}

void* operator new[]( size_t n, CPCHAR file, UINT line )
{
    BYTE* pmem = NULL;
    if( !n ) n = 1;
    alloc_node* pnode = (alloc_node*)malloc( n + 2*sizeof(g_guard) + sizeof(alloc_node) );
    if( pnode )
    {
        pmem = (BYTE*)pnode + sizeof(alloc_node) + sizeof(g_guard);
        memcpy( pmem - sizeof(g_guard), g_guard, sizeof(g_guard) );
        memset( pmem, time(NULL), n );
        memcpy( pmem + n, g_guard, sizeof(g_guard) );

        pnode->lptr = pnode->rptr = NULL;
        pnode->len = n;
        pnode->file = file;
        pnode->line = line;
        alloc_node** ppuplink = &g_vector_heap;
        alloc_node* pcur = g_vector_heap;
        while( pcur )
        {
            if( pnode == pcur )
            {
                dbgout( "*** FATAL: duplicate memory allocated ***" );
                assert( false );
                exit( -1 );
            }
            if( pnode < pcur )
            {
                ppuplink = &pcur->lptr;
                pcur = pcur->lptr;
            }
            else
            {
                ppuplink = &pcur->rptr;
                pcur = pcur->rptr;
            }
        }
        *ppuplink = pnode;
    }

    return pmem;
}

void operator delete( void* p )
{
    if( !p ) return;
    if( !g_heap )
    {
        dbgout( "*** FATAL: delete with empty heap ***" );
        assert( false );
        exit( -1 );
    }

    alloc_node* pcur = g_heap;
    alloc_node** ppuplink = &g_heap;
    while( pcur )
    {
        void* pcurblk = (char*)pcur + sizeof(alloc_node) + sizeof(g_guard);
        if( p == pcurblk )
        {
            BYTE* pmem = (BYTE*)p;
            if( memcmp( pmem - sizeof(g_guard), g_guard, sizeof(g_guard) ) != 0 ||
                memcmp( pmem + pcur->len, g_guard, sizeof(g_guard) ) != 0 )
            {
                dbgout( "*** FATAL: corrupted memory at %08X", p );
                assert( false );
                exit( -1 );
            }
            memset( pmem, time(NULL), pcur->len );
            if( pcur->lptr && pcur->rptr )
            {
                // node has both ptrs so replace it with left child and move
                // right child to bottom right of left child's tree
                alloc_node* pend = pcur->lptr;
                while( pend->rptr ) pend = pend->rptr;
                *ppuplink = pcur->lptr;
                pend->rptr = pcur->rptr;
            }
            else
            {
                // move child up
                *ppuplink = (pcur->lptr) ? pcur->lptr : pcur->rptr;
            }
            free( pcur );
            return;
        }
        if( p < pcurblk )
        {
            ppuplink = &pcur->lptr;
            pcur = pcur->lptr;
        }
        else
        {
            ppuplink = &pcur->rptr;
            pcur = pcur->rptr;
        }
    }

    dbgout( "*** FATAL: delete on unalloced memory ***" );
    assert( false );
    exit( -1 );
}

void operator delete[]( void* p )
{
    if( !p ) return;
    if( !g_vector_heap )
    {
        dbgout( "*** FATAL: delete with empty heap ***" );
        assert( false );
        exit( -1 );
    }

    alloc_node* pcur = g_vector_heap;
    alloc_node** ppuplink = &g_vector_heap;
    while( pcur )
    {
        void* pcurblk = (char*)pcur + sizeof(alloc_node) + sizeof(g_guard);
        if( p == pcurblk )
        {
            BYTE* pmem = (BYTE*)p;
            if( memcmp( pmem - sizeof(g_guard), g_guard, sizeof(g_guard) ) != 0 ||
                memcmp( pmem + pcur->len, g_guard, sizeof(g_guard) ) != 0 )
            {
                dbgout( "*** FATAL: corrupted memory at %08X", p );
                assert( false );
                exit( -1 );
            }
            memset( pmem, time(NULL), pcur->len );
            if( pcur->lptr && pcur->rptr )
            {
                // node has both ptrs so replace it with left child and move
                // right child to bottom right of left child's tree
                alloc_node* pend = pcur->lptr;
                while( pend->rptr ) pend = pend->rptr;
                *ppuplink = pcur->lptr;
                pend->rptr = pcur->rptr;
            }
            else
            {
                // move child up
                *ppuplink = (pcur->lptr) ? pcur->lptr : pcur->rptr;
            }
            free( pcur );
            return;
        }
        if( p < pcurblk )
        {
            ppuplink = &pcur->lptr;
            pcur = pcur->lptr;
        }
        else
        {
            ppuplink = &pcur->rptr;
            pcur = pcur->rptr;
        }
    }

    dbgout( "*** FATAL: delete on unalloced memory ***" );
    assert( false );
    exit( -1 );
}

void* operator new( size_t n )
{
    return ::operator new( n, "(unknown)", 0 );
}

void* operator new[]( size_t n )
{
    return ::operator new[]( n, "(unknown)", 0 );
}

static void walk_alloc_tree( alloc_node* pcur, size_t* pttl )
{
    if( pcur )
    {
        walk_alloc_tree( pcur->lptr, pttl );
        dbgout( "%s(%u): %u bytes at %08X", pcur->file, pcur->line,
                pcur->len, (char*)pcur + sizeof(alloc_node) );
        *pttl += pcur->len;
        walk_alloc_tree( pcur->rptr, pttl );
    }
}

void dump_alloc_heaps( void )
{
    if( g_heap || g_vector_heap )
    {
        size_t ttl = 0;
        dbgout( "Memory leaks detected" );
        dbgout( "=====================" );
        dbgout( "" );

        if( g_heap )
        {
            dbgout( "Scalar objects" );
            dbgout( "--------------" );
            walk_alloc_tree( g_heap, &ttl );
            dbgout( "" );
        }
        if( g_vector_heap )
        {
            dbgout( "Vector objects" );
            dbgout( "--------------" );
            walk_alloc_tree( g_vector_heap, &ttl );
            dbgout( "" );
        }

        dbgout( "=====================" );
        dbgout( "Total bytes: %u", ttl );
        dbgout( "=====================" );
    }
}

#endif

}
