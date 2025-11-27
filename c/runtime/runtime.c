#include "runtime.h"
#include "ordered-map.h"
#include <gc.h>


#include <execinfo.h>
void printBacktrace(FILE * file) {

    void * frames[10];
    char ** lines;
  
    int numEntries = backtrace (frames, 10);
    lines = backtrace_symbols (frames, numEntries);
    if (lines != NULL) {
        fprintf (file, "Backtrace Num Entries: %d\n", numEntries);
        for (int i = 0; i < numEntries; i++) {
            fprintf (file, "Stack Frame[%d]: %s\n", i, lines[i]);
        }
    }
  
    free (lines);
}



__attribute__((noreturn))
void fatalError(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fflush(stdout);
    fprintf(stderr, "\n\n");
    fprintf(stderr, "FATAL ERROR: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
    printBacktrace(stderr);
    fprintf(stderr, "\n\n");
    // abort(); // dump a core file
    exit(1);
}

// #define debugf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#define debugf(fmt, ...)

#ifndef SAFETY_CHECK_ERROR
#define SAFETY_CHECK_ERROR(a) (a)
// #define SAFETY_CHECK_ERROR(a) (0)
// #define SAFETY_CHECK_ERROR(a) (__builtin_expect(a, 0))
#endif

#ifndef SAFETY_CHECK_OK
#define SAFETY_CHECK_OK(a) (a)
// #define SAFETY_CHECK_OK(a) (1)
// #define SAFETY_CHECK_OK(a) (__builtin_expect(a, 1))
// #define SAFETY_CHECK_OK(a) (!(SAFETY_CHECK_ERROR(!(a))))
#endif

#ifndef SAFETY_CHECK_ASSERT
#define SAFETY_CHECK_ASSERT(a) if ((a) != 0) { fatalError("Safety Check Assertion Failed: %s %d %s", __FILE__, __LINE__, #a); }
#endif

// #ifndef INLINE
#define INLINE 
// #define INLINE inline
// #endif 



long mallocCounter = 0;

void checkLargeMalloc(size_t size) {
    // if (size > 1024 * 1024) {
    //     fprintf(stderr, "Large Malloc %0.6f MB\n", ((float)size) / 1000 / 1000);
    //     printBacktrace(stderr);
    //     fflush(stderr);
    // }
}

// perform all mallocs in one place, and check for failure
void *malloc_or_panic(size_t size) {
    checkLargeMalloc(size);
    // void *ptr = malloc(size);
    void *ptr = GC_malloc(size);
    // The use of the "_ignore_off_page" variant is ok if:
    //   - a pointer to the start of block always exists if any pointer to the block exists.
    // Not sure that this is / will always be true,
    //   but at present it seems to work in practice.
    // void *ptr = GC_malloc_ignore_off_page(size);
    if (SAFETY_CHECK_ERROR(ptr == NULL)) {
        fatalError("malloc failed");
    }
    mallocCounter += 1;
    return ptr;
}

void *malloc_atomic_or_panic(size_t size) {
    checkLargeMalloc(size);
    // void *ptr = malloc(size);
    // void *ptr = GC_malloc(size);
    // void *ptr = GC_malloc_atomic(size);
    // void *ptr = GC_malloc_ignore_off_page(size);
    void *ptr = GC_malloc_atomic_ignore_off_page(size);
    if (SAFETY_CHECK_ERROR(ptr == NULL)) {
        fatalError("malloc failed");
    }
    mallocCounter += 1;
    return ptr;
}

void *realloc_or_panic(void *ptr, size_t size) {
    // ptr = realloc(ptr, size);
    ptr = GC_realloc(ptr, size);
    if (SAFETY_CHECK_ERROR(ptr == NULL)) {
        fatalError("realloc failed");
    }
    return ptr;
}


const void * mallocValue(Repr repr, const void * value) {
    if (repr->size == 0) {
        return NULL;
    }
    void * ptr = malloc_or_panic(repr->size);
    memcpy(ptr, value, repr->size);
    return ptr;
}


/*
#define MALLOC(type, name, ...) \
      type *name = malloc_or_panic(sizeof(type)); \
      *name = (type)__VA_ARGS__;
*/

// const char *inspect_malloc(void *ptr) {
//     char *ptr2 = ptr;
//     ptr2 -= sizeof(max_align_t);
//     char **ptr3 = (void*) ptr2;
//     char *info = *ptr3;
//     return info;
// }

// void typeError(const char *msg, const char *expectedType, const char *actualType) {
//     fprintf(stderr, "TypeError: %s %s %s\n", msg, expectedType, actualType);
// }

// void *void_ptr_as(const char *typeName, void *ptr) {
//     const char *info = inspect_malloc(ptr);
//     if (strcmp(typeName, info) != 0) {
//         typeError("void_ptr_as", typeName, info);
//     }
//     return ptr;
// }

// perform all frees in one place, (or not at all if using GC)
void freePtr(const void *ptr) {
    // free((void*)ptr);
    // GC_free((void*)ptr);
}

#define VOID_PTR_ADD(ptr, num, size) ((void*)((char*)(ptr) + (num) * (size)))

#define MEMCPY(toBase, toOffset, fromBase, fromOffset, numElems, elemSize) \
    memcpy(VOID_PTR_ADD(toBase, toOffset, elemSize), VOID_PTR_ADD(fromBase, fromOffset, elemSize), numElems * elemSize)


void printDiagnostics() {
    fflush(stdout);
    fprintf(stderr, "Malloc Counter: %ld\n", mallocCounter);
    fflush(stderr);
}




// TODO ? store nil, booleans and 63-bit integers directly in the Refs ?
// TODO ? store short strings / characters directly in Ref
// TODO ?   strings of length 0 and 1 make most sense, but strings of length 7 could work (for 8-bit chars, less for wider chars).
// TODO ? interned strings, strings typically used as enums, 
// TODO ?   we can just store an integer key into a string-intern table.
// TODO ?   this would make for quick string comparisons
// TODO ? include an flag in the Ref to indicate the value has been hash-consed already ?
// TODO ?   this would save a check and make for faster construction and comparisons

// TypeHdr anyType = { { Ty_Hdr }, NULL }; // a type that contains any value (as long as it starts with a Header)
// ReprHdr anyRepr = { { Repr_Hdr } }; // a type that contains any value (as long as it starts with a Header)

ReprAny anyRepr = { { Repr_Any, sizeof(Any) } };
ReprNo noRepr = { { Repr_No, sizeof(No) } };
ReprBool boolRepr = { { Repr_Bool, sizeof(bool) } };
ReprInteger intRepr = { { Repr_Int, sizeof(int) }, true, sizeof(int) };
ReprString strRepr = { { Repr_Str, sizeof(Str) } };
ReprChar charRepr = { { Repr_Char, sizeof(Char) } };
ReprType typeRepr = { { Repr_Type, sizeof(Type) } };
ReprPair pairRepr = { { Repr_Pair, sizeof(Pair) } };
ReprTupleTail tupleTailRepr = { { Repr_TupleTail, sizeof(TupleTail) } };
ReprObject objectRepr = { { Repr_Object, sizeof(Object) } };

// Object objectMk(void *env, Any initState) {
//     Any methods = *(Any*) env;
//     MALLOC(Any, state, initState);
//     Object object = { state, methods };
//     return object;
// }

// Object objectMk_fun(void const * const objectMk_env, Any initState) {
//     FunAnyAny handlerMk = *(FunAnyAny*) objectMk_env;
//     MALLOC(Any, state, initState);
//     // Any handler = any_call(handlerMk, initState);
//     Any handler = handlerMk.func(handlerMk.env, initState);
//     MALLOC(Any, handlerPtr, handler);
//     // Object object = { state, methods };
//     Object object = { handlerPtr };
//     return object;
// }

// ObjectMk objectMk_from_handlerMk(FunAnyAny handlerMk) {
//     MALLOC(FunAnyAny, objectMk_env, handlerMk);
//     ObjectMk om = { objectMk_fun, objectMk_env };
//     return om;
// }




// Any f (Any a, Any b)
Any adaptFunc_AnyAny_to_Any(const void *nativeFunc, const void *closEnv, Any partialArgs[], Any lastArg) {
    Any (*func)(Any, Any) = nativeFunc;
    Any result = func(partialArgs[0], lastArg);
    return result;
}
ReprFunc fn_pair_any_any_to_any_Repr = {
    { Repr_Func },
    2, 
    (Repr[2]){ &anyRepr.base, &anyRepr.base },
    &anyRepr.base,
    adaptFunc_AnyAny_to_Any,
    NULL,

};
Any adaptFunction_AnyAny_to_Any(Any (*nativeFunc)(Any, Any)) {
    Any result = any_from_func(&fn_pair_any_any_to_any_Repr, nativeFunc);
    return result;
}



// Any f (void *env , Any a)
Any adaptClos_Any_to_Any(const void *nativeFunc, const void *closEnv, Any partialArgs[], Any lastArg) {
    Any (*func)(const void *, Any) = nativeFunc;
    Any arg0 = lastArg;
    Any result = func(closEnv, arg0);
    return result;
}
Any adaptClosInv_Any_to_Any (void * closEnv, Any arg_0) {
    Any arg0 = arg_0;
    AnyFuncClosEnv * anyFuncClosEnv = closEnv;
    Any func = anyFuncClosEnv->func;
    Any result = any_call(func, arg0);    
    return result;
}
ReprClos anyToAny_Repr = {
    { Repr_Clos },
    1, 
    (Repr[1]){ &anyRepr.base },
    &anyRepr.base,
    adaptClos_Any_to_Any,
    adaptClosInv_Any_to_Any
};
Any adaptClosure_Any_to_Any(Any (*nativeFunc)(void *, Any), void *closEnv) {
    Any result = any_from_clos(&anyToAny_Repr, nativeFunc, closEnv);
    return result;
}


// Any f (void *env , Str a)
Any adaptClos_Str_to_Any(const void *nativeFunc, const void *closEnv, Any partialArgs[], Any lastArg) {
    Any (*func)(const void *, Str) = nativeFunc;
    Str arg0 = any_to_str(lastArg);
    Any result = func(closEnv, arg0);
    return result;
}
Any adaptClosInv_Str_to_Any (void * closEnv, Str arg_0) {
    Any arg0 = any_from_str(arg_0);
    AnyFuncClosEnv * anyFuncClosEnv = closEnv;
    Any func = anyFuncClosEnv->func;
    Any result = any_call(func, arg0);    
    return result;
}
ReprClos strToAny_Repr = {
    { Repr_Clos },
    1, 
    (Repr[1]){ &strRepr.base },
    &anyRepr.base,
    adaptClos_Str_to_Any,
    adaptClosInv_Str_to_Any
};
Any adaptClosure_Str_to_Any(Any (*nativeFunc)(void *, Str), void *closEnv) {
    Any result = any_from_clos(&strToAny_Repr, nativeFunc, closEnv);
    return result;
}







size_t max(size_t a, size_t b) {
    return a > b ? a : b;
}

size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}


// typedef struct {
//     Ref ref;
// } HashConsTableEntry;

// typedef struct {
//     unsigned idx;
// } HashConsMapEntry;

// HashConsTableEntry *hashConsTable;

// uint64_t hashConsNextIdx = 0;
// // uint64_t hashConsTableSize = 0;
// uint64_t hashConsTableCapacity = 0;

// OrderedMapPtr hashConsMap;

// Ref hashConsRef(Ref a) {
//     if (((uint64_t)a & 1) != 0) {
//         return a;
//     }
//     uint64_t aBits = (uint64_t) omap_get(hashConsMap, a);
//     if (aBits == 0) {
//         uint64_t aIdx = hashConsNextIdx ++;
//         if (aIdx >= hashConsTableCapacity) {
//             hashConsTableCapacity = max(16, 2 * hashConsTableCapacity);
//             hashConsTable = realloc_or_panic(hashConsTable, sizeof(HashConsTableEntry) * hashConsTableCapacity);
//         }
//         hashConsTable[aIdx].ref = a;
//         aBits = (aIdx << 1) | 1;
//         omap_set(hashConsMap, a, (Ref)aBits);
//     }
//     return (Ref) aBits;
// }

// Ref getRef (Ref ref) {
//     uint64_t refBits = (uint64_t) ref;
//     if ((refBits & 1) != 0) {
//         uint64_t idx = refBits >> 1;
//         Ref refPtr = hashConsTable[idx].ref;
//         return refPtr;
//     }
//     return ref;
// }


typedef struct {
    char *data;
    size_t capacity;
    size_t len;
} StringBuffer;

void sb_init(StringBuffer *sb) {
    sb->data = NULL;
    sb->capacity = 0;
    sb->len = 0;
}
void sb_free(StringBuffer *sb) {
    freePtr(sb->data);
    sb_init(sb);
}

void sb_append(StringBuffer *sb, const char *in, size_t len) {
    size_t required_space = len + 1;
    size_t remaining_space = sb->capacity - sb->len;
    if (required_space > remaining_space) {
        size_t required_capacity = sb->len + required_space;
        size_t new_capacity = max(required_capacity, max(2 * sb->capacity, 16));
        sb->data = realloc_or_panic(sb->data, new_capacity);
        sb->capacity = new_capacity;
    }
    memcpy(sb->data+sb->len, in, len);
    sb->len += len;
    // ensure there's always a null-terminator, this isn't included the length
    // this makes interop with native C functions easier
    sb->data[sb->len] = '\0';
}

void sb_printf(StringBuffer * sb, const char * fmt, ...) {
    va_list ap;
    // TODO ? try to print directly into the StringBuffer in-place
    // TODO ?   only call vsnprintf a second time if needed (once should be enough, most of the time)
    va_start(ap, fmt);
    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char buf[size+1];
    va_start(ap, fmt);
    vsnprintf(buf, size+1, fmt, ap);
    va_end(ap);
    sb_append(sb, buf, size);
}

// TODO ? provide a dynamic form of string interning / hashing?
//   currently it is the callers responsibility to ensure this is only called once for each string value
//   that is, hashing must be handled externally, typically in the code-generator
Str strStatic(const char *val, size_t len) {
    bool isStatic = true;
    // char *val2 = malloc_or_panic(len + 1);
    // memcpy(val2, val, len);
    // val2[len] = '\0';
    Str s = { isStatic, len, val };
    return s;
}

Str strC(const char *val) {
    int len = strlen(val);
    char *val2 = malloc_or_panic(len+1);
    memcpy(val2, val, len);
    val2[len] = '\0';
    return (Str){false, len, val2};
}

Str str_new(const char *val, size_t len) {
    char *val2 = malloc_or_panic(len+1);
    memcpy(val2, val, len);
    val2[len] = '\0';
    return (Str){false, len, val2};
}


List list_prepend1 (Repr elemRepr, List lp, void * elem) {
    if (lp.segment != NULL && elemRepr != lp.segment->elemRepr) {
        fatalError("list_prepend1: incorrect elemRepr (%p) (%p)", elemRepr, lp.segment ? lp.segment->elemRepr : NULL);
    }
    size_t elemSize = elemRepr->size;
    if (lp.segment == NULL) {
        int capacity = 1;
        int numElems = 1;
        int offset = capacity - 1;
        void * elems = malloc_or_panic(capacity * elemSize);
        MEMCPY(elems,offset,  elem,0,  1,elemSize);
        MALLOC(ListSegment, segment, { capacity, numElems, elems, { NULL, 0 }, elemRepr });
        return (List){ segment, offset };
    }
    else {
        int spaceExpected = lp.offset;
        int spaceAvailable = lp.segment->capacity - lp.segment->numElems;
        if (spaceAvailable > spaceExpected) {
            fatalError("list_prepend1: impossible offset(%d)+ numElems(%d) > capacity(%d)", lp.offset, lp.segment->numElems, lp.segment->capacity);
        }
        if (spaceAvailable == spaceExpected && spaceAvailable > 0) {
            int numElems = lp.segment->numElems;
            numElems += 1;
            int offset = lp.offset;
            offset -= 1;
            MEMCPY(lp.segment->elems, offset, elem, 0, 1, elemSize);
            lp.segment->numElems = numElems;
            lp.offset = offset;
            return lp;
        }
        else {
            // either there's no space left in this segment, 
            //   or the list has already been prepended to via another reference
            // int capacity = 2 * lp.segment->capacity;
            int capacity = 2 * lp.segment->numElems;
            capacity = min(capacity, 1000);
            int numElems = 1;
            int offset = capacity - 1;
            void * elems = malloc_or_panic(capacity * elemSize);
            MEMCPY(elems, offset, elem, 0, 1, elemSize);
            memset(elems, '\0', (capacity - numElems) * elemSize);
            MALLOC(ListSegment, segment, { capacity, numElems, elems, lp, elemRepr });
            return (List){ segment, offset };
        }
    }
}

List list_prependN_old (Repr elemRepr, List lp, int numElems, void * elems) {
    size_t elemSize = elemRepr->size;
    for (int i=0; i != numElems; i++) {
        lp = list_prepend1(elemRepr, lp, VOID_PTR_ADD(elems, (numElems - 1 - i), elemSize));
    }
    return lp;
}

// extern Repr u371_Tu2_St_Li_int;

void breakHere() {
    printf("Break Here\n");
}

List list_prependN (Repr elemRepr, List lp, int numElems, void * elems) {
    // if (elemRepr == u371_Tu2_St_Li_int) {
    //     breakHere();
    // }

    if (lp.segment != NULL && elemRepr != lp.segment->elemRepr) {
        fatalError("list_prependN: incorrect elemRepr (%p) (%p)", elemRepr, lp.segment ? lp.segment->elemRepr : NULL);
    }
    size_t elemSize = elemRepr->size;
    if (lp.segment == NULL) {
        int capacity = numElems;
        void * elems2 = malloc_or_panic(capacity * elemSize);
        MEMCPY(elems2, 0, elems, 0, numElems, elemSize);
        MALLOC(ListSegment, segment, { capacity, numElems, elems2, { NULL, 0 }, elemRepr });
        int offset = capacity - numElems;
        return (List){ segment, offset };
    }
    else {
        int spaceExpected = lp.offset;
        int spaceAvailable = lp.segment->capacity - lp.segment->numElems;
        if (spaceAvailable > spaceExpected) {
            fatalError("list_prependN: impossible offset(%d)+ numElems(%d) > capacity(%d)", lp.offset, lp.segment->numElems, lp.segment->capacity);
        }
        int numElemsFit = 0;
        if (spaceAvailable == spaceExpected && spaceAvailable > 0) {
            numElemsFit = min(numElems, spaceAvailable);
            MEMCPY(lp.segment->elems, lp.offset - numElemsFit, elems, numElems - numElemsFit, numElemsFit, elemSize);
            lp.segment->numElems += numElemsFit;
            lp.offset -= numElemsFit;
        }
        int numElemsLeft = numElems - numElemsFit;

        if (numElemsLeft == 0) {
            return lp;
        }

        // int capacity = 2 * lp.segment->capacity;
        int capacity = 2 * lp.segment->numElems;
        capacity = min(capacity, 1000);
        capacity = max(capacity, numElemsLeft);
        void * elems2 = malloc_or_panic(capacity * elemSize);
        MEMCPY(elems2, capacity - numElemsLeft, elems, 0, numElemsLeft, elemSize);
        memset(elems2, '\0', (capacity - numElemsLeft) * elemSize);
        MALLOC(ListSegment, segment, { capacity, numElemsLeft, elems2, lp, elemRepr });
        int offset = capacity - numElemsLeft;
        return (List){ segment, offset };
    }
}

bool list_isNil(List lp) {
    return lp.segment == NULL;
}
bool list_isPair(List lp) {
    return lp.segment != NULL;
}
void * list_head(Repr elemRepr, List lp) {
    if (lp.segment != NULL && elemRepr != lp.segment->elemRepr) {
        // fatalError("list_head: incorrect elemRepr (%p) (%p)", elemRepr, lp.segment ? lp.segment->elemRepr : NULL);
        fatalError("list_head: incorrect elemRepr (%s) (%s)", showRepr(elemRepr), lp.segment ? showRepr(lp.segment->elemRepr) : NULL);
    }
    // assume/require that there is always at least one element in a segment,
    // and so we never have to follow the tail field
    if (lp.segment == NULL) {
        fatalError("list_head: expected a non-empty List");
    }

    size_t elemSize = elemRepr->size;
    void * elemValue = VOID_PTR_ADD(lp.segment->elems, lp.offset, elemSize);

    return elemValue;
}
List list_tail(Repr elemRepr, List lp) {
    if (lp.segment != NULL && elemRepr != lp.segment->elemRepr) {
        fatalError("list_tail: incorrect elemRepr (%p) (%p)", elemRepr, lp.segment ? lp.segment->elemRepr : NULL);
    }
    if (lp.segment == NULL) {
        fatalError("list_tail: expected a non-empty List");
    }
    else if (lp.offset == lp.segment->capacity - 1) {
        // there's exactly one element in this segment, so return the next segment
        return lp.segment->tail;
    }
    else {
        return (List) { lp.segment, lp.offset+1 };
    }
}
void   list_frontPush(Repr elemRepr, List * lp, int numElems, void * elems) {
    fatalError("TODO: list_frontPush");
}
void * list_frontPop (Repr elemRepr, List * lp) {
    fatalError("TODO: list_frontPush");
}

int list_length(List lp) {
    int length = 0;
    while (lp.segment != NULL) {
        int occupancy = lp.segment->capacity - lp.offset;
        length += occupancy;
        lp = lp.segment->tail;
    }
    return length;
}

List list_reverse(Repr elemRepr, List lp) {
    int len = list_length(lp);
    if (len == 0) {
        return (List){ NULL, 0 };
    }
    void * data = malloc_or_panic(len * elemRepr->size);
    int i = 0;
    while (i != len) {
        if (lp.segment == NULL) {
            fatalError("impossible");
        }
        MEMCPY(data, len - 1 - i, lp.segment->elems, lp.offset, 1, elemRepr->size);
        lp.offset += 1;
        if (lp.offset == lp.segment->capacity) {
            lp = lp.segment->tail;
        }
        i += 1;
    }
    List lp2 = { NULL, 0 };
    lp2 = list_prependN(elemRepr, lp2, len, data);
    return lp2;
}

bool list_iterate(Repr elemRepr, List * lp, void * * elem) {
    if (lp->segment != NULL && elemRepr != lp->segment->elemRepr) {
        fatalError("list_iterate: incorrect elemRepr (%p) (%p)", elemRepr, lp->segment ? lp->segment->elemRepr : NULL);
    }
    if (lp->segment == NULL) {
        return false;
    }
    *elem = VOID_PTR_ADD(lp->segment->elems, lp->offset, elemRepr->size);
    lp->offset += 1;
    if (lp->offset == lp->segment->capacity) {
        *lp = lp->segment->tail;
    }
    return true;
}

void * list_lookup(Repr keyValRepr, Repr valMbRepr, Str key, List lp) {

    if (keyValRepr->tag != Repr_Tuple) {
        fatalError("list_lookup: expected keyValRepr to be a tuple repr");
    }
    const ReprTuple *keyValReprTuple = (const ReprTuple*) keyValRepr;
    if (keyValReprTuple->schema->numFields != 2) {
        fatalError("list_lookup: expected keyValRepr to be a 2-tuple repr");
    }
    if (valMbRepr->tag != Repr_Maybe) {
        fatalError("list_lookup: expected valMbRepr to be a tuple repr");
    }
    const ReprMaybe *valMbReprMaybe = (const ReprMaybe*) valMbRepr;
    Field keyF = keyValReprTuple->schema->fields[0];
    Field valF = keyValReprTuple->schema->fields[1];
    size_t isYesOffset = 0;
    size_t valueOffset = valMbReprMaybe->valueOffset;

    if (keyF.repr->tag != Repr_Str) {
        fatalError("list_lookup: expected keyValRepr to contain a Str key");
    }
    if (valMbReprMaybe->valueRepr != valF.repr) {
        fatalError("list_lookup: expected the value-Repr in valMbRepr to match the value-Repr in keyValRepr");
    }

    void * result = malloc_or_panic(valMbReprMaybe->base.size);



    List lp2 = lp;
    void * kv = NULL;
    while (list_iterate(keyValRepr, &lp2, &kv)) {
        Str k = *(Str*)VOID_PTR_ADD(kv, 1, keyF.offset);
        if (strEq(k, key)) {
            *(bool*)(VOID_PTR_ADD(result, isYesOffset, 1)) = true;
            memcpy(VOID_PTR_ADD(result, valueOffset, 1), VOID_PTR_ADD(kv, 1, valF.offset), valF.repr->size);
            return result;
        }
    }
    *(bool*)(VOID_PTR_ADD(result, isYesOffset, 1)) = false;
    return result;
}


void convertValue(const Repr inRepr, const void *inValue, const Repr outRepr, void *outValue) {
    fatalError("TODO");
}

TupleTail tuple_tail(Repr tupleRepr, int pos, const void * value) {
    if (tupleRepr->tag != Repr_Tuple) {
        fatalError("tuple_tail: ReprTuple required");
    }
    ReprTuple * tr = (ReprTuple*) tupleRepr;
    if (pos > tr->schema->numFields) {
        fatalError("tuple_tail: invalid pos(%d) > tuple-length(%d)", pos, tr->schema->numFields);
    }
    const void * value2 = mallocValue(tupleRepr, value);
    return (TupleTail){ tr, pos, NULL, value2 };
}


void any_to_value(Any in, const Repr outRepr, void *outValue) {
    bool ok = any_try_to_value(in, outRepr, outValue);
    if (!ok) {
        fatalError("any_to_value failed");
    }
}

const void * any_to_value_ptr(Any in, const Repr outRepr) {
    void * result = malloc_or_panic(outRepr->size);
    any_to_value(in, outRepr, result);
    return result;
}


bool any_try_to_value(Any in, const Repr outRepr, void *outValue) {
    if (in.repr == outRepr) {
        memcpy(outValue, in.value, in.repr->size);
        return true;
    }
    if (in.repr->tag == Repr_Any) {
        Any any = *(Any*) in.value;
        bool ok = any_try_to_value(any, outRepr, outValue);
        return ok;
    }
    if (in.repr->tag == Repr_Union) {
        ReprUnion * unionRepr = (ReprUnion*) in.repr;
        int inTag = *(int*) VOID_PTR_ADD(in.value, 0, 1);
        const void * inValuePtr = VOID_PTR_ADD(in.value, unionRepr->valueOffset, 1);
        Repr altRepr = unionRepr->alts[inTag];
        Any any = any_from_value(altRepr, inValuePtr);
        bool ok = any_try_to_value(any, outRepr, outValue);
        if (ok) {
            return true;
        }
        return false;
    }
    if (in.repr->tag == Repr_Ptr) {
        ReprPtr * ptrRepr = (ReprPtr*) in.repr;
        void * ptrValue = * (void**) in.value;
        Any any = {ptrRepr->valueRepr, ptrValue };
        bool ok = any_try_to_value(any, outRepr, outValue);
        return ok;
    }
    switch (outRepr->tag) {
        case Repr_No: {
            if (any_isNil(in)) {
                * (No*) outValue = any_to_nil(in);
                return true;
            }
            else {
                return false;
            }
        }
        case Repr_Bool: {
            if (any_isBool(in)) {
                * (bool*) outValue = any_to_bool(in);
                return true;
            }
            else {
                return false;
            }
        }
        case Repr_Int: {
            if (any_isInt(in)) {
                * (int*) outValue = any_to_int(in);
                return true;
            }
            else {
                return false;
            }
        }
        case Repr_Str: {
            if (in.repr->tag == Repr_Str) {
            // if (any_isStr(in)) {
                * (Str*) outValue = any_to_str(in);
                return true;
            }
            else if (in.repr->tag == Repr_Single) {
                ReprSingle *singleRepr = (ReprSingle*) in.repr;
                * (Str*) outValue = singleRepr->value;
                return true;
            }
            else if (in.repr->tag == Repr_Char) {
                Char ch = *(Char*) in.value;
                * (Str*) outValue = str_new(&ch.value, 1);
                return true;
            }
            else {
                return false;
            }
        }
        case Repr_Char: {
            if (in.repr->tag == Repr_Char) {
                * (Char*) outValue = *(Char*) in.value;
                return true;
            }
            if (in.repr->tag == Repr_Str) {
                Str s = *(Str*) in.value;
                if (s.len == 1) {
                    * (Char*) outValue = (Char) { s.data[0] };
                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        case Repr_Single: {
            ReprSingle *outSingleRepr = (ReprSingle*)outRepr;
            if (in.repr->tag == Repr_Single) {
                ReprSingle * inSingleRepr = (ReprSingle*) in.repr;
                // bool ok = strEq(inSingleRepr->value, outSingleRepr->value);
                bool ok = inSingleRepr == outSingleRepr;
                return ok;
            }
            if (in.repr->tag == Repr_Str) {
                Str value = any_to_str(in);
                if (strEq(value, outSingleRepr->value)) {
                    // all is well and as expected
                    // there's no data to copy to the outValue as the value is already in the outRepr
                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        case Repr_Type: {
            if (in.repr->tag == Repr_Type) {
                * (Type*) outValue = (Type){};
                return true;
            }
            else {
                return false;
            }
        }
        case Repr_Any: {
            // TODO ? use any_to_any, no point building unnecessary indirections
            // * (Any*) outValue = in;
            * (Any*) outValue = any_to_any(in);
            return true;
        }
        case Repr_List: {
            // TODO ? call any_to_list ?
            const ReprList *outReprList = (const ReprList*) outRepr;
            Repr elemRepr = outReprList->elem;
            List outList = {NULL, 0};
            void * elemValue = malloc_or_panic(elemRepr->size);
            Any a = in;
            while (any_isPair(a)) {
                Any elem = any_head(a);
                Any next = any_tail(a);
                // fprintf(stderr, "any_try_to_value: next(1): %s\n", showAny(next));
                bool ok = any_try_to_value(elem, elemRepr, elemValue);
                // fprintf(stderr, "any_try_to_value: next(2): %s\n", showAny(next));
                if (!ok) {
                    return false;
                }
                outList = list_prepend1(elemRepr, outList, elemValue);
                // outList = list_prependN(elemRepr, outList, 1, elemValue);
                a = next;
            }
            if (!any_isNil(a)) {
                return false;
            }
            outList = list_reverse(elemRepr, outList);
            * (List*) outValue = outList;
            return true;
        }
        case Repr_Tuple: {
            const ReprTuple *outReprTuple = (const ReprTuple *) outRepr;

            Schema *schema = outReprTuple->schema;
            if (outReprTuple->base.size != schema->size) {
                fatalError("any_try_to_value: incorrect size in TupleRepr: (%d) != (%d)", outReprTuple->base.size, schema->size);
            }
            char *tupleValue = malloc_or_panic(schema->size);
            memset(tupleValue, '\0', schema->size);
            Any a = in;
            for (int i=0; i != schema->numFields; i++) {
                Field field = schema->fields[i];
                if (!any_isPair(a)) {
                    return false;
                }
                Any elem = any_head(a);
                Any next = any_tail(a);
                bool ok = any_try_to_value(elem, field.repr, tupleValue+field.offset);
                if (!ok) {
                    return false;
                }
                a = next;
            }
            if (!any_isNil(a)) {
                return false;
            }
            memcpy(outValue, tupleValue, outReprTuple->schema->size);
            return true;
        }
        case Repr_Yes: {
            if (!any_isPair(in)) {
                return false;
            }
            Any h = any_head(in);
            Any t = any_tail(in);
            if (!any_isNil(t)) {
                return false;
            }
            const ReprYes *outReprYes = (const ReprYes *) outRepr;
            bool ok = any_try_to_value(h, outReprYes->elemRepr, outValue);
            return ok;
        }
        case Repr_Maybe: {
            // printf("any_try_to_value: %s\n", showAny(in));
            const ReprMaybe *outReprMaybe = (const ReprMaybe *) outRepr;
            size_t isYesOffset = 0;
            size_t valueOffset = outReprMaybe->valueOffset;
            if (any_isNil(in)) {
                memset(outValue, '\0', outReprMaybe->base.size);
                *(bool*)VOID_PTR_ADD(outValue, isYesOffset, 1) = false;
                return true;
            }
            Any h = any_head(in);
            Any t = any_tail(in);
            if (any_isNil(t)) {
                bool ok = any_try_to_value(h, outReprMaybe->valueRepr, VOID_PTR_ADD(outValue, valueOffset, 1));
                if (!ok) {
                    return false;
                }
                *(bool*)VOID_PTR_ADD(outValue, isYesOffset, 1) = true;
                return true;
            }
            return false;
        }
        case Repr_Union: {
            const ReprUnion *repr = (const ReprUnion *) outRepr;
            bool result = any_try_to_union(in, repr, outValue);
            return result;
        }
        case Repr_Ptr: {
            ReprPtr *ptrRepr = (ReprPtr*) outRepr;
            void *valuePtr = malloc_or_panic(ptrRepr->valueRepr->size);
            bool ok = any_try_to_value(in, ptrRepr->valueRepr, valuePtr);
            if (ok) {
                *(void**)outValue = valuePtr;
                return true;
            }
            else {
                return false;
            }
        }
        case Repr_Func: {
            // functions are always boxed as closures
            fatalError("any_try_to_value: impossible Repr_Func");
        }
        case Repr_Clos: {
            // it is not possible to tentatively match against a closure
            // so use the apply_ds to defer the matching/conversion to the function arguments+return value
            // use apply_ds + apply_sd to adapt the function, as needed
            ReprClos * closRepr = (ReprClos*) outRepr;
            void * func = closRepr->apply_ds;
            const void * env = any_func_to_clos_env(in);
            // TODO ? we probably need another type-specific function in the ReprClos,
            // TODO ?   so as to construct a type-specific closure correctly.
            // TODO ? this relies on the ClosureGeneric and the generated type-specific closures having the same layout.
            ClosureGeneric closure = { func, env };
            *(ClosureGeneric*) outValue = closure;
            return true;
        }
        default:
            fatalError("any_try_to_value: unhandled type/repr tag (%d)", outRepr->tag);
    }
}

Any any_from_value(const Repr inRepr, const void * inValue) {
    switch (inRepr->tag) {
        case Repr_Any: {
            Any result = *(Any*) inValue;
            return result;
        }
        case Repr_Union: {
            ReprUnion * unRepr = (ReprUnion*) inRepr;
            int tag = *(int*) VOID_PTR_ADD(inValue, 0, 1);
            Repr repr2 = unRepr->alts[tag];
            const void * value = VOID_PTR_ADD(inValue, unRepr->valueOffset, 1);
            // if the inValue was on the stack, make sure we copy it off
            const void * value2 = mallocValue(repr2, value);
            Any result = { repr2, value2 };
            return result;
        }
        default: {
            void * value = malloc_or_panic(inRepr->size);
            memcpy(value, inValue, inRepr->size);
            Any result = { inRepr, value };
            return result;
        }
    }
}


Any any_nil() {
    No no = {};
    Any result = any_from_value(&noRepr.base, &no);
    return result;
}

Any any_pair(Any h, Any t) {
    Pair p = { h, t };
    Any result = any_from_value(&pairRepr.base, &p);
    return result;
}

Any any_tuple2(Any a, Any b) {
    return any_pair(a, any_pair(b, any_nil()));
}


Any any_list_c(Any elem0, ...) {
    Any elems = any_pair(elem0, any_nil());
    va_list ap;
    va_start(ap, elem0);
    Any elem = {};
    while ((elem = va_arg(ap, Any)).repr != NULL) {
        elems = any_pair(elem, elems);
    }
    va_end(ap);
    Any it = elems;
    elems = any_nil();
    elem = (Any){};
    while (any_iterate(&it, &elem)) {
        elems = any_pair(elem, elems);
    }
    return elems;
}

ListStr str_list_c(Str elem0, ...) {
    List elems = {};
    elems = list_prepend1(&strRepr.base, elems, &elem0);
    va_list ap;
    va_start(ap, elem0);
    Str elem = {};
    while ((elem = va_arg(ap, Str)).data != NULL) {
        elems = list_prepend1(&strRepr.base, elems, &elem);
    }
    va_end(ap);
    List it = elems;
    elems = (List){};
    Str * elemPtr = NULL;
    while (list_iterate(&strRepr.base, &it, (void**)&elemPtr)) {
        elems = list_prepend1(&strRepr.base, elems, elemPtr);
    }
    ListStr result = { elems };
    return result;
}



// TODO ? a generic function to copy any stack-allocated parts of a value to the heap, ?
// TODO ?   to be called just before that value is returned from a function ?
void copyOffStack(Repr repr, void *value) {
    // TODO ?
    // traverse and copy stack-allocated parts of lists and closures,
    //   pruning the traversal when heap-allocated values are found.
    // This would save needing to preemptively allocate closures environments on the heap.
    //   means we need a way to determine if something is on the stack though.
    // call it: Copy-on-Return ?
    // call it: Escape-on-Return ?
    //
    // Possibly reference-couting inc/dec operations can be moved from argument passing time to value-escaping time.
    // Most functions would probably not care if they are called with a unique reference to an argument or not.
    // Dropping the references to arguments can be made the callers exclusive reposibility, 
    //   and be done after copyOffStack has incremented any reference counts it needs to.
    // This does risk holding on to memory for longer than needed,
    //   this would affect some code styles more than others.
    //   for example
    //     let y = f (g (h (x)));
    //   would result in all the intermediate values being held onto until the end,
    //   because some parts of the intermediate may (or may not) be used in the next value.
    // User annotations could be used to add earlier copying/dropping calls if memory needs to be freed earlier.
    // Functions which make use of copy-on-write will need to be responsible for handling the ref-counts though,
    //   so will need to make this clear in their calling-convention / ReprFunc.
    // (currently using panic-on-stale-write rather than copy-on-shared-write, so the issue doesn't immediately need handling )
    //
    // call it: Reference-Couting-on-Return ?
}





bool strEq(Str a, Str b) {
    if (a.isStatic && b.isStatic) {
        return a.len == b.len && a.data == b.data;
    }
    else {
        return (a.len == b.len && memcmp(a.data, b.data, a.len) == 0);
    }
}

bool str_char_eq(Str a, Char b) {
    if (a.len == 1 && a.data[0] == b.value) {
        return true;
    }
    else {
        return false;
    }
}

bool char_eq(Char a, Char b) {
    return a.value == b.value;
}


Any any_listAt(Any list, int pos);



Any any_to_any(Any a) {
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            return any_to_any(b);
        }
        case Repr_Union: {
            ReprUnion * unionRepr = (ReprUnion*) a.repr;
            int tag = *(int*) VOID_PTR_ADD(a.value, 0, 1);
            const void * value = VOID_PTR_ADD(a.value, unionRepr->valueOffset, 1);
            Repr repr = unionRepr->alts[tag];
            Any b = { repr, value };
            return any_to_any(b);
        }
        case Repr_Ptr: {
            ReprPtr *ptrRepr = (ReprPtr*) a.repr;
            void const * const * ptr = a.value;
            void const * value = *ptr;
            Any b = { ptrRepr->valueRepr, value };
            return any_to_any(b);
        }
        default:
            return a;
    }
}


Any any_head(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            Any result = any_head(b);
            return result;
        }
        case Repr_Pair: {
            Pair p = *(Pair*) a.value;
            return p.hd;
        }
        case Repr_List: {
            ReprList listRepr = *(ReprList*) a.repr;
            List listValue = *(List*) a.value;
            Repr elemRepr = listRepr.elem;
            void *elemValue = list_head(elemRepr, listValue);
            // use a NULL header for now
            Any result = { elemRepr, elemValue };
            return result;
        }
        case Repr_Tuple: {
            ReprTuple *tupleRepr = (ReprTuple*) a.repr;
            Any result = { tupleRepr->schema->fields[0].repr, a.value };
            return result;
        }
        case Repr_TupleTail: {
            TupleTail * tt = (TupleTail*) a.value;
            if (tt->pos == tt->tupleRepr->schema->numFields) {
                fatalError("any_head: cannot take head of empty TupleTail");
            }
            Field * field = tt->tupleRepr->schema->fields + tt->pos;
            Any result = {field->repr, VOID_PTR_ADD(tt->tupleValue, field->offset, 1) };
            return result;
        }
        case Repr_Union: {
            ReprUnion * unionRepr = (ReprUnion*) a.repr;
            Any b = any_from_union(unionRepr, a.value);
            Any result = any_head(b);
            return result;
        }
        case Repr_Maybe: {
            ReprMaybe * maybeRepr = (ReprMaybe*) a.repr;
            size_t isYesOffset = 0;
            size_t valueOffset = maybeRepr->valueOffset;
            // TODO make sure an actual bool is used to set the isYes field, 
            // TODO   or we cannot read it as a bool and expect correct behaviour
            // bool isYes = *(bool*) VOID_PTR_ADD(a.value, okField.offset, 1);
            // if (!isYes) {
            int isYes = *(bool*) VOID_PTR_ADD(a.value, isYesOffset, 1);
            if (isYes == false) {
                fatalError("any_head: failed to take head of a no-maybe");
            }
            const void * valuePtr = VOID_PTR_ADD(a.value, valueOffset, 1);
            Any result = { maybeRepr->valueRepr, valuePtr };
            return result;
        }
        case Repr_Yes: {
            ReprYes * yesRepr = (ReprYes*) a.repr;
            Any result = { yesRepr->elemRepr, a.value };
            return result;

        }
        default:
            fatalError("any_head: cannot take head of non-pair");
    }
}

Any any_tail(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            Any result = any_tail(b);
            return result;
        }
        case Repr_Pair: {
            Pair p = *(Pair*) a.value;
            return p.tl;
        }
        case Repr_List: {
            ReprList * listRepr = (ReprList*) a.repr;
            List listValue = *(List*) a.value;
            Repr elemRepr = listRepr->elem;
            List tailValue = list_tail(elemRepr, listValue);
            List *tailPtr = malloc_or_panic(sizeof(List));
            *tailPtr = tailValue;
            // use a NULL header for now
            Any result = { &listRepr->base, tailPtr };
            return result;
        }
        case Repr_Tuple: {
            ReprTuple *tupleRepr = (ReprTuple*) a.repr;
            if (tupleRepr->schema->numFields == 1) {
                Any result = { &noRepr.base, NULL };
                return result;
            }
            else {
                MALLOC(TupleTail, tt, {tupleRepr, 1, NULL, a.value});
                Any result = { & tupleTailRepr.base, tt };
                return result;
            }
        }
        case Repr_TupleTail: {
            TupleTail * tt = (TupleTail*) a.value;
            if (tt->pos == tt->tupleRepr->schema->numFields) {
                fatalError("any_tail: cannot take tail of empty TupleTail");
            }
            if (tt->pos - 1 == tt->tupleRepr->schema->numFields) {
                return (Any){ &noRepr.base, NULL};
            }
            else {
                MALLOC(TupleTail, tt2, {tt->tupleRepr, tt->pos+1, tt->hdr, tt->tupleValue});
                Any result = { & tupleTailRepr.base, tt2 };
                return result;
            }
        }
        case Repr_Union: {
            ReprUnion * unionRepr = (ReprUnion*) a.repr;
            Any b = any_from_union(unionRepr, a.value);
            Any result = any_tail(b);
            return result;
        }
        case Repr_Maybe: {
            ReprMaybe * maybeRepr = (ReprMaybe*) a.repr;
            size_t isYesOffset = 0;
            size_t valueOffset = maybeRepr->valueOffset;
            int isYes = *(bool*) VOID_PTR_ADD(a.value, isYesOffset, 1);
            if (!isYes) {
                fatalError("any_tail: failed to take tail of a no-maybe");
            }
            Any result = { &noRepr.base, NULL };
            return result;
        }
        case Repr_Yes: {
            Any result = { &noRepr.base, NULL };
            return result;
        }
        default:
            fatalError("any_tail: cannot take head of non-pair");
    }
}

bool any_isNil(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            bool result = any_isNil(b);
            return result;
        }
        case Repr_No:
            return true;
        case Repr_Pair:
            return false;
        case Repr_List: {
            ReprList listRepr = *(ReprList*) a.repr;
            List listValue = *(List*) a.value;
            bool result = list_isNil(listValue);
            return result;
        }
        case Repr_Yes:
            return false;
        case Repr_Maybe: {
            ReprMaybe * maybeRepr = (ReprMaybe*) a.repr;
            size_t isYesOffset = 0;
            bool isYes = *(bool*) VOID_PTR_ADD(a.value, isYesOffset, 1);
            return ! isYes;
        }
        case Repr_Tuple: {
            ReprTuple *tupleRepr = (ReprTuple*) a.repr;
            bool result = tupleRepr->schema->numFields == 0;
            return result;
        }
        case Repr_TupleTail: {
            TupleTail * tt = (TupleTail*) a.value;
            bool isNil = tt->pos == tt->tupleRepr->schema->numFields;
            return isNil;
        }
        case Repr_Union:
            ReprUnion * unionRepr = (ReprUnion*) a.repr;
            Any b = any_from_union(unionRepr, a.value);
            bool result = any_isNil(b);
            return result;
        // TODO list cases explicitly, default should call fatalError missing-case
        default:
            // fatalError("any_isNil: missing case: %s", showRepr(a.repr))
            return false;
    }
}

bool any_isPair(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            bool result = any_isPair(b);
            return result;
        }
        case Repr_Pair:
            return true;
        case Repr_List: {
            ReprList listRepr = *(ReprList*) a.repr;
            List listValue = *(List*) a.value;
            bool result = list_isPair(listValue);
            return result;
        }
        case Repr_Maybe: {
            ReprMaybe * maybeRepr = (ReprMaybe*) a.repr;
            size_t isYesOffset = 0;
            bool isYes = *(bool*) VOID_PTR_ADD(a.value, isYesOffset, 1);
            return isYes;
        }
        case Repr_Yes:
            return true;
        case Repr_Tuple: {
            ReprTuple *tupleRepr = (ReprTuple*) a.repr;
            bool result = tupleRepr->schema->numFields != 0;
            return result;
        }
        case Repr_TupleTail: {
            TupleTail * tt = (TupleTail*) a.value;
            bool isNil = tt->pos == tt->tupleRepr->schema->numFields;
            return ! isNil;
        }
        case Repr_Union:
            ReprUnion * unionRepr = (ReprUnion*) a.repr;
            Any b = any_from_union(unionRepr, a.value);
            bool result = any_isPair(b);
            return result;
        // TODO ? list cases explicitly, default should call fatalError missing-case
        // TODO ? alternatively, switch to placing the various functions on the Repr definitions in an OO-like way
        default:
            // fatalError("any_isPair: missing case: %s", showRepr(a.repr))
            return false;
    }
}


bool any_isBool(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            return any_isBool(b);
        }
        case Repr_Bool:
            return true;
        default:
            return false;
    }
}
bool any_isInt(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            return any_isInt(b);
        }
        case Repr_Int:
            return true;
        default:
            return false;
    }
}
bool any_isStr(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            return any_isStr(b);
        }
        case Repr_Str:
            return true;
        case Repr_Single:
            return true;
        case Repr_Char:
            return true;
        default:
            return false;
    }
}
bool any_isType(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Any: {
            Any b = *(Any*) a.value;
            return any_isType(b);
        }
        case Repr_Type:
            return true;
        default:
            return false;
    }
}

bool any_isFunc(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Clos:
        case Repr_Func:
        case Repr_PartialApply:
            return true;
        case Repr_Any: {
            Any b = *(Any*) a.value;
            return any_isFunc(b);
        }
        default:
            return false;
    }
}



bool any_iterate(Any * it, Any * elem) {
    if (any_isNil(*it)) {
        *it = (Any){ NULL, NULL };
        *elem = (Any){ NULL, NULL };
        return false;
    }
    else if (any_isPair(*it)) {
        *elem = any_head(*it);
        *it = any_tail(*it);
        return true;
    }
    else {
        fatalError("any_iterate: expected a list");
    }
}

int any_to_int(Any a) {
    a = any_to_any(a);
    switch (a.repr->tag) {
        case Repr_Int: {
            int result = *(int*)a.value;
            return result;
        }
        case Repr_Any: {
            Any b = *(Any*) a.value;
            int result = any_to_int(b);
            return result;
        }
        default:
            fatalError("any_to_int: failed");
    }
}
Any any_from_int(int a) {
    int *b = malloc_or_panic(sizeof(int));
    *b = a;
    Any result = { & intRepr.base, b };
    return result;
}

No any_to_nil(Any a) {
    if (!any_isNil(a)) {
        fatalError("any_to_nil: failed");
    }
    return (No){};
}

Any any_from_nil(No) {
    Any result = { & noRepr.base, NULL };
    return result;
}

bool any_to_bool(Any a) {
    a = any_to_any(a);
    if (a.repr->tag == Repr_Bool) {
        bool b = *(bool*) a.value;
        return b;
    }
    else {
        fatalError("any_to_bool: failed");
    }
}
Any any_from_bool(bool a) {
    bool *b = malloc_or_panic(sizeof(bool));
    *b = a;
    Any result = { & boolRepr.base, b };
    return result;
}

Str any_to_str(Any a) {
    a = any_to_any(a);
    if (a.repr->tag == Repr_Str) {
        Str b = *(Str*) a.value;
        return b;
    }
    else if (a.repr->tag == Repr_Single) {
        ReprSingle *reprSingle = (ReprSingle*) a.repr;
        Str b = reprSingle->value;
        return b;
    }
    else if (a.repr->tag == Repr_Char) {
        Char ch = *(Char*) a.value;
        return str_new(&ch.value, 1);
    }
    else if (a.repr->tag == Repr_Union) {
        Str b;
        any_to_value(a, &strRepr.base, &b);
        return b;
    }
    else {
        fatalError("any_to_str: failed");
    }
}
Any any_from_str(Str a) {
    Str *b = malloc_or_panic(sizeof(Str));
    *b = a;
    Any result = { & strRepr.base, b };
    return result;
}

Char any_to_char(Any a) {
    Char ch = {};
    bool ok = any_try_to_value(a, &charRepr.base, &ch);
    if (!ok) {
        fatalError("any_to_char: failed");
    }
    return ch;

}


Type any_to_type(Any a) {
    a = any_to_any(a);
    if (a.repr->tag == Repr_Type) {
        Type b = *(Type*) a.value;
        return b;
    }
    else {
        fatalError("any_to_type: failed");
    }

}
Any any_from_type(Type a) {
    Type *b = malloc_or_panic(sizeof(Type));
    *b = a;
    Any result = { & typeRepr.base, b };
    return result;
}

void * any_to_tuple(const ReprTuple *repr, Any a) {
    a = any_to_any(a);
    Schema *schema = repr->schema;
    if (repr->base.size != schema->size) {
        fatalError("incorrect size in TupleRepr: (%d) != (%d)", repr->base.size, schema->size);
    }
    char *result = malloc_or_panic(schema->size);
    memset(result, '\0', schema->size);
    for (int i=0; i != schema->numFields; i++) {
        Field field = schema->fields[i];
        Any elem = any_head(a);
        Any next = any_tail(a);
        any_to_value(elem, field.repr, result+field.offset);
        a = next;
    }
    // debugf("any_to_tuple Result: %s\n", showReprData(&repr->base, result));
    return result;
}

Any any_from_tuple(const ReprTuple *tupleRepr, const void * a) {
    void * a2 = malloc_or_panic(tupleRepr->base.size);
    memcpy(a2, a, tupleRepr->base.size);
    Any result = { & tupleRepr->base, a2 };
    return result;
}


Any any_from_maybe(const ReprMaybe *maybeRepr, const void * a) {
    // TODO make sure an actual bool is used to set the isYes field, 
    // TODO   or we cannot read it as a bool and expect correct behaviour
    // bool isYes = *(bool*) VOID_PTR_ADD(a.value, okField.offset, 1);
    // if (!isYes) {
    size_t isYesOffset = 0;
    int isYes = *(bool*) VOID_PTR_ADD(a, isYesOffset, 1);
    if (isYes) {
        Repr valueRepr = maybeRepr->valueRepr;
        const void * valuePtr = VOID_PTR_ADD(a, maybeRepr->valueOffset, 1);
        void * a2 = malloc_or_panic(valueRepr->size);
        memcpy(a2, a, valueRepr->size);
        Any result = { valueRepr, a2 };
        return result;
    }
    else {
        return (Any){&noRepr.base, NULL};
    }
}

Any any_from_yes(const ReprYes *yesRepr, const void * a) {
    Any result = any_from_value(&yesRepr->base, a);
    return result;
}


List any_to_list(const Repr elemRepr, Any a) {
    a = any_to_any(a);
    size_t elemSize = elemRepr->size;
    if (a.repr->tag == Repr_List) {        
        List lp = *(List*)a.value;
        // if the elemRepr matches, then the boxed List can be used directly
        if ((lp.segment == NULL || elemRepr == lp.segment->elemRepr)) {
            return lp;
        }
        // otherwise, forget we ever had a boxed List,
        //   fall through to Repr-independent method of
        //     extracting and converting the elements one by one below
    }
    // else 
    {
        size_t len = 0;
        Any it = a;
        Any elem;
        while (any_iterate(&it, &elem)) {
            len += 1;
        }
        if (it.repr != NULL && it.repr->tag != Repr_List) {
            fatalError("any_to_list: list-ptr tail must be nil, or another list-ptr, not (%d)\n", it.repr->tag);
        }
        if (len == 0) {
            List listPtr = { NULL, 0 };
            return listPtr;
        }

        // TODO allocate data in chunks, so as not to get
        // TODO   "GC Warning: Repeated allocation of very large block"
        // TODO warnings from the Boehm GC at runtime.
        void *data = malloc_or_panic(elemSize * len);
        it = a;
        size_t i = 0;
        // only copy up until a List is encountered, and use that in the tail segment
        // while (it->tag != BOX_ListPtr && iterate(&it, &elem)) {
        while (any_iterate(&it, &elem)) {
        // while (it.repr->tag != Repr_List && any_iterate(&it, &elem)) {
            any_to_value(elem, elemRepr, VOID_PTR_ADD(data, i, elemSize));
            i += 1;
        }
        List tailPtr = { NULL, 0 };
        if (it.repr == NULL) {
            // nothing more to do
        }
        else if (it.repr->tag == Repr_List) {
            // TODO we can no longer reach this branch, so delete this
            tailPtr = *(List*) it.value;
        }
        else {
            fatalError("impossible: any_to_list");
        }
        List listPtr = list_prependN(elemRepr, tailPtr, len, data);
        return listPtr;
    }
}

Any any_from_list(const Repr listRepr, List a) {
    List *b = malloc_or_panic(sizeof(List));
    *b = a;
    Any result = { listRepr, b };
    return result;
}



bool any_try_to_union(Any a, ReprUnion const * outRepr, void * outPtr) {
    a = any_to_any(a);
    const ReprUnion *repr = outRepr;
    void * value = NULL;
    for (int i=0; i != repr->numAlts; i++) {
        Repr altRepr = repr->alts[i];
        int * tagPtr = outPtr;
        void * valuePtr = VOID_PTR_ADD(outPtr, repr->valueOffset, 1);
        bool ok = any_try_to_value(a, altRepr, valuePtr);
        if (ok) {
            *tagPtr = i;
            // debugf("dynToUnion Result: %s\n", showReprData(&repr->base, &result));
            return true;
        }
    }
    return false;
}

void any_to_union(Any a, ReprUnion const * outRepr, void * outPtr) {
    bool ok = any_try_to_union(a, outRepr, outPtr);
    if (!ok) {
        fatalError("any_to_union: failed to convert value(%s) to union(%s)", showAny(a), showRepr(&outRepr->base));
    }
}

Any any_from_union(ReprUnion const * repr, const void *a) {
    int tag = *(int*) VOID_PTR_ADD(a, 0, 1);
    const void * value = VOID_PTR_ADD(a, repr->valueOffset, 1);
    Repr outRepr = repr->alts[tag];
    Any result = { outRepr, value };
    return result;
}


ReprPartialApply partialApplyRepr = { { Repr_PartialApply } };


Schema AnyFuncClosEnv_Schema = {"AnyFuncClosEnv", sizeof(AnyFuncClosEnv), 1, (Field[]){{"func", &anyRepr.base, offsetof(AnyFuncClosEnv,func)}}};

const AnyFuncClosEnv * any_func_to_clos_env(Any func) {
    MALLOC(AnyFuncClosEnv, env, {{&AnyFuncClosEnv_Schema}, func});
    return env;
}

Any any_call (Any func, Any arg) {
    // fprintf(stderr, "any_call: func: %s\n", showAny(func));
    // printf("any_call: arg: %s\n", showAny(arg));
    func = any_to_any(func);
    switch (func.repr->tag) {
        case Repr_Clos: {
            ReprClos * closRepr = (ReprClos*) func.repr;
            ClosureGeneric clos = *(ClosureGeneric*) func.value;
            if (closRepr->numParam == 1) {
                Any * partialArgs = NULL;
                Any lastArg = arg;
                Any result = closRepr->apply_sd(clos.func, clos.env, partialArgs, lastArg);
                return result;
            }
            else {
                MALLOC(Any, args, arg);
                MALLOC(PartialApply, pa, { closRepr, clos.func, clos.env, 1, args } );
                Any result = { &partialApplyRepr.base, pa };
                return result;
            }
        }
        case Repr_Func: {
            ReprFunc * funcRepr = (ReprFunc*) func.repr;
            void * funcPtr = *(void**) func.value;
            void * env = NULL;
            if (funcRepr->numParam == 1) {
                Any * partialArgs = NULL;
                Any lastArg = arg;
                Any result = funcRepr->apply_sd(funcPtr, env, partialArgs, lastArg);
                return result;
            }
            else {
                MALLOC(Any, args, arg);
                MALLOC(PartialApply, pa, { funcRepr, funcPtr, env, 1, args } );
                Any result = { &partialApplyRepr.base, pa };
                return result;
            }
        }
        case Repr_FuncNull: {
            ReprFuncNull * closRepr = (ReprFuncNull*) func.repr;
            void * funcPtr = *(void**) func.value;
            void * envPtr = NULL;
            if (closRepr->numParam == 1) {
                Any * partialArgs = NULL;
                Any lastArg = arg;
                Any result = closRepr->apply_sd(funcPtr, envPtr, partialArgs, lastArg);
                return result;
            }
            else {
                MALLOC(Any, args, arg);
                MALLOC(PartialApply, pa, { closRepr, funcPtr, envPtr, 1, args } );
                Any result = { &partialApplyRepr.base, pa };
                return result;
            }
        }
        case Repr_PartialApply: {
            PartialApply pa = *(PartialApply*) func.value;
            switch (pa.clos->base.tag) {
                case Repr_Clos:
                case Repr_Func:
                case Repr_FuncNull: {
                    ReprClos * closRepr = (ReprClos*) pa.clos;
                    if (pa.numArgs + 1 == closRepr->numParam) {
                        Any lastArg = arg;
                        Any * partialArgs = pa.args;
                        Any result = closRepr->apply_sd(pa.func, pa.env, partialArgs, lastArg);
                        return result;
                    }
                    else {
                        Any * partialArgs = malloc_or_panic((pa.numArgs + 1) * sizeof(Any));
                        MEMCPY(partialArgs, 0, pa.args, 0, pa.numArgs, sizeof(Any));
                        partialArgs[pa.numArgs] = arg;
                        MALLOC(PartialApply, pa2, { pa.clos, pa.func, pa.env, pa.numArgs + 1, partialArgs });
                        Any result = { &partialApplyRepr.base, pa2 };
                        return result;
                    }
                }
                default:
                    fatalError("any_call: cannot apply (%s)", showRepr(&pa.clos->base));
            }
        }
        default:
            fatalError("any_call: cannot apply (%s)", showRepr(func.repr));
    }
}



Any any_from_func (const ReprFunc *repr, const void * func) {
    if (repr->base.tag != Repr_Func) {
        fatalError("any_from_func: expected a Repr_Func");
    }
    MALLOC(void *, funcPtr, func);
    Any result = { &repr->base, funcPtr };
    return result;
}

// any_to_clos returns a type-specific closure
// const void * any_to_clos (const ReprFunc *repr, Any);

// any_from_clos takes type-specific function and environment pointers
Any any_from_clos (const ReprClos * repr, const void * func, const void * env) {
    if (repr->base.tag != Repr_Clos) {
        fatalError("any_from_clos: expected a Repr_Clos");
    }
    const Header * envHdr = env;
    if (envHdr != NULL && envHdr->schema == &AnyFuncClosEnv_Schema) {
        // If the env is an AnyFuncClosEnv, 
        //   then it was created by a call to any_func_to_clos_env,
        //   when using a dynamic function is a static context.
        // We're now using an apparently static function in a dynamic context.
        // We can just return that previous underlying dynamic function from the environment.
        // It's safe to ignore "func" as in this case
        //   it will just be one of the repr->apply_ds adaptor functions.
        // This avoids building alternating chains of static->dynamic and dynamic->static conversions.
        const AnyFuncClosEnv * anyFuncClosEnv = env;
        return anyFuncClosEnv->func;
    }
    // MALLOC(PartialApply, pa, { repr, func, env, 0, NULL });
    // Any result = { &partialApplyRepr.base, NULL, pa };

    MALLOC(ClosureGeneric, clos, { func, env });
    Any result = { &repr->base, clos };


    return result;
}

// Any any_from_func_null (const ReprFuncNull * repr, const void * func) {
//     if (repr->base.tag != Repr_FuncNull) {
//         fatalError("any_from_func_null: expected a Repr_FuncNull");
//     }
//     MALLOC(PartialApply, pa, { repr, func, NULL, 0, NULL });
//     Any result = { &partialApplyRepr.base, NULL, pa };
//     return result;
// }

Any any_from_func_null (const ReprFuncNull *repr, const void * func) {
    if (repr->base.tag != Repr_FuncNull) {
        fatalError("any_from_func_null: expected a Repr_FuncNull");
    }
    MALLOC(void *, funcPtr, func);
    Any result = { &repr->base, funcPtr };
    return result;
}



// TODO holdRefs and dropRefs, for holding/dropping multiple refs in one call
// TODO maintain a global ref-count and check it goes back to zero at the end of the program
// TODO maintain a max ref-count and include it in diagnostic output

// void holdRef(Ref ref) {
//     // ref->refCount ++;
//     if (SAFETY_CHECK_ERROR(ref->refCount < 0)) {
//         fatalError("negative refCount (%d) !!!", ref->refCount);
//     }
// }

// void dropRef(Ref ref) {
//     // ref->refCount --;
//     if (ref->refCount > 0) {
//         return;
//     }
//     if (SAFETY_CHECK_ERROR(ref->refCount < 0)) {
//         fatalError("negative refCount (%d) !!!", ref->refCount);
//     }
//     switch (ref->tag) {
//         case BOX_Any:
//             // TODO
//             break;
//         default:
//             fatalError("dropRef: unexpected tag (%d)", ref->tag);
//     }
// }



int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }

bool gt (int a, int b) { return a >  b; }
bool lt (int a, int b) { return a <  b; }
bool gte(int a, int b) { return a >= b; }
bool lte(int a, int b) { return a <= b; }

bool any_eq(Any a, Any b) {
    a = any_to_any(a);
    b = any_to_any(b);
    bool result;
    // fprintf(stderr, "eq "); printRef(stderr, a); fprintf(stderr, " == "); printRef(stderr, b); fprintf(stderr, "\n");
    if (any_isNil(a) && any_isNil(b)) {
        result = true;
    }
    else if (any_isBool(a) && any_isBool(b)) {
        bool aBool = any_to_bool(a);
        bool bBool = any_to_bool(b);
        result = aBool == bBool;
    }
    else if (any_isInt(a) && any_isInt(b)) {
        int aInt = any_to_int(a);
        int bInt = any_to_int(b);
        result = aInt == bInt;
    }
    else if (any_isStr(a) && any_isStr(b)) {
        Str aStr = any_to_str(a);
        Str bStr = any_to_str(b);
        result = strEq(aStr, bStr);
    }
    else if (any_isNil(a) || any_isBool(a) || any_isInt(a) || any_isStr(a)) {
        result = false;
    }
    else if (any_isNil(b) || any_isBool(b) || any_isInt(b) || any_isStr(b)) {
        result = false;
    }
    // TODO ? faster equality checks for Tuples with the same schema, save redynamifying all the elements ?
    else if (any_isPair(a) && any_isPair(b)) {
        result = any_eq(any_head(a), any_head(b)) && any_eq(any_tail(a), any_tail(b));
    }
    else if ( (any_isFunc(a) || any_isType(a)) && (any_isFunc(b) || any_isType(b)) ) {
        fatalError("eq: cannot compare functions+types with functions+types (%d, %d)", a.repr->tag, b.repr->tag);
    }
    else {
        fatalError("eq: impossible, unaccounted for tags (%d, %d)", a.repr->tag, b.repr->tag);
    }
    // fprintf(stderr, "eq "); printRef(stderr, a); fprintf(stderr, " == "); printRef(stderr, b); fprintf(stderr, " = %d\n", result);
    return result;
}


int int_sign (int num) {
    if (num < 0) {
        return -1;
    }
    else if (num > 0) {
        return 1;
    }
    else {
        return 0;
    }
}

int str_compare(Str a, Str b) {
    size_t minLen = min(a.len, b.len);
    int contentDiff = memcmp(a.data, b.data, minLen);
    if (contentDiff == 0) {
        size_t lenDiff = a.len - b.len;
        return int_sign(lenDiff);
    }
    else {
        return int_sign(contentDiff);
    }
}

int any_compare (Any a, Any b) {
    a = any_to_any(a);
    b = any_to_any(b);
    if (any_isNil(a) && any_isNil(b)) {
        return 0;
    }

    if (any_isBool(a) && any_isBool(b)) {
        bool aBool = any_to_bool(a);
        bool bBool = any_to_bool(b);
        return int_sign(aBool - bBool);
    }
    if (any_isBool(a)) { return -1; }
    if (any_isBool(b)) { return +1; }

    if (any_isInt(a) && any_isInt(b)) {
        int aInt = any_to_int(a);
        int bInt = any_to_int(b);
        return int_sign(aInt - bInt);
    }
    if (any_isInt(a)) { return -1; }
    if (any_isInt(b)) { return +1; }

    if (any_isStr(a) && any_isStr(b)) {
        Str aStr = any_to_str(a);
        Str bStr = any_to_str(b);
        return str_compare(aStr, bStr);
    }
    if (any_isStr(a)) { return -1; }
    if (any_isStr(b)) { return +1; }

    if (any_isPair(a) && any_isPair(b)) {
        int hc = any_compare(any_head(a), any_head(b));
        if (hc != 0) {
            return hc;
        }
        int tc = any_compare(any_tail(a), any_tail(b));
        return tc;
    }

    fatalError("any_compare: incomparable reprs a(%s) b(%s)", showRepr(a.repr), showRepr(b.repr));

}



bool not_(bool a) { return ! a; }

// TODO ? support for short-circuited behaviour, expect b to be a { [] -> Bool } thunk
bool boolOr(bool a, bool b) { return a || b; }
bool boolAnd(bool a, bool b) { return a && b; }

Type TypeConsUnary(Type a) {
    return (Type){};
}
Type TypeConsBinary(Type a, Type b) {
    return (Type){};
}

Type TypeConsFuncAny(Any a) {
    return (Type){};
}
Type TypeConsFuncStr(Str a) {
    return (Type){};
}


void any_matchNil(Any a) {
    if(any_isNil(a)) {
        return;
    }
    fatalError("matchNil: expected a nil, got (%s)", showAny(a));
}

bool any_tryMatchPair(Any a, Any * hd, Any * tl) {
    if(any_isPair(a)) {
        *hd = any_head(a);
        *tl = any_tail(a);
        return true;
    }
    else {
        return false;
    }
}


void any_matchPair(Any a, Any * hd, Any * tl) {
    bool ok = any_tryMatchPair(a, hd, tl);
    if (!ok) {
        fatalError("matchPair: expected a pair, got(%s)", showAny(a));
    }
}


void any_matchTuple1(Any a, Any * fst) {
    Any it = a;
    any_matchPair(it, fst, &it);
    any_matchNil(it);
}

void any_matchTuple2(Any a, Any * fst, Any * snd) {
    Any it = a;
    any_matchPair(it, fst, &it);
    any_matchPair(it, snd, &it);
    any_matchNil(it);
}






Any any_listAt (Any list, int pos0) {
    int pos = pos0;
    Any elem = {};
    while (any_iterate(&list, &elem)) {
        if (pos == 0) {
            return elem;
        }
        else {
            pos --;
        }
    }
    fatalError("any_listAt: expected a list of at least length (%d)", pos0);
}

// TODO
// Ref reverseOnto(Ref a, Ref b) {
// }


// TODO use a (List Str) repr, not Any
// TODO first need to give the codegen and runtime a common definition of a listStr repr.
Str strCat (ListStr a) {
    int len = 0;
    List it = a.elems;
    Str * elem = NULL;
    while (list_iterate(&strRepr.base, &it, (void**)&elem)) {
        len += elem->len;
    }
    char *resultStr = malloc_atomic_or_panic(len+1);
    int pos = 0;
    it = a.elems;
    while (list_iterate(&strRepr.base, &it, (void**)&elem)) {
        memcpy (resultStr+pos, elem->data, elem->len);
        pos += elem->len;
    }
    resultStr[len] = '\0';
    Str result = {false, len, resultStr};
    return result;
}


Str strJoin (Str delim, ListStr list) {
    int len = 0;
    Str delimStr = delim;
    List it = list.elems;
    Str * elem = NULL;
    bool first = true;
    while (list_iterate(&strRepr.base, &it, (void**)&elem)) {
        if (!first) {
            len += delimStr.len;
        }
        Str s = * elem;
        len += s.len;
        first = false;
    }
    char *resultStr = malloc_atomic_or_panic(len+1);
    int pos = 0;
    it = list.elems;
    first = true;
    while (list_iterate(&strRepr.base, &it, (void**)&elem)) {
        if (!first) {
            memcpy (resultStr+pos, delimStr.data, delimStr.len);
            pos += delimStr.len;
        }
        Str s = * elem;
        memcpy (resultStr+pos, s.data, s.len);
        pos += s.len;
        first = false;
    }
    resultStr[len] = '\0';
    Str result = {false, len, resultStr};
    return result;
}

Str strAdd(Str a, Str b) {
    size_t len = a.len + b.len;
    char *resultStr = malloc_atomic_or_panic(len+1);
    memcpy(resultStr, a.data, a.len);
    memcpy(resultStr+a.len, b.data, b.len);
    resultStr[len] = '\0';
    Str result = {false, len, resultStr};
    return result;
}

int strLen(Str a) {
    return a.len;
}

int strOrd(Str a) {
    Str s = a;
    if (s.len == 0) {
        // temporary compatibility fudge, js runtime returns NaN in this case
        return -1;
    }
    // if (s.len != 1) {
    //     fatalError("strOrd: expected a string of length 1 (%d)", s.len);
    // }
    return s.data[0];
}

Str strChr(int a) {
    int i = a;
    if (SAFETY_CHECK_ERROR(!(0 <= i && i <= 127))) {
        // only support 7-bit ASCII strings for now
        fatalError("strChr: expected a 7-bit char (%d)", i);
    }
    char *result = malloc_or_panic(2);
    result[0] = i;
    result[1] = '\0';
    return (Str){false, 1, result};
}

Str strCharAt(Str a, int b) {
    Str s = a;
    int i = b;
    if (!(0 <= i && i < s.len)) {
        // fatalError("strCharAt: position out of range (%d, %d)", i, s.len);
        // this matches the current js runtime behaviour and is used to detect the end of a string
        return (Str){false, 0, ""}; // TODO use a static reference to the empty string
        // TODO ? use dependent-types to ensure the position is within range ?
        // TODO ? re-enable the out-of-range fatal-error ?
    }
    char *result = malloc_or_panic(2);
    result[0] = s.data[i];
    result[1] = '\0';
    return (Str){false, 1, result};
}

MaybeChar strCharAtMb(Str a, int b) {
    Str s = a;
    int i = b;
    if (!(0 <= i && i < s.len)) {
        return (MaybeChar){false};
    }
    Char ch = { s.data[i] };
    return (MaybeChar){ true, ch };
}


Str char_concat (ListChar a) {
    List list = a.elems;
    int length = list_length(list);
    char * chars = malloc_atomic_or_panic(length + 1);
    char * ch = NULL;
    int pos = 0;
    while(list_iterate(&charRepr.base, &list, (void**)&ch)) {
        chars[pos] = *ch;
        pos += 1;
    }
    chars[length] = '\0';
    Str result = { false, length, chars };
    return result;
}



Any fixCurried; // = adaptFunction_RefRef_to_Ref(fix);

Any fix(Any f, Any x) {
    // fix f x = f (fix f) x
    return any_call (any_call (f, any_call (fixCurried, f)), x);
}

Any any_ifNil(Any a, Any kTF) {
    Any kT = {}, kF = {};
    any_matchTuple2(kTF, &kT, &kF);
    if (any_isNil(a)) {
        return any_call(kT, a);
    }
    else {
        return any_call(kF, a);
    }
}

Any any_ifPair(Any a, Any k) {
    if (any_isPair(a)) {
        return any_call(any_listAt(k, 0), a);
    }
    else {
        return any_call(any_listAt(k, 1), a);
    }
}

Any any_ifType(Any a, Any kYN) {
    Any kY = {}, kN = {};
    any_matchTuple2(kYN, &kY, &kN);
    if (any_isType(a)) {
        return any_call(kY, a);
    }
    else {
        return any_call(kN, a);
    }
}

Any any_ifBool(Any a, Any k) {
    if (any_isBool(a)) {
        return any_call(any_listAt(k, 0), a);
    }
    else {
        return any_call(any_listAt(k, 1), a);
    }
}

Any any_ifInt(Any a, Any k) {
    if (any_isInt(a)) {
        return any_call(any_listAt(k, 0), a);
    }
    else {
        return any_call(any_listAt(k, 1), a);
    }
}

Any any_ifStr(Any a, Any k) {
    if (any_isStr(a)) {
        return any_call(any_listAt(k, 0), a);
    }
    else {
        return any_call(any_listAt(k, 1), a);
    }
}

Any any_if(bool a, Any kTF) {
    if (a) {
        return any_call(any_listAt(kTF, 0), any_nil());
    }
    else {
        return any_call(any_listAt(kTF, 1), any_nil());
    }
}

Any any_error(Any msg) {
    fatalError("error: program invoked the \"error\" function\n    %s\n\n", showAny(msg));
}

Any any_loopOne (Any func, Any value) {
    Any loopResult = any_call(func, value);
    while (strEq(any_to_str(any_head(loopResult)), STR_continue)) {
        value = any_head(any_tail(loopResult));
        loopResult = any_call(func, value);
    }
    if (!(strEq(any_to_str(any_head(loopResult)), STR_break))) {
        fatalError("any_loopOne: loop must return either [\"break\", ...] or [\"continue\", ...]");
    }
    return any_head(any_tail(loopResult));
}

Any any_loopTwo (Any value, Any func) { return any_loopOne(func, value); }


Any any_break(Any a) {
    return any_tuple2(any_from_str(STR_break), a);
}

Any any_continue(Any a) {
    return any_tuple2(any_from_str(STR_continue), a);
}


// We'll only need grLoop/grWhile when the C codegen works with graph-instantiated+readback expressions.
// The C codegen is tied to using tree-types for now, so we shouldn't encounter grLoop/grWhile at present in practice.
Any any_grLoop (Any func, Any value) {
    fatalError("any_grWhile: not implemented");
    // The same as loopOne, except the value is assumed to already be in ["continue", value] form.
    // Any loopResult = value;
    // while (strEq(any_to_str(any_head(loopResult)), STR_continue)) {
    //     value = any_head(any_tail(loopResult));
    //     loopResult = any_call(func, value);
    // }
    // if (!(strEq(any_to_str(any_head(loopResult)), STR_break))) {
    //     fatalError("any_grLoop: loop must return either [\"break\", ...] or [\"continue\", ...]");
    // }
    // return any_head(any_tail(loopResult));
}

Any any_grWhile (Any aMb, Any b, Any func) {
    fatalError("any_grWhile: not implemented");
    // while (any_isPair(aMb)) {
    //     b = any_head(aMb);
    //     a = any_call(func, b);
    // }
    // return b
}



Any any_unknownVariable(Str name) {
    // This is a deferred error.
    // Sometimes it is useful allow the build to continue, even though we have already found an error.
    fatalError("any_unknownVariable: %s.", name.data);
}

Any any_unknownPrimitive(Str name) {
    // This is a deferred error.
    // Sometimes it is useful allow the build to continue, even though we have already found an error.
    fatalError("any_unknownPrimtive: %s.", name.data);
}


Any any_noOrA (bool cond, Any a) {
    if (cond) {
        return a;
    }
    else {
        return any_nil();
    }
}

Any any_noOrYesA (bool cond, Any a) {
    if (cond) {
        return any_pair(a, any_nil());
    }
    else {
        return any_nil();
    }
}

Any any_identity(Any a) {
    return a;
}

Any any_trace(Any msg, Any val) {
    // TODO if the only thing being printed is a multi-line string, then
    // TODO print multi-line strings on mutiple lines
    // TODO   (currently they appear as a single string with embedded "\n" escapes)
    fprintf(stderr, "C_RuntimeTrace: %s\n", showAny(msg));
    return val;
}

Any any_traceTwo(Any msg, Any k) {
    // TODO print multi-line strings on mutiple lines
    fprintf(stderr, "C_RuntimeTrace: %s\n", showAny(msg));
    return any_call(k, any_nil());
}

void sb_showRepr(StringBuffer * sb, Repr repr);
void sb_showReprData(StringBuffer * sb, Repr repr, const void * data);
void sb_showAny(StringBuffer * sb, Any a);


void sb_showSchemaData(StringBuffer * sb, Schema * schema, const void * data) {
    sb_printf(sb, "(%s: ", schema->name);
    for (int i=0; i!=schema->numFields; i++) {
        Field f = schema->fields[i];
        if (i != 0) {
            sb_printf(sb, ", ");
        }
        sb_printf(sb, "%s = ", f.name);
        sb_showReprData(sb, f.repr, VOID_PTR_ADD(data, f.offset, 1));
    }
    sb_printf(sb, ")");
}

void sb_showStr(StringBuffer *sb, Str in) {
    sb_append(sb, "\"", 1);
    for (int i=0; i != in.len; i++) {
        char c = in.data[i];
        if (c == '"') {
            sb_append(sb, "\\\"", 2);
        }
        else if (c == '\\') {
            sb_append(sb, "\\\\", 2);
        }
        else if (c >= 32 && c <= 126) {
            sb_append(sb, &c, 1);
        }
        else if (c == '\t') {
            sb_append(sb, "\\t", 2);
        }
        else if (c == '\n') {
            sb_append(sb, "\\n", 2);
        }
        else if (c == '\r') {
            sb_append(sb, "\\r", 2);
        }
        else {
            // TODO use unicode syntax "\u{??????}" for chars > 255
            // TODO or maybe arbitrary-width hex syntax "\x{??????}" for chars > 255
            // TODO   the core-level language can handle wide-chars without a full understanding of unicode
            // TODO   and sometimes wide non-unicode chars are needed
            // TODO whether a string is unicode or not is perhaps best left 
            // TODO   as a base-level language concept
            char hexChars[] = "0123456789ABCDEF";
            char hexEsc[] = "\\x??";
            hexEsc[2] = hexChars[(c >> 4) & 0x0F];
            hexEsc[3] = hexChars[ c       & 0x0F];
            sb_append(sb, hexEsc, 4);
        }
    }
    sb_append(sb, "\"", 1);
    return;
}


Any any_show(Any a) {
    StringBuffer sb;
    sb_init(&sb);
    sb_showAny(&sb, a);
    char * text = malloc_or_panic(sb.len + 1);
    memcpy(text, sb.data, sb.len);
    text[sb.len] = '\0';
    Str resultStr = { false, sb.len, text };
    Any resultAny = any_from_str(resultStr);
    sb_free(&sb);
    return resultAny;
}


Any primHpsHandlerMk(Any handlerMk, Any initState) {
    return any_call(handlerMk, initState);
}

// TODO this is just the "const" function, could just put in in with the primitives
Any primHpsCall_k (Any result, Any handler) {
    return result;
}
Any primHpsCall (Any action, Any handler) {
    // let k = (result) => (handler) => result
    // let result = action (k) (handler)
    Any k = adaptFunction_AnyAny_to_Any(primHpsCall_k);
    Any result = any_call(any_call(action, k), handler);
    return result;
}

Any primHpsDo_k(Any result, Any handler) {
    return any_pair(handler, any_pair(result, any_nil()));
}
Any primHpsDo (Any action, Any handler) {
    // let k = (result) => (handler) => [handler, [result, null]]
    // let [handler2, [result,]] = action (k) (handler)
    // return [handler2, [result, null]] 
    Any k = adaptFunction_AnyAny_to_Any(primHpsDo_k);
    Any result = any_call(any_call(action, k), handler);
    return result;
}



typedef struct {
    Any *data;
    int len;
    int capacity;
    int seqId;
} Array;
typedef Array * ArrayPtr;

// ReprTupleTail tail3ArrayRepr = {{Repr_TupleTail, sizeof(Array)}, (&(Field){"seqId",    &intRepr.base, offsetof(Array,seqId)   }), NULL};
// ReprTupleTail tail2ArrayRepr = {{Repr_TupleTail, sizeof(Array)}, (&(Field){"capacity", &intRepr.base, offsetof(Array,capacity)}), (&tail3ArrayRepr)};
// ReprTupleTail tail1ArrayRepr = {{Repr_TupleTail, sizeof(Array)}, (&(Field){"len",      &intRepr.base, offsetof(Array,len)     }), (&tail2ArrayRepr)};
// ReprTupleTail tail0ArrayRepr = {{Repr_TupleTail, sizeof(Array)}, (&(Field){"data",     &ptrRepr.base, offsetof(Array,data)    }), (&tail1ArrayRepr)};
// ReprTuple const arrayRepr = 
//     {{Repr_Tuple, sizeof(Array)}, 
//         (&(Schema){"Array", sizeof(Array), 4, 
//             (Field[])
//                 { {"data",     &ptrRepr.base,  offsetof(Array,data)}
//                 , {"len",      &intRepr.base,  offsetof(Array,len)}
//                 , {"capacity", &intRepr.base,  offsetof(Array,capacity)}
//                 , {"seqId",    &intRepr.base,  offsetof(Array,seqId)}
//                 }})
//     , (&tail1ArrayRepr)
//     };


ArrayPtr arrayCopy(ArrayPtr arrayRef) {
    ArrayPtr array = arrayRef;
    Any *data = malloc_or_panic(array->len * sizeof(Any));
    memcpy(data, array->data, array->len * sizeof(Any));
    MALLOC(Array, copy, {data, array->len, array->len, 0});
    return copy;
}


typedef struct {
    Header hdr;
    ArrayPtr arrayR;
    int seqIdR;
} Env_Array;

Header Env_Array_Header = 
    { &(Schema){ "Env_Array", sizeof(Env_Array), 0, (Field[0]){
        // TODO ? populate this list of fields ?
        // TODO ?   it might be useful for diagnostic purposes
    } } };


Any ArrayFastAccessNoCopy_obj1(void *env, Any param);

Any ArrayFastAccessNoCopy_snapshot_restore(void *env0, Any param) {
    Env_Array *env = env0;
    any_matchNil(param);
    ArrayPtr snapshotArray = env->arrayR;
    // fprintf(stderr, "ARRAY: snapshot_restore: %ld\n", getArray(snapshotArray)->len);
    ArrayPtr newArray = arrayCopy(snapshotArray);
    // fprintf(stderr, "ARRAY: snapshot_restore: %ld\n", getArray(newArray)->len);
    int seqId = newArray->seqId;
    MALLOC(Env_Array, env2, { Env_Array_Header, newArray, seqId });
    Any arrayObj = adaptClosure_Any_to_Any(ArrayFastAccessNoCopy_obj1, env2);
    return arrayObj;
}
Any ArrayFastAccessNoCopy_obj_snapshot_create(ArrayPtr arrayRef) {
    // fprintf(stderr, "ARRAY: snapshot_create: %ld\n", getArray(arrayRef)->len);
    ArrayPtr newArray = arrayCopy(arrayRef);
    // fprintf(stderr, "ARRAY: snapshot_create: %ld\n", getArray(newArray)->len);
    MALLOC(Env_Array, env2, { Env_Array_Header, newArray, -1 });
    Any snapshot = adaptClosure_Any_to_Any(ArrayFastAccessNoCopy_snapshot_restore, env2);
    return snapshot;
}

Any ArrayFastAccessNoCopy_obj(ArrayPtr arrayR, int seqIdR, Any request) {
    ArrayPtr array = arrayR;
    int seqId = seqIdR;
    Any requestAny = request;
    if (SAFETY_CHECK_ERROR(seqId != array->seqId)) {
        fatalError("ArrayFastAccessNoCopy_obj: incorrect seqId (%d, %d)", seqId, array->seqId);
        // fprintf(stderr, "*** \n");
        // fprintf(stderr, "*** ArrayFastAccessNoCopy_obj: incorrect seqId (%d, %d)\n", seqId, array->seqId);
        // fprintf(stderr, "*** \n");
    }
    array->seqId += 1;
    seqIdR = array->seqId;
    MALLOC(Env_Array, newEnv, { Env_Array_Header, arrayR, seqIdR });
    Any arrayObj = adaptClosure_Any_to_Any(ArrayFastAccessNoCopy_obj1, newEnv);
    Any arrayObjAny = arrayObj;
    // fprintf(stderr, "ARRAY: %d %ld ", array->seqId, array->len); printRef(stderr, request); fprintf(stderr, "\n");
    // switch on request
    Str req = any_to_str(any_head(requestAny));
    if (strEq(req, STR_length)) {
        Any result = any_tuple2(arrayObj, any_from_int(array->len));
        return result;
    }
    else if (strEq(req, STR_get)) {
        int pos = any_to_int(any_listAt(requestAny, 1));
        if (SAFETY_CHECK_ERROR(!(0 <= pos && pos < array->len))) {
            fatalError("ArrayFastAccessNoCopy_obj: get: pos out of range (%d, %d)", pos, array->len);
        }
        Any val = array->data[pos];
        Any resultAny = any_tuple2(arrayObjAny, val);
        return resultAny;
    }
    else if (strEq(req, STR_set)) {
        int pos = any_to_int(any_listAt(requestAny, 1));
        Any val = any_listAt(requestAny, 2);
        if (SAFETY_CHECK_ERROR(!(0 <= pos && pos < array->len))) {
            fatalError("ArrayFastAccessNoCopy_obj: set: pos out of range (%d, %d)", pos, array->len);
        }
        array->data[pos] = val;
        Any resultAny = any_tuple2(arrayObjAny, any_nil());
        return resultAny;
    }
    else if (strEq(req, STR_extend)) {
        Any newElems = any_listAt(requestAny, 1);
        size_t newLen = array->len;
        Any it = newElems;
        Any elem = {};
        while (any_iterate(&it, &elem)) {
            newLen += 1;
        }
        // realloc in chunks proportional to current size, to prevent quadratic growing pains
        if (newLen > array->capacity) {
            size_t newCapacity = max(newLen, max(array->len * 2, 16));
            array->data = realloc_or_panic(array->data, newCapacity * sizeof(Any));
            array->capacity = newCapacity;
        }
        size_t pos = array->len;
        it = newElems;
        while (any_iterate(&it, &elem)) {
            array->data[pos++] = elem;
        }
        array->len = newLen;
        Any resultAny = any_tuple2(arrayObjAny, any_nil());
        return resultAny;
    }
    else if (strEq(req, STR_slice)) {
    }
    else if (strEq(req, STR_snapshot)) {
        Any snapshot = ArrayFastAccessNoCopy_obj_snapshot_create(arrayR);
        Any resultAny = any_tuple2(arrayObjAny, snapshot);
        return resultAny;
    }
    else {
    }
    fatalError("TODO: finish implementing ArrayFastAccessNoCopy_obj: %s", showAny(requestAny));
}

Any ArrayFastAccessNoCopy_obj1(void *env0, Any param) { 
    Env_Array *env = env0;
    ArrayPtr arrayR = env->arrayR;
    int seqIdR = env->seqIdR;
    Any result = ArrayFastAccessNoCopy_obj(arrayR, seqIdR, param);
    return result;
}

Any primMkArrayFastAccessNoCopy(Any repr, Any elems) {
    Any elemsAny = elems;
    // find length of list 
    size_t len = 0;
    Any it = elemsAny;
    Any elem = {};
    while (any_iterate(&it, &elem)) {
        len += 1;
    }
    // allocate array
    Any * data = malloc_or_panic(len * sizeof(Any));
    it = elemsAny;
    size_t pos = 0;
    // copy vals into array
    while (any_iterate(&it, &elem)) {
        data[pos++] = elem;
    }
    int seqId = 0;
    // fprintf(stderr, "ARRAY: mk: %ld\n", len);
    int capacity = len;
    MALLOC(Array, arrayR, {data, len, capacity, seqId});
    int seqIdR = seqId;
    // call into array-object constructor with vals and a fresh sequence-counter
    MALLOC(Env_Array, newEnv, { Env_Array_Header, arrayR, seqIdR });
    Any arrayObj = adaptClosure_Any_to_Any(ArrayFastAccessNoCopy_obj1, newEnv);
    return arrayObj;
}




Any ArrayFastAccessSlowCopy_obj1(void *env, Any param);

Any ArrayFastAccessSlowCopy_obj(ArrayPtr arrayR, Any request) {
    ArrayPtr array = arrayR;
    Any requestAny = request;
    MALLOC(Env_Array, newEnv, { Env_Array_Header, arrayR, -1 });
    Any arrayObj = adaptClosure_Any_to_Any(ArrayFastAccessSlowCopy_obj1, newEnv);
    // switch on request
    Str req = any_to_str(any_head(requestAny));
    if (strEq(req, STR_length)) {
        Any resultAny = any_tuple2(arrayObj, any_from_int(array->len));
        return resultAny;
    }
    else if (strEq(req, STR_get)) {
        int pos = any_to_int(any_listAt(requestAny, 1));
        if (SAFETY_CHECK_ERROR(!(0 <= pos && pos < array->len))) {
            fatalError("ArrayFastAccessSlowCopy_obj: get: pos out of range (%d, %d)", pos, array->len);
        }
        Any val = array->data[pos];
        Any resultAny = any_tuple2(arrayObj, val);
        return resultAny;
    }
    else if (strEq(req, STR_set)) {
        int pos = any_to_int(any_listAt(requestAny, 1));
        Any val = any_listAt(requestAny, 2);
        if (SAFETY_CHECK_ERROR(!(0 <= pos && pos < array->len))) {
            fatalError("ArrayFastAccessSlowCopy_obj: set: pos out of range (%d, %d)", pos, array->len);
        }
        // create a fresh copy of the data for every write
        size_t len = array->len;
        Any * data = malloc_or_panic(len * sizeof(Any));
        memcpy(data, array->data, len * sizeof(Any));
        data[pos] = val;
        // TODO drop old array
        MALLOC(Array, arrayR, {data, len, len, 0});
        MALLOC(Env_Array, newEnv, { Env_Array_Header, arrayR, -1 });
        Any arrayObj = adaptClosure_Any_to_Any(ArrayFastAccessSlowCopy_obj1, newEnv);
        Any resultAny = any_tuple2(arrayObj, any_nil());
        return resultAny;
    }
    else if (strEq(req, STR_extend)) {
        Any newElems = any_listAt(requestAny, 1);
        size_t len = array->len;
        Any it = newElems;
        Any elem = {};
        while (any_iterate(&it, &elem)) {
            len += 1;
        }
        // create a fresh copy of the data for every write
        Any * data = malloc_or_panic(len * sizeof(Any));
        memcpy(data, array->data, array->len * sizeof(Any));
        size_t pos = array->len;
        it = newElems;
        while (any_iterate(&it, &elem)) {
            data[pos++] = elem;
        }
        // TODO drop old array
        MALLOC(Array, arrayR, {data, len, len, 0});
        MALLOC(Env_Array, newEnv, { Env_Array_Header, arrayR, -1 });
        Any arrayObj = adaptClosure_Any_to_Any(ArrayFastAccessSlowCopy_obj1, newEnv);
        Any resultAny = any_tuple2(arrayObj, any_nil());
        return resultAny;
    }
    // else if (eq(req, STR_slice)) {
    // }
    // else if (eq(req, STR_snapshot)) {
    // }
    // else {
    // }

    fatalError("TODO: finish implementing ArrayFastAccessSlowCopy_obj: %s", showAny(requestAny));
}
Any ArrayFastAccessSlowCopy_obj1(void *env0, Any param) { 
    Env_Array *env = env0;
    ArrayPtr arrayR = env->arrayR;
    Any request = param;
    return ArrayFastAccessSlowCopy_obj(arrayR, request); 
}

Any primMkArrayFastAccessSlowCopy(Any repr, Any elems) {
    Any elemsAny = elems;
    // find length of list 
    size_t len = 0;
    Any it = elemsAny;
    Any elem = {};
    while (any_iterate(&it, &elem)) {
        len += 1;
    }
    // and allocate array
    Any * data = malloc_or_panic(len * sizeof(Any));
    it = elemsAny;
    size_t pos = 0;
    // copy vals into array
    while (any_iterate(&it, &elem)) {
        data[pos++] = elem;
    }
    int seqId = 0;
    MALLOC(Array, arrayR, {data, len, len, seqId});
    // call into array-object constructor with vals
    MALLOC(Env_Array, newEnv, { Env_Array_Header, arrayR, -1 });
    Any arrayObj = adaptClosure_Any_to_Any(ArrayFastAccessSlowCopy_obj1, newEnv);
    return arrayObj;
}




typedef struct {
    OrderedMapPtr map;
    int seqId;
} Assoc;

typedef Assoc * AssocPtr;

typedef struct {
    Header hdr;
    AssocPtr state;
    Str reqName;
} Env_AssocP;

Header Env_AssocP_Header = 
    { &(Schema){ "Env_AssocP", sizeof(Env_AssocP), 0, (Field[0]){
        // TODO ? populate these fields ?
    } } };

Any mkAssocObj_persistent2(void *env, Str param);

Any mkAssocObj_persistent(AssocPtr state, Str reqName, Any reqArgsAny) {
    AssocPtr assoc = state;
    Str req = reqName;
    Any reqArgs = reqArgsAny;
    if (strEq(req, STR_get)) {
        Any key = {};
        any_matchTuple1(reqArgs, &key);
        Any result = omap_get(assoc->map, key);
        if (result.repr == NULL) {
            result = any_nil();
        }
        else {
            result = any_pair(result, any_nil());
        }
        Str noReq = {};
        MALLOC(Env_AssocP, newEnv, { Env_AssocP_Header, state, noReq });
        Any obj = adaptClosure_Str_to_Any(mkAssocObj_persistent2, newEnv);
        return any_tuple2(obj, result);
    }
    else if (strEq(req, STR_set)) {
        Any keyVal = reqArgs;
        Any key = {};
        Any val = {};
        any_matchTuple2(keyVal, &key, &val);
        // copy-on-EVERY-write
        OrderedMapPtr om = omap_copy(assoc->map);
        if (any_isNil(val)) {
            omap_erase(om, key);
        }
        else {
            omap_set(om, key, any_head(val));
        }
        int seqId = 0;
        MALLOC(Assoc, newState, { om, seqId });
        Any result = any_nil();
        Str noReq = {};
        MALLOC(Env_AssocP, newEnv, { Env_AssocP_Header, newState, noReq });
        Any obj = adaptClosure_Str_to_Any(mkAssocObj_persistent2, newEnv);
        return any_tuple2(obj, result);
    }
    else if (strEq(req, STR_persistent)) {
        Str noReq = {};
        MALLOC(Env_AssocP, newEnv, { Env_AssocP_Header, state, noReq });
        Any obj = adaptClosure_Str_to_Any(mkAssocObj_persistent2, newEnv);
        return any_tuple2(obj, obj);
    }
    else if (strEq(req, STR_ephemeral)) {
        Str noReq = {};
        MALLOC(Env_AssocP, newEnv, { Env_AssocP_Header, state, noReq });
        Any obj = adaptClosure_Str_to_Any(mkAssocObj_persistent2, newEnv);
        return any_tuple2(obj, obj);
    }
    else if (strEq(req, STR_copy)) {
        Str noReq = {};
        MALLOC(Env_AssocP, newEnv, { Env_AssocP_Header, state, noReq });
        Any obj = adaptClosure_Str_to_Any(mkAssocObj_persistent2, newEnv);
        return any_tuple2(obj, obj);
    }
    else {
        // printRef(stderr, reqName);
        fatalError("mkAssocObj_persistent: unknown request name: %s", showStr(reqName));
    }
    // fprintf(stderr, "\n"); printRef(stderr, reqName);
    // fprintf(stderr, "\n"); printRef(stderr, reqArgs);
    fatalError("TODO: finish implementing mkAssocObj_persistent: %s %s", showStr(reqName), showAny(reqArgs));
}

Any mkAssocObj_persistent1(void *env0, Any param) { 
    Env_AssocP *env = env0;
    AssocPtr state = env->state;
    Str reqName = env->reqName;
    Any reqArgs = param;
    return mkAssocObj_persistent(state, reqName, reqArgs);
}

Any mkAssocObj_persistent2(void *env0, Str param) { 
    Env_AssocP *env = env0;
    AssocPtr state = env->state;
    Str reqName = param;
    MALLOC(Env_AssocP, newEnv, { Env_AssocP_Header, state, reqName });
    return adaptClosure_Any_to_Any(mkAssocObj_persistent1, newEnv); 
}


Any primAssoc1MkPersistent(Any elemsAny) { 
    Any elems = elemsAny;
    OrderedMapPtr om = omap_init();
    Any it = elems;
    Any elem = {};
    while (any_iterate(&it, &elem)) {
        Any key = {}, val = {};
        any_matchTuple2(elem, &key, &val);
        omap_set(om, key, val);
    }
    int seqId = 0;
    MALLOC(Assoc, state, { om, seqId });
    Str noReq = {};
    MALLOC(Env_AssocP, newEnv, { Env_AssocP_Header, state, noReq });
    Any obj = adaptClosure_Str_to_Any(mkAssocObj_persistent2, newEnv);
    return obj;
}

typedef struct {
    Header hdr;
    AssocPtr state;
    int seqIdR;
    Str reqName;
} Env_AssocE;

Header Env_AssocE_Header = 
    { &(Schema){ "Env_AssocE", sizeof(Env_AssocE), 0, (Field[0]){
        // TODO ? populate these fields ?
    } } };



Any mkAssocObj_ephemeral2(void *env, Any param);

Any mkAssocObj_ephemeral(AssocPtr state, int seqIdR, Str reqName, Any reqArgsAny) {
    AssocPtr assoc = state;
    Str req = reqName;
    int seqId = seqIdR;
    Any reqArgs = reqArgsAny;
    if (SAFETY_CHECK_ERROR(seqId != assoc->seqId)) {
        fatalError("mkAssocObj_ephemeral: incorrect seqId (%d) expected (%d)", seqId, assoc->seqId);
        // fprintf(stderr, "*** \n");
        // fprintf(stderr, "*** mkAssocObj_ephemeral: incorrect seqId (%d) expected (%d)", seqId, assoc->seqId);
        // fprintf(stderr, "*** \n");
    }
    assoc->seqId += 1;
    seqIdR = assoc->seqId;
    if (strEq(req, STR_get)) {
        Any key = {};
        any_matchTuple1(reqArgs, &key);
        Any result = omap_get(assoc->map, key);
        if (result.repr == NULL) {
            result = any_nil();
        }
        else {
            result = any_pair(result, any_nil());
        }        
        MALLOC(Env_AssocE, newEnv, { Env_AssocE_Header, state, seqIdR, (Str){} });
        Any obj = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnv); 
        return any_tuple2(obj, result);
    }
    else if (strEq(req, STR_set)) {
        Any keyVal = reqArgs;
        Any key = {};
        Any val = {};
        any_matchTuple2(keyVal, &key, &val);
        // update the map in-place
        OrderedMapPtr om = assoc->map;
        if (any_isNil(val)) {
            omap_erase(om, key);
        }
        else {
            omap_set(om, key, any_head(val));
        }
        Any result = any_nil();
        MALLOC(Env_AssocE, newEnv, { Env_AssocE_Header, state, seqIdR, (Str){} });
        Any obj = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnv); 
        return any_tuple2(obj, result);
    }
    else if (strEq(req, STR_persistent)) {
        MALLOC(Env_AssocE, newEnvObj, { Env_AssocE_Header, state, seqIdR, (Str){} });
        Any obj = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnvObj); 
        // Ref newState = rAssoc(omap_copy(assoc->map), 0);
        MALLOC(Assoc, newState, { omap_copy(assoc->map), 0});
        Str noReq = {};
        MALLOC(Env_AssocP, newEnvResult, { Env_AssocP_Header, newState, noReq });
        Any result = adaptClosure_Str_to_Any(mkAssocObj_persistent2, newEnvResult);
        return any_tuple2(obj, result);
    }
    else if (strEq(req, STR_ephemeral)) {
        MALLOC(Env_AssocE, newEnvObj, { Env_AssocE_Header, state, seqIdR, (Str){} });
        Any obj = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnvObj); 
        int seqId2 = 0;
        // Ref newState = rAssoc(omap_copy(assoc->map), seqId2);
        MALLOC(Assoc, newState, { omap_copy(assoc->map), seqId2 });        
        int seqId2R = seqId2;
        MALLOC(Env_AssocE, newEnvResult, { Env_AssocE_Header, state, seqIdR, (Str){} });
        Any result = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnvResult); 
        return any_tuple2(obj, result);
    }
    else if (strEq(req, STR_copy)) {
        MALLOC(Env_AssocE, newEnvObj, { Env_AssocE_Header, state, seqIdR, (Str){} });
        Any obj = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnvObj); 
        int seqId2 = 0;
        // Ref newState = rAssoc(omap_copy(assoc->map), seqId2);
        MALLOC(Assoc, newState, { omap_copy(assoc->map), seqId2 });
        int seqId2R = seqId2;
        MALLOC(Env_AssocE, newEnvResult, { Env_AssocE_Header, newState, seqId2R, (Str){} });
        Any result = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnvResult); 
        return any_tuple2(obj, result);
    }
    else {
        // printRef(stderr, reqName);
        fatalError("mkAssocObj_ephemeral: unknown request name: %s", showStr(reqName));
    }
}
Any mkAssocObj_ephemeral1(void *env0, Any param) { 
    Env_AssocE *env = env0;
    AssocPtr state = env->state;
    int seqId = env->seqIdR;
    Str reqName = env->reqName;
    return mkAssocObj_ephemeral(state, seqId, reqName, param); 
}
Any mkAssocObj_ephemeral2(void *env0, Any param) { 
    Env_AssocE *env = env0;
    AssocPtr state = env->state;
    int seqId = env->seqIdR;
    Str reqName = any_to_str(param);
    MALLOC(Env_AssocE, newEnv, { Env_AssocE_Header, state, seqId, reqName });
    return adaptClosure_Any_to_Any(mkAssocObj_ephemeral1, newEnv); 
}

Any primAssoc1MkEphemeral(Any elemsAny) { 
    Any elems = elemsAny;
    OrderedMapPtr om = omap_init();
    Any it = elems;
    Any elem = {};
    while (any_iterate(&it, &elem)) {
        Any key = {}, val = {};
        any_matchTuple2(elem, &key, &val);
        omap_set(om, key, val);
    }
    int seqId = 0;
    MALLOC(Assoc, state, { om, seqId });
    int seqIdR = seqId;
    MALLOC(Env_AssocE, newEnv, { Env_AssocE_Header, state, seqIdR, (Str){} });
    Any obj = adaptClosure_Any_to_Any(mkAssocObj_ephemeral2, newEnv); 
    return obj;
}





Any tryReadFile(Str filenameStr) {
    Any result = any_nil();
    const char *filenameCStr = filenameStr.data;
    char buffer [4096];
    FILE *file = fopen(filenameCStr, "rb");
    if (file == NULL || ferror(file)) {
        goto exit;
    }
    Any chunks = any_nil();
    size_t numCharsRead = 0;
    do {
        numCharsRead = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (ferror(file)) {
            goto exit;
        }
        chunks = any_pair(any_from_str(str_new(buffer, numCharsRead)), chunks);
    }
    while (numCharsRead == sizeof(buffer));
    Any it = chunks;
    // reverse the chunks
    chunks = any_nil();
    Any chunk = {};
    while (any_iterate(&it, &chunk)) {
        chunks = any_pair(chunk, chunks);
    }
    ListStr chunks_listStr = { any_to_list(&strRepr.base, chunks) };
    result = any_pair(any_from_str(strCat(chunks_listStr)), any_nil());
    exit:
    if (file != NULL) {
        fclose(file);
    }
    // TODO drop chunks
    return result;
}

void writeFile(Str filenameStr, Str contents) {
    const char *filenameCStr = filenameStr.data;
    FILE *file = fopen(filenameCStr, "wb");
    if (ferror(file)) {
        goto exit;
    }
    fwrite(contents.data, sizeof(char), contents.len, file);
    if (ferror(file)) {
        goto exit;
    }
    exit:
    fclose(file);
    // freePtr(filenameCStr);
    return;
}

typedef struct {
    Header hdr;
    Any cmdLineArgs;
} Env_ioDo;

Header Env_ioDo_Header = 
    { &(Schema){ "Env_ioDo", sizeof(Env_ioDo), 1, (Field[1]){
        { "cmdLineArgs",   &anyRepr.base, offsetof(Env_ioDo, cmdLineArgs)   },
    } } };


Any ioDoPrim(Any cmdLineArgs, Any reqResp) {
    Any ioState = any_nil();
    Any exitValue = {};
    while (exitValue.repr == NULL) {
        Any req = {};
        Any kResp = {};
        any_matchTuple2(reqResp, &req, &kResp);
        Any reqName = {};
        Any reqArgs = {};
        any_matchPair(req, &reqName, &reqArgs);
        Any result = any_nil();
        Str reqNameStr = any_to_str(reqName);
        // fprintf(stderr, "C_IO: "); printRef(stderr, req); fprintf(stderr, "\n"); fflush(stderr);
        // fprintf(stderr, "C_IO: %s\n", showRef(req)); fflush(stderr);
        // fprintf(stderr, "C_IO: %s\n", showRef(head(req))); fflush(stderr);
        if (strEq(reqNameStr, strC("readFile"))) {
            Any filename = any_head(reqArgs);
            Str filenameStr = any_to_str(filename);
            fprintf(stderr, "C_IO: readFile %s\n", showAny(filename)); fflush(stderr);
            Any contentsMb = tryReadFile(filenameStr);
            if (any_isNil(contentsMb)) {
                fatalError("ioDoPrim: readFile: failed to read file: %s", showAny(filename));
            }
            result = any_head(contentsMb);
        } 
        else if (strEq(reqNameStr, strC("readFile2"))) {
            Any filename = any_head(reqArgs);
            Str filenameStr = any_to_str(filename);
            fprintf(stderr, "C_IO: readFile2 %s\n", showAny(filename)); fflush(stderr);
            Any contentsMb = tryReadFile(filenameStr);
            if (any_isNil(contentsMb)) {
                result = any_tuple2(any_from_str(strC("Error")), any_from_str(strC("TODO: A more informative error message!")));
            }
            else {
                result = any_tuple2(any_from_str(strC("Ok")), any_head(contentsMb));
            }
        } 
        else if (strEq(reqNameStr, strC("writeFile"))) {
            Any filename = {}, contents = {};
            any_matchTuple2(reqArgs, &filename, &contents);
            fprintf(stderr, "C_IO: writeFile %s\n", showAny(filename)); fflush(stderr);
            Str filenameStr = any_to_str(filename);
            Str contentsStr = any_to_str(contents);
            writeFile(filenameStr, contentsStr);
            result = any_nil();
        } 
        else if (strEq(reqNameStr, strC("print"))) {
            Any arg = {};
            any_matchTuple1(reqArgs, &arg);
            // print on stdout, 
            // printf("C_IO Print "); printRef(stdout, arg); printf("\n");
            fprintf(stderr, "C_IO: print %s\n", showAny(arg)); fflush(stderr);
            printf("%s\n", showAny(arg));
            // or print on stderr
            // fprintf(stderr, "C_IO Print "); printRef(stderr, arg); fprintf(stderr, "\n"); fflush(stderr);
            result = any_nil();
        } 
        else if (strEq(reqNameStr, strC("getArgs"))) {
            any_matchNil(reqArgs);
            result = cmdLineArgs;
        } 
        else if (strEq(reqNameStr, strC("getFerrumDir"))) {
            const char *srcDir = getFeDir();
            if (srcDir == NULL) {
                result = any_nil();
            }
            else {
                result = any_pair(any_from_str(strC(srcDir)), any_nil());
            }
        } 
        else if (strEq(reqNameStr, strC("getEnvVar"))) {
            Any nameAny = {};
            any_matchTuple1(reqArgs, &nameAny);
            Str nameStr = any_to_str(nameAny);
            const char *name_cstr = nameStr.data;
            const char *value = getEnvVar(name_cstr);
            if (value == NULL) {
                result = any_nil();
            }
            else {
                result = any_pair(any_from_str(strC(value)), any_nil());
            }
        } 
        else if (strEq(reqNameStr, strC("get"))) {
            Any key = {}, mkInitVal = {};
            any_matchTuple2(reqArgs, &key, &mkInitVal);
            Any it = ioState, kv = {};
            while (any_iterate(&it, &kv)) {
                Any k = {}, v = {};
                any_matchTuple2(kv, &k, &v);
                if (any_eq(key, k)) {
                    result = v;
                    break;
                }
            }
            if (it.repr == NULL) {
                result = any_call(mkInitVal, any_nil());
            }
        } 
        else if (strEq(reqNameStr, strC("set"))) {
            Any keyVal = reqArgs;
            Any key = {}, val = {};
            any_matchTuple2(keyVal, &key, &val);
            Any it = ioState, kv = {};
            Any popped = any_nil();
            Any newIoState = {};
            while (any_iterate(&it, &kv)) {
                Any k = {}, v = {};
                any_matchTuple2(kv, &k, &v);
                if (any_eq(key, k)) {
                    newIoState = any_pair(keyVal, it);
                    break;
                }
                else {
                    popped = any_pair(kv, popped);
                }
            }
            if (it.repr == NULL) {
                newIoState = any_pair(keyVal, ioState);
            }
            else {
                Any kv = {};
                while(any_iterate(&popped, &kv)) {  
                    newIoState = any_pair(kv, newIoState);
                }
            }
            ioState = newIoState;
            result = any_nil();
        }
        else if (strEq(reqNameStr, strC("done")) || strEq(reqNameStr, strC("exit"))) {
            Any arg = {};
            any_matchTuple1(reqArgs, &arg);
            exitValue = arg;
            break;
        } 
        else {
            fatalError("ioDoPrim: unknown request: %s", showAny(reqName));
        }
        reqResp = any_call(kResp, result);
    }
    return exitValue;
}
Any ioDoPrim1(void *env0, Any param) { 
    Env_ioDo *env = env0;
    Any cmdLineArgs = env->cmdLineArgs;
    return ioDoPrim(cmdLineArgs, param); 
}
Any ioDoPrimCurried;


Any parseData(const char *in);

void ioDoAsyncProxy(Any rr) {
    Any req = {}, kResp = {};
    any_matchTuple2(rr, &req, &kResp);
    while (!any_isNil(kResp)) {
        // fprintf(stderr, "CAIO_Request: %s\n", showAny(req));
        printf("%s\n", showAny(req)); fflush(stdout);
        char *line = NULL;
        size_t len = 0;
        getline(&line, &len, stdin);
        // fprintf(stderr, "CAIO_Response: %s\n", line);
        Any resp = parseData(line);
        rr = any_call(kResp, resp);
        any_matchTuple2(rr, &req, &kResp);
    }
    printf("%s\n", showAny(req));
}



typedef struct {
    const char *in;
    const size_t len;
    int pos;
} ParseState;


void ps_skipWhitespace(ParseState *ps) {
    if (ps->pos > ps->len) {
        fatalError("skipWhitespace: invalid position, past end of string (%d) > (%d)", ps->pos, ps->len);
    }
    if (ps->pos == ps->len) {
        return;
    }
    char c = ps->in[ps->pos];
    while (ps->pos < ps->len && (c == ' ' || c == '\n' || c == '\t' || c == '\r')) {
        ps->pos += 1;
        c = ps->in[ps->pos];
    }
}

bool ps_trySkipStr(ParseState *ps, const char *expectedStr) {
    size_t expectedLen = strlen(expectedStr);
    if (ps->pos + expectedLen > ps->len) {
        return false;
    }
    if (strncmp(ps->in+ps->pos, expectedStr, expectedLen) == 0) {
        ps->pos += expectedLen;
        return true;
    }
    else {
        return false;
    }
}

bool ps_trySkipChar(ParseState *ps, char expectedChar) {
    if (ps->pos > ps->len) {
        fatalError("trySkipChar: invalid position, past end of string (%d) > (%d)", ps->pos, ps->len);
    }
    if (ps->pos == ps->len) {
        return false;
    }
    char c = ps->in[ps->pos];
    if (c == expectedChar) {
        ps->pos += 1;
        return true;
    }
    else {
        return false;
    }
}

void ps_skipChar(ParseState *ps, char expectedChar) {
    if (ps->pos > ps->len) {
        fatalError("skipChar: invalid position, past end of string (%d) > (%d)", ps->pos, ps->len);
    }
    if (ps->pos == ps->len) {
        fatalError("skipChar: expected (%c) at (%d), not end of string", expectedChar, ps->pos);
    }
    char c = ps->in[ps->pos];
    if (c != expectedChar) {
        fatalError("skipChar: expected (%c) at (%d), not (%c)", expectedChar, ps->pos, c);
    }
    ps->pos += 1;
}

char ps_peekChar(ParseState *ps) {
    if (ps->pos > ps->len) {
        fatalError("skipChar: invalid position, past end of string (%d) > (%d)", ps->pos, ps->len);
    }
    return ps->in[ps->pos];
}

void ps_takeChar(ParseState *ps) {
    if (ps->pos > ps->len) {
        fatalError("skipChar: invalid position, past end of string (%d) > (%d)", ps->pos, ps->len);
    }
    ps->pos++;
}


Any ps_parseInteger(ParseState *ps) {
    char c = ps_peekChar(ps);
    int num = 0;
    while ('0' <= c && c <= '9') {
        ps_takeChar(ps);
        num *= 10;
        num += c - '0';
        c = ps_peekChar(ps);
    }
    return any_from_int(num);
}

Any ps_parseString(ParseState *ps) {
    StringBuffer sb;
    sb_init(&sb);
    ps_skipChar(ps, '"');
    while (!ps_trySkipChar(ps, '"')) {
        char c = ps_peekChar(ps);
        ps_takeChar(ps);
        if (c == '\\') {
            c = ps->in[(ps->pos)++];
            switch (c) {
                case 'n':  sb_append(&sb, "\n", 1); break;
                case 'r':  sb_append(&sb, "\r", 1); break;
                case 't':  sb_append(&sb, "\t", 1); break;
                case '\\': sb_append(&sb, "\\", 1); break;
                case '"':  sb_append(&sb, "\"", 1); break;
                default:
                    fatalError("parseString: unknown escape char (%d, %c)", c, c);
            }
        }
        else {
            sb_append(&sb, &c, 1);
        }
    }
    Any result = any_from_str(str_new(sb.data, sb.len));
    sb_free(&sb);
    return result;
}


Any ps_parseData(ParseState *ps);


Any ps_parseTuple(ParseState *ps) {
    ps_skipChar(ps, '[');
    Any elems = any_nil();
    Any tupleTail = any_nil();
    bool first = true;
    while (!ps_trySkipChar(ps, ']')) {
        ps_skipWhitespace(ps);
        if (!first) {
            ps_skipChar(ps, ',');
            // if this is actually a double comma, then
            if (ps_trySkipChar(ps, ',')) {
                // parse the explicit tail, and end the tuple parsing
                tupleTail = ps_parseData(ps);
                ps_skipWhitespace(ps);
                ps_skipChar(ps, ']');
                break;
            }
        }
        Any elem = ps_parseData(ps);
        ps_skipWhitespace(ps);
        elems = any_pair(elem, elems);
        first = false;
    }
    Any it = elems;
    Any elem = {};
    elems = tupleTail;
    while (any_iterate(&it, &elem)) {
        elems = any_pair(elem, elems);
    }
    return elems;
}


Any ps_parseData(ParseState *ps) {
    ps_skipWhitespace(ps);
    char c = ps_peekChar(ps);
    if ('0' <= c && c <= '9') {
        return ps_parseInteger(ps);
    }
    else if (c=='"') {
        return ps_parseString(ps);
    }
    else if (c=='[') {
        return ps_parseTuple(ps);
    }
    else if (ps_trySkipStr(ps, "true")) {
        return any_from_bool(true);
    }
    else if (ps_trySkipStr(ps, "false")) {
        return any_from_bool(false);
    }
    else {
        fatalError("parseData, unexpected character (%d, %c)", c, c);
    }
}

Any parseData(const char *str) {
    size_t len = strlen(str);
    ParseState ps = { str, len, 0 };
    Any data = ps_parseData(&ps);
    return data;
}




Any any_jsEval(Any jsExprRef) {
    system("rm gen/test4c-jsEval-output.txt");
    system("rm gen/test4c-jsEval-result.txt");
    Str jsExprStr = any_to_str(jsExprRef);
    ListStr jsProgramLines = 
        str_list_c
            ( strC("")
            , strC("import * as fs from 'node:fs'")
            , strCat(str_list_c(strC("import { primitivesCore, runtimeUtils } from '"), strC(getFeDir()), strC("/ts/gen/runtime/runtime-core.js'"), (Str){}))
            , strCat(str_list_c(strC("import { primitivesNodeJs } from '"), strC(getFeDir()), strC("/ts/gen/runtime/runtime-nodejs.js'"), (Str){}))
            , strC("const rt = runtimeUtils")
            , strC("const p = { ...primitivesCore, ...primitivesNodeJs }")
            , strC("")
            , strC("const func7 = (rt, _) => (")
            , jsExprStr
            , strC(")")
            , strC("const result7 = func7(rt, p)")
            , strC("const result7Str = rt.showValueFerrum(result7, true)")
            // , strC("console.log(JSON.stringify(rt.feData_toJson(result7)))")
            // , strC("console.log(rt.showValueFerrum(result7, true))")
            , strC("fs.writeFileSync(\"gen/test4c-jsEval-result.txt\", result7Str)")
            , (Str){}
            );
    Str jsProgram = strJoin(strC("\n"), jsProgramLines);
    writeFile(strC("gen/test4c-jsEval.mjs"), jsProgram);
    system("node -stack-size=10000 gen/test4c-jsEval.mjs > gen/test4c-jsEval-output.txt");
    Any output = tryReadFile(strC("gen/test4c-jsEval-result.txt"));
    Str out = any_to_str(any_head(output));
    int pos = 0;
    ParseState ps = { out.data, out.len, 0 };
    Any data = ps_parseData(&ps);
    return data;
}

// Any any_jsEval(Any jsExprRef) {
//     system("rm gen/test4c-jsEval-output.txt");
//     system("rm gen/test4c-jsEval-result.txt");
//     Str jsExprStr = any_to_str(jsExprRef);
//     ListStr jsProgramLines = 
//         str_list_c
//             ( strC("")
//             , strC("import * as fs from 'node:fs'")
//             // , strC("import * as runtime from '../../../langs/js/runtime/runtime2.js'")
//             // , strCat(str_list_c(strC("import * as runtime from '"), strC(getFeDir()), strC("/langs/js/runtime/runtime2.js'"), (Str){}))
//             , strCat(str_list_c(strC("import * as rt from '"), strC(getFeDir()), strC("/bootstrap/ts/gen/runtime.js'"), (Str){}))
//             // , strC("let rt = runtime.exports")
//             , strC("let p = rt.primitives")
//             , strC("let func7 = (rt, _) => (")
//             , jsExprStr
//             , strC(")")
//             , strC("let result7 = func7(rt, p)")
//             , strC("let result7Str = rt.showValueFerrum(result7, true)")
//             // , strC("console.log(JSON.stringify(rt.dataFeToJs(result7)))")
//             // , strC("console.log(rt.showValueFerrum(result7, true))")
//             , strC("fs.writeFileSync(\"gen/test4c-jsEval-result.txt\", result7Str)")
//             , (Str){}
//             );
//     Str jsProgram = strJoin(strC("\n"), jsProgramLines);
//     writeFile(strC("gen/test4c-jsEval.js"), jsProgram);
//     system("deno run -A --v8-flags=--stack-size=4000,--max-heap-size=4000 gen/test4c-jsEval.js > gen/test4c-jsEval-output.txt");
//     Any output = tryReadFile(strC("gen/test4c-jsEval-result.txt"));
//     Str out = any_to_str(any_head(output));
//     int pos = 0;
//     ParseState ps = { out.data, out.len, 0 };
//     Any data = ps_parseData(&ps);
//     return data;
// }

// TODO implement jsEvalMaybe correctly, so as to fail gracefully
//   this just calls the ungraceful version
Any any_jsEvalMaybe(Any jsExprRef) { return any_pair(any_jsEval(jsExprRef), any_nil()); }



Type any_Primitive(Str name) {
    return (Type){};
}

// Ref todo(Ref a) {
//     Str str = getStr(a);
//     fatalError("TODO: not implemented (%s)", str.data);
// }

void initPrimitives(int argc, const char *argv[]) {

    // hashConsMap = omap_init();

    fixCurried = adaptFunction_AnyAny_to_Any(fix);

    Any cmdLineArgs = any_nil();
    // skip the programs own name in the command line args
    for (int i=0; i < argc-1; i++) {
        Any arg = any_from_str(strC(argv[argc-1-i]));
        cmdLineArgs = any_pair(arg, cmdLineArgs);
    }

    MALLOC(Env_ioDo, newEnv, { Env_ioDo_Header, cmdLineArgs });
    ioDoPrimCurried = adaptClosure_Any_to_Any(ioDoPrim1, newEnv);

}

// This provides access to the Ferrum source directory.
// This means the correct behaviour of the code is no longer dependent on where the program is launched from.
// This is currently only used to access the "runtime2.js" file when running generated JS code through jsEval in the C runtime.
// An alternative would be to include the "runtime2.js" verbatim within the generated C code.
const char * feSrcDir = NULL;
void setFeDir(const char *srcDir) { 
    feSrcDir = srcDir; 
    printf("C_setFeDir: %s\n", feSrcDir);
}
const char * getFeDir() {
    const char *ferrumDir = getenv("ferrumDir");
    if (ferrumDir == NULL) {
        fatalError("The environment variable \"ferrumDir\" must be set");
    }
    // printf("C_getFeDir: %s\n", feSrcDir);
    return ferrumDir;
}

const char * getEnvVar(const char *name) {
    const char *value = getenv(name);
    // printf("C_getEnvVar: %s %s\n", name, value);
    return value;
}

// TODO ? add the feSrcDir as an additional arg to initRuntime ?
void initRuntime(int argc, const char *argv[]) {
    initPrimitives(argc, argv);
}



void sb_showRepr(StringBuffer * sb, Repr repr) {
    switch (repr->tag) {
        case Repr_Hdr:
            sb_printf(sb, "Hdr");
            break;
        case Repr_Bool:
            sb_printf(sb, "Bool");
            break;
        case Repr_Int:
            sb_printf(sb, "Int");
            break;
        case Repr_Str:
            sb_printf(sb, "Str");
            break;
        case Repr_Func: {
            ReprFunc * repr2 = (ReprFunc *) repr;
            sb_printf(sb, "(Func ");
            for (int i=0; i!=repr2->numParam; i++) {
                sb_showRepr(sb, repr2->domRs[i]);
                sb_printf(sb, " -> ");
            }
            sb_showRepr(sb, repr2->codR);
            sb_printf(sb, ")");
            break;
        }
        case Repr_Clos: {
            ReprClos * repr2 = (ReprClos *) repr;
            sb_printf(sb, "(Clos ");
            for (int i=0; i!=repr2->numParam; i++) {
                sb_showRepr(sb, repr2->domRs[i]);
                sb_printf(sb, " -> ");
            }
            sb_showRepr(sb, repr2->codR);
            sb_printf(sb, ")");
            break;
        }
        case Repr_PartialApply: {
            sb_printf(sb, "(PartialApply ");
            sb_printf(sb, ")");
            break;
        }
        case Repr_Pair: {
            ReprPair * repr2 = (ReprPair *) repr;
            sb_printf(sb, "Pair");
            break;
        }
        case Repr_List: {
            ReprList * repr2 = (ReprList *) repr;
            sb_printf(sb, "List ");
            sb_showRepr(sb, repr2->elem);
            break;
        }
        case Repr_Tuple: {
            ReprTuple * repr2 = (ReprTuple *) repr;
            sb_printf(sb, "Tuple %d", repr2->schema->numFields);
            for (int i=0; i != repr2->schema->numFields; i++) {
                sb_printf(sb, " ");
                sb_showRepr(sb, repr2->schema->fields[i].repr);
            }
            break;
        }
        case Repr_No:
            sb_printf(sb, "No");
            break;
        case Repr_Yes: {
            ReprYes * repr2 = (ReprYes*) repr;
            sb_printf(sb, "Yes ");
            sb_showRepr(sb, repr2->elemRepr);
            break;
        }
        case Repr_Maybe: {
            ReprMaybe * repr2 = (ReprMaybe*) repr;
            sb_printf(sb, "Maybe ");
            sb_showRepr(sb, repr2->valueRepr);
            break;
        }
        case Repr_Any:
            sb_printf(sb, "Any");
            break;
        case Repr_Type:
            sb_printf(sb, "Type");
            break;
        case Repr_Union: {
            ReprUnion * repr2 = (ReprUnion *) repr;
            sb_printf(sb, "Union %d", repr2->numAlts);
            for (int i=0; i != repr2->numAlts; i++) {
                sb_printf(sb, " ");
                sb_showRepr(sb, repr2->alts[i]);
            }
            break;
        }
        case Repr_Ptr: {
            ReprPtr *ptrRepr = (ReprPtr*) repr;
            sb_printf(sb, "Ptr ");
            sb_showRepr(sb, ptrRepr->valueRepr);
            break;
        }
        case Repr_Single: {
            ReprSingle * repr2 = (ReprSingle*) repr;
            sb_printf(sb, "Single ");
            sb_showStr(sb, repr2->value);
            break;
        }
        default:
            // fatalError("sb_showRepr: unhandled case (%d)", repr->tag);
            fprintf(stderr, "sb_showRepr: unhandled case (%d)\n", repr->tag);
            sb_printf(sb, "(sb_showRepr: unhandled case (%d))", repr->tag);
    }
}

void sb_showReprData(StringBuffer * sb, Repr repr, const void * data) {
    // fprintf(stderr, "sb_showReprData: %16p - %20s\n", data, showRepr(repr));
    switch (repr->tag) {
        case Repr_Bool: {
            bool value = *(bool*) data;
            sb_printf(sb, "%s", value ? "true" : "false");
            break;
        }
        case Repr_Int: {
            int value = *(int*) data;
            sb_printf(sb, "%d", value);
            break;
        }
        case Repr_Str: {
            Str value = *(Str*) data;
            sb_showStr(sb, value);
            break;
        }
        case Repr_Char: {
            Char value = *(Char*) data;
            Str s = { false, 1, &value.value };
            sb_showStr(sb, s);
            break;
        }
        case Repr_Func: {
            ReprFunc * repr2 = (ReprFunc *) repr;
            sb_printf(sb, "#Func");
            break;
        }
        case Repr_Clos: {
            ReprFunc * repr2 = (ReprClos *) repr;
            sb_printf(sb, "#Clos");
            break;
        }
        case Repr_PartialApply: {
            PartialApply * pa = (PartialApply*) data;
            sb_printf(sb, "#PartialApply{ repr=%p, func=%p, env=%p, numArgs=%d args=[", pa->clos, pa->func, pa->env, pa->numArgs);
            for (int i=0; i != pa->numArgs; i++) {
                sb_showAny(sb, pa->args[i]);
                sb_printf(sb, ", ");
            }
            sb_printf(sb, "]}");
            break;
        }
        case Repr_Pair: {
            Pair p = *(Pair*) data;
            // TODO dont' construct the Any manually
            // Any it = (Any){ &pairRepr.base, data };
            Any it = any_from_value(repr, data);
            sb_printf(sb, "[");
            bool first = true;
            while(any_isPair(it)) {
                Any elem = any_head(it);
                if (!first) {
                    sb_printf(sb, ",");
                }
                sb_showAny(sb, elem);
                first = false;
                it = any_tail(it);
            }
            if (!any_isNil(it)) {
                sb_printf(sb, ",,");
                sb_showAny(sb, it);
            }
            sb_printf(sb, "]");
            break;
        }
        case Repr_List: {
            ReprList * repr2 = (ReprList *) repr;
            Repr elemRepr = repr2->elem;
            List lp = *(List*) data;
            void * elem = NULL;
            bool first  = true;
            sb_printf(sb, "[");
            while(list_iterate(elemRepr, &lp, &elem)) {
                if (!first) {
                    sb_printf(sb, ",");
                }
                sb_showReprData(sb, elemRepr, elem);
                first = false;
            }
            sb_printf(sb, "]");
            break;
        }
        case Repr_Tuple: {
            ReprTuple * repr2 = (ReprTuple *) repr;
            sb_printf(sb, "[");
            for (int i=0; i != repr2->schema->numFields; i++) {
                if (i != 0) {
                    sb_printf(sb, ",");
                }
                Field field = repr2->schema->fields[i];
                sb_showReprData(sb, field.repr, VOID_PTR_ADD(data, field.offset, 1));
            }
            sb_printf(sb, "]");
            break;
        }
        case Repr_TupleTail: {
            TupleTail * tt = (TupleTail*) data;
            sb_printf(sb, "[");
            for (int i=tt->pos; i != tt->tupleRepr->schema->numFields; i++) {
                if (i != tt->pos) {
                    sb_printf(sb, ",");
                }
                Field field = tt->tupleRepr->schema->fields[i];
                sb_showReprData(sb, field.repr, VOID_PTR_ADD(tt->tupleValue, field.offset, 1));
            }
            sb_printf(sb, "]");
            break;
        }
        case Repr_No:
            sb_printf(sb, "[]");
            break;
        case Repr_Yes: {
            ReprYes * repr2 = (ReprYes*) repr;
            sb_printf(sb, "[");
            sb_showReprData(sb, repr2->elemRepr, data);
            sb_printf(sb, "]");
            break;
        }
        case Repr_Maybe: {
            ReprMaybe * repr2 = (ReprMaybe*) repr;
            size_t isYesOffset = 0;
            size_t valueOffset = repr2->valueOffset;
            sb_printf(sb, "[");
            bool isYes = *(bool*) VOID_PTR_ADD(data, isYesOffset, 1);
            if (isYes) {
                sb_showReprData(sb, repr2->valueRepr, VOID_PTR_ADD(data, valueOffset, 1));
            }
            sb_printf(sb, "]");
            break;
        }
        case Repr_Any: {
            Any any = *(Any*) data;
            sb_showReprData(sb, any.repr, any.value);
            break;
        }
        case Repr_Type:
            sb_printf(sb, "#Type");
            break;
        case Repr_Union: {
            ReprUnion * repr2 = (ReprUnion *) repr;
            int tag = *(int*) VOID_PTR_ADD(data, 0, 1);
            const void * value = VOID_PTR_ADD(data, repr2->valueOffset, 1);
            Repr repr3 = repr2->alts[tag];
            sb_showReprData(sb, repr3, value);
            break;
        }
        case Repr_Ptr: {
            ReprPtr *ptrRepr = (ReprPtr*) repr;
            void **ptr = (void**) data;
            sb_showReprData(sb, ptrRepr->valueRepr, *ptr);
            break;
        }
        case Repr_Single: {
            ReprSingle * repr2 = (ReprSingle*) repr;
            sb_showStr(sb, repr2->value);
            break;
        }
        default:
            // fatalError("sb_showReprData: unhandled case (%d)", repr->tag);
            fprintf(stderr, "sb_showReprData: unhandled case (%d)\n", repr->tag);
            sb_printf(sb, "(sb_showReprData: unhandled case (%d))", repr->tag);
            
    }
}

void sb_showAny(StringBuffer * sb, Any a) {
    sb_showReprData(sb, a.repr, a.value);
}


const char * showRepr(Repr repr) {
    StringBuffer sb;
    sb_init(&sb);
    sb_showRepr(&sb, repr);
    return sb.data;
}

const char * showReprData(Repr repr, const void * data) {
    StringBuffer sb;
    sb_init(&sb);
    sb_showReprData(&sb, repr, data);
    return sb.data;
}

const char * showAny(Any any) {
    const char * result = showReprData(any.repr, any.value);
    return result;
}

const char * showStr(Str s) {
    StringBuffer sb;
    sb_init(&sb);
    sb_showStr(&sb, s);
    return sb.data;
}

