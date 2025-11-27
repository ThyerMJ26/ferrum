
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
// #include <stdalign.h>

#ifndef INCLUDE_RUNTIME_H
#define INCLUDE_RUNTIME_H


typedef struct {
    bool isStatic;
    size_t len;
    const char *data;
} Str;

typedef struct Char {
    char value;
} Char;

typedef enum {
    Repr_Hdr =  100,    // a pointer to something that starts with a Header (typically an environment)
    Repr_Bool = 120, Repr_Int, Repr_Str, Repr_Char, // unboxed datums
    Repr_Func = 140,  // an unboxed function
    Repr_Clos,        // an unboxed closure 
    Repr_FuncNull,    // an unboxed function/closure, known to take a NULL env
    Repr_Pair = 150,
    Repr_List,  // an unboxed List
    Repr_Tuple,       // an unboxed Tuple
    Repr_TupleTail,   // an unboxed Tuple tail
    Repr_No,
    Repr_Yes,
    Repr_Maybe,
    Repr_Any  = 160,  // a dynamic pointer to a static Repr and pointer to a heap/stack allocated value with a Header.
    Repr_Type = 170,
    Repr_Union = 180,
    Repr_Ptr,
    Repr_Single = 190, // a singleton, store the value once in the type-schema, and not in every runtime value
    Repr_PartialApply = 200,
    Repr_Object = 210,
} ReprTag;

typedef struct {
    ReprTag tag;
    size_t size;
    // TODO ? include alignof(), so as to place payloads close to the headers
    // TODO ?   the alternatives include 
    // TODO ?     - always allowing a multiple of max_align_t space for the header
    // TODO ?     - creating a new boxed struct type for every possible payload type
    // size_t align;
} ReprBase;

typedef const ReprBase *Repr;


typedef struct Schema Schema;
typedef struct Header Header;


typedef struct {
} No;

// typedef struct {
//     Repr repr; // a dynamic pointer to a pre-existing static Repr
//     // no new Reprs are created dynamically (unless perhaps a dynamic library is loaded at runtime)    
//     Header * hdr;
//     // the offset from the Header to the payload / raw data.
//     //   this would simplify pointing to:
//     //     -- values within tuples
//     //     -- values within ListSegments
//     //     -- values within tuples within ListSegments
//     //     -- values within tuples within tuples within ListSegments
//     //     -- values within closures (closure will probably be tuples in the future)
//     //   otherwise many more static Reprs maybe needed to represent nested values
//     // ptrdiff_t offset;
//     // Alternatively:
//     //   - rather than use a pointer and an offset,
//     //   - use two pointers, it comes to much the same thing, use (a, b-a) or (a, b)
//     const void * value;
// } Any;

typedef struct {
    Repr repr; // a dynamic pointer to a pre-existing static Repr
    // no new Reprs are created dynamically (unless perhaps a dynamic library is loaded at runtime)    

    // so long as the Boehm conservative GC is used, we don't need a header/resource pointer
    // in future this could be a pointer to a ref-count,
    //   or a stack-level, if using a stack of regions copying GC,
    //   or use a hybrid approach, 
    //     treat the current stack as a nursery generation,
    //     copy everything that outlives the stack into ref-counted memory.
    // Header * hdr;

    // TODO ? improve handling of Any values containing a List
    // TODO ?   store the List segment pointer in the header
    // TODO ?   and store the offset/value pointer in the value
    // TODO ? this (and special Reprs for TupleTails) should 
    // TODO ?   make it possible to traverse arbitrary data without
    // TODO ?   any reboxing and associated dynamic memory allocation
    // TODO ? ( need to be careful handling lists of nils,
    // TODO ?   switching from using an integer offset to a pointer
    // TODO ?   works fine, except when the element size is zero
    // TODO ? )

    const void * value;
} Any;


typedef struct {
    Any hd, tl;
} Pair;

typedef struct {
    ReprBase base;
} ReprAny;
extern ReprAny anyRepr;

typedef struct {
    ReprBase base;
} ReprRef;
extern ReprRef refRepr;

typedef struct {
    ReprBase base;
} ReprBool;
extern ReprBool boolRepr;

typedef struct {
    ReprBase base;
    bool isSigned;
    int byteWidth;
} ReprInteger;
extern ReprInteger intRepr;

typedef struct {
    ReprBase base;
} ReprString;
extern ReprString strRepr;

typedef struct {
    ReprBase base;
} ReprChar;
extern ReprChar charRepr;

typedef struct {
    ReprBase base;
    // still assuming/requiring all singleton-types to be string singleton-types, for now.
    Str value;
} ReprSingle;

typedef struct {
    ReprBase base;
} ReprType;
extern ReprType typeRepr;

typedef struct ClosureGeneric {
    const void* func;
    // TODO ? add a function name, for diagnostic purposes ?
    const void* env;
} ClosureGeneric;




// // FuncCurry makes a native function usable as an "Any" function
// typedef Any (*FuncCurry)(ClosureGeneric clos);
// // FuncUnCurry makes an "Any" function usable as a native function
// typedef ClosureGeneric (*FuncUnCurry) (Any);

// curry/uncurry take and return type-specific closures, not ClosureGeneric
// // FuncCurry makes a native function usable as an "Any" function
// typedef Any (*FuncCurry)(void * clos);
// // FuncUnCurry makes an "Any" function usable as a native function
// typedef void * (*FuncUnCurry) (Any);

// typedef Any (*FuncApply1)(const void *func, const void *env, Any arg);
// // there can be: too few, enough, or too many args, 
// // the "numArgs" and "args" variables are updated to indicate which args remains, if any.
// typedef Any (*FuncApplyN)(const void *func, const void *env, int * numArgs, Any * * args);

// takes a native type-specific function, and converts all arguments and result from/to "Any"
typedef Any (*FuncApplyFull)(const void *func, const void *env, int numArgs, Any arg[]);
// takes an "Any", returns a type-specific closure
typedef void * (*FuncFromAny) (Any);
// typedef void * (*AnyToFunc) (Any);
// typedef Any (*FuncToAny) (const void *func, const void *env);

// takes a native type-specific function, and converts all arguments and result from/to "Any"
// typedef Any (*FuncApply_StaticFunc_DynamicArgs)(const void *func, const void *env, int numArgs, Any arg[]);
typedef Any (*FuncApply_StaticFunc_DynamicArgs)(const void *func, const void *env, Any partialArgs [], Any lastArg);
// takes an "Any", returns a type-specific closure
typedef void * FuncApply_DynamicFunc_StaticArgs;
// typedef void * (*AnyToFunc) (Any);
// typedef Any (*FuncToAny) (const void *func, const void *env);


typedef struct {
    ReprBase const base;
    int const numParam;
    const Repr * const domRs;
    Repr const codR;

    // FuncCurry curry;
    // FuncUnCurry uncurry;
    // FuncApply1 apply1;
    // FuncApplyFull applyFull;
    // FuncFromAny fromAny;

    // call a function with a statically-known Repr in a dynamic context
    FuncApply_StaticFunc_DynamicArgs apply_sd;
    // call a function with a dynamically-known Repr in a static context
    FuncApply_DynamicFunc_StaticArgs apply_ds;

} ReprClos;

// a function Repr is the same as a closure Repr,
//   except:
//     - the adapter will still take an environment pointer, 
//         but will then ignore it (it should always be NULL in this case anyway), 
//         and call the native function without passing in the environment pointer
//     - the inverse adapter will always be NULL
//         it is impossible in general to convert an arbitrary closure to a function pointer
//           ( unless dynamic code generation is performed )
typedef ReprClos ReprFunc;

// a function-null Repr is the same as a function repr, 
//   except:
//     - the native function still takes a env-pointer, even though it will always be NULL,
//         this means a func-null is usable where a full closure is expected
typedef ReprClos ReprFuncNull;



typedef struct PartialApply {
    ReprClos const * clos;
    void const * func;
    void const * env;
    int numArgs;
    Any * args;
} PartialApply;

typedef struct ReprPartialApply {
    ReprBase const base;
} ReprPartialApply;
extern ReprPartialApply partialApplyRepr;


typedef struct ReprPair {
    ReprBase base;
    Pair value;
} ReprPair;
extern ReprPair pairRepr;


typedef struct {
    ReprBase base;
} ReprNo;
extern ReprNo noRepr;

typedef struct {
    ReprBase base;
    Repr elemRepr;
} ReprYes;

typedef struct {
    ReprBase base;
    Repr elem;
} ReprList;


typedef struct {
    ReprBase base;
    Repr valueRepr;
    size_t valueOffset;
} ReprMaybe;


typedef struct {
    ReprBase const base;
    int const numAlts;
    Repr * const alts;
    size_t valueOffset;
} ReprUnion;

typedef struct {
    ReprBase const base;
    Repr valueRepr;
} ReprPtr;


typedef struct Field {
    const char *name;
    Repr repr;
    size_t offset;
} Field;


typedef struct ReprTuple  ReprTuple;

typedef struct TupleTail {
    ReprTuple * tupleRepr;
    int pos;
    Header * hdr;
    const void * tupleValue;
} TupleTail;

typedef struct ReprTupleTail {
    ReprBase base;
    TupleTail value;
} ReprTupleTail;
extern ReprTupleTail tupleTailRepr;



typedef struct ReprTuple {
    ReprBase base;

    // TODO ? include Schema directly in TypeTuple / ReprTuple
    // TODO ? and remove the Schema definition, it only has meaning within a ReprTuple
    // TODO we use Schemas to describe closure environments too
    // TODO ?  perhaps clos-envs should have ReprTuple representations ?

    Schema *schema;
} ReprTuple;


typedef struct Schema {
    const char *name;
    size_t size;
    size_t numFields;
    Field *fields;
} Schema;

typedef struct Header {
    Schema *schema; // a Schema/Repr is needed for closure headers, but not in general, typically the Repr should be already known (or a ReprAny used)
    // int32_t refCount; // not currently used
    // bool onStack; // not currently used, could be combined with the refCount (e.g. positive => on-heap, negative => on-stack (or global))
    // Header *copiedTo // needed so as to maintain sharing, if copying-off-stack, ideally this would be unioned with the payload.
} Header;

// TODO ? memory-management header
// we can union the indirection/copiedTo field with the refCount, we'll never need both at the same time
// using stackFrameNum would make it possible to avoid heap allocation when returning pointers to values in an ancester's stack-frame
typedef struct MmHeader MmHeader;
struct MmHeader {
    union {
        MmHeader *indirect;     // ((value & 0x3) == 0) 
        uint64_t refCount;      // ((value & 0x3) == 1)
        uint64_t stackFrameNum; // ((value & 0x3) == 2)
        // onStack (somewhere)  // (value == 0)
    };
};


typedef struct List {
    struct ListSegment *segment;
    int offset;
} List;

typedef struct ListSegment {
    // TODO place a Header at the start, ListSegments need to be reference-counted, (or marked to indicate they are on the stack)
    // Header hdr
    size_t capacity;
    int numElems;
    void *elems;
    struct List tail;
    // purely for diagnostic and debugging purposes
    //   alternatively, if the Header contains a ReprList, then this will also have the elemRepr
    Repr elemRepr;
} ListSegment;



// typedef struct {
//     int numChunks;
//     uint32_t *chunks;
// } BigInt;

typedef struct {
} Type;

// typedef struct {
//     int tag;
//     // Header * hdr;
//     const void * value;
// } Union;

typedef struct Object {
    // Any *state;
    // Any methods;
    Any *clos;
} Object;

typedef struct {
    ReprBase const base;
} ReprObject;
extern ReprObject objectRepr;


// Auto-generated Reprs
typedef struct ListStr ListStr;      extern const ReprList listStr;
typedef struct MaybeChar MaybeChar;  extern const ReprMaybe maybeChar;
typedef struct ListChar ListChar;    extern const ReprList listChar;

// typedef struct ObjectMk ObjectMk;    extern const ReprClos objectMk;
typedef struct FunAnyAny FunAnyAny;  extern const ReprClos funAnyAny;



// ObjectMk objectMk_from_handlerMk(FunAnyAny handlerMk);







Str strStatic(const char *val, size_t len);


List list_prepend1 (Repr elemRepr, List lp, void * elem);
List list_prependN (Repr elemRepr, List lp, int numElems, void * elems);

bool list_isNil(List lp);
bool list_isPair(List lp);
void * list_head(Repr elemRepr, List lp);
List list_tail(Repr elemRepr, List lp);

// TODO ? a more imperative interface, update the listPtr in place, 
// TODO ?   make generated pattern matching code less cluttered / more readable ?
void   list_frontPush(Repr elemRepr, List * lp, int numElems, void * elems);
void * list_frontPop (Repr elemRepr, List * lp);

int list_length(List lp);
List list_reverse(Repr elemRepr, List lp);
void * list_lookup(Repr keyValRepr, Repr valMbRepr, Str key, List lp);


TupleTail tuple_tail(Repr tupleRepr, int pos, const void * value);

// // TODO holdRefs and dropRefs, for holding/dropping multiple refs in one call
// // TODO maintain a global ref-count and check it goes back to zero at the end of the program
// // TODO maintain a max ref-count and include it in diagnostic output
// void holdRef(Ref ref);
// void dropRef(Ref ref);

// TODO ? have "hold" return the reference, so that hold can be used in place ?
//   f(hold(a), hold(b))
//   f((Ref[2]){hold(a), hold(b)}, hold(c))
// Ref hold(Ref ref);

void initPrimitives(int argc, const char *argv[]);

void setFeDir(const char *);
const char * getFeDir();

const char * getEnvVar(const char *name);


Any any_head(Any);
Any any_tail(Any);
bool any_isNil(Any);
bool any_isPair(Any);

bool any_isBool(Any);
bool any_isInt(Any);
bool any_isStr(Any);
bool any_isType(Any);

bool any_iterate(Any * it, Any * elem);

int any_to_int(Any);
Any any_from_int(int);
No any_to_nil(Any);
Any any_from_nil(No);
bool any_to_bool(Any);
Any any_from_bool(bool);
Str any_to_str(Any);
Char any_to_char(Any);
Any any_from_str(Str);
Type any_to_type(Any);
Any any_from_type(Type);


void * any_to_tuple(const ReprTuple *repr, Any a);
Any any_from_tuple(const ReprTuple *repr, const void * a);
// void * any_to_maybe(const ReprMaybe *repr, Any a);
Any any_from_maybe(const ReprMaybe *repr, const void * a);
Any any_from_yes(const ReprYes *repr, const void * a);
List any_to_list(const Repr elemRepr, Any a);
Any any_from_list(const Repr elemRepr, List a);

// Union any_to_union(ReprUnion const * repr, Any a);
// Any any_from_union(ReprUnion const * repr, Union a);

void any_to_union    (Any a, ReprUnion const * outRepr, void * outValue);
bool any_try_to_union(Any a, ReprUnion const * outRepr, void * outValue);
Any any_from_union(ReprUnion const * repr, const void *a);

void any_to_value(Any in, const Repr outRepr, void *outValue);
bool any_try_to_value(Any in, const Repr outRepr, void *outValue);
Any any_from_value(const Repr inRepr, const void * inValue);

const void * any_to_value_ptr(Any in, const Repr outRepr);

Any any_to_any(Any a);


Any any_nil();
Any any_pair(Any h, Any t);

// // any_{to,from}_func take and return type-specific function pointers
// const void * any_to_func (const ReprFunc *repr, Any);
Any any_from_func (const ReprFunc *repr, const void *);

// any_to_clos returns a type-specific closure
const void * any_to_clos (const ReprFunc *repr, Any);

typedef struct AnyFuncClosEnv {
    Header hdr;
    Any func;
} AnyFuncClosEnv;
const AnyFuncClosEnv * any_func_to_clos_env(Any func);

// any_from_clos takes type-specific function and environment pointers
Any any_from_clos (const ReprClos *repr, const void * func, const void * env);
Any any_from_func_null (const ReprFuncNull *repr, const void * func);

Any any_call (Any func, Any arg);

bool any_eq(Any a, Any b);



int any_compare (Any a, Any b);


int add(int a, int b);
int sub(int a, int b);
int mul(int a, int b);

bool gt (int a, int b);
bool lt (int a, int b);
bool gte(int a, int b);
bool lte(int a, int b);

bool boolOr(bool a, bool b);
bool boolAnd(bool a, bool b);
bool not_(bool a);

Any any_noOrA (bool cond, Any a);
Any any_noOrYesA (bool cond, Any a);

Any any_if(bool a, Any kTF);

Any any_ifNil(Any a, Any kTF);
Any any_ifBool(Any a, Any k);
Any any_ifInt(Any a, Any k);
Any any_ifStr(Any a, Any k);
Any any_ifPair(Any a, Any k);
Any any_ifType(Any a, Any k);

Str strAdd(Str aR, Str bR);
int strLen(Str a);
Str strCat (ListStr a);
Str strJoin (Str delim, ListStr list);
int strOrd(Str a);
Str strChr(int a);
Str strCharAt(Str a, int b);
MaybeChar strCharAtMb(Str a, int b);
bool strEq(Str a, Str b);
bool str_char_eq(Str a, Char b);
bool char_eq(Char a, Char b);

Str char_concat (ListChar a);


Any any_loopOne (Any func, Any value);
Any any_loopTwo (Any value, Any func);

Any any_grLoop (Any func, Any value);
Any any_grWhile (Any valueMb, Any value, Any func);

Any any_unknownVariable(Str name);
Any any_unknownPrimitive(Str name);

Any any_break(Any a);
Any any_continue(Any a);

Type TypeConsUnary(Type a);
Type TypeConsBinary(Type a, Type b);

Type TypeConsFuncAny(Any a);
Type TypeConsFuncStr(Str a);


Any fix(Any f, Any x);

Any any_show(Any a);
Any any_identity(Any a);
Any any_trace(Any msg, Any val);
Any any_traceTwo(Any msg, Any k);
Any any_error(Any msg);

Any any_jsEval(Any jsExprRef);
Any any_jsEvalMaybe(Any jsExprRef);

Any primMkArrayFastAccessNoCopy(Any repr, Any elems);
Any primMkArrayFastAccessSlowCopy(Any repr, Any elems);

Any primAssoc1MkEphemeral(Any elems);
Any primAssoc1MkPersistent(Any elems);

Any primHpsCall (Any action, Any handler);
Any primHpsDo (Any action, Any handler);
Any primHpsHandlerMk(Any handlerMk, Any initState);

Type any_Primitive(Str name);
// The implementation of any_primitive is dynamically generated auxilliary code.
Any any_primitive(Str name);


extern Any ioDoPrimCurried;

void ioDoAsyncProxy(Any rr);

void printDiagnostics();

__attribute__((noreturn))
void fatalError(const char * fmt, ...);


extern const Str STR_break;
extern const Str STR_continue;
extern const Str STR_length;
extern const Str STR_get;
extern const Str STR_set;
extern const Str STR_extend;
extern const Str STR_slice;
extern const Str STR_snapshot;
extern const Str STR_persistent;
extern const Str STR_ephemeral;
extern const Str STR_copy;


const char * showStr(Str s);
const char * showAny(Any a);
const char * showRepr(Repr repr);
const char * showReprData(Repr repr, const void * data);



const void * mallocValue(Repr repr, const void * value);




void *malloc_or_panic(size_t size);
// void *void_ptr_as(const char *typeName, void *ptr);


#define MALLOC(type, name, ...) \
      type *name = malloc_or_panic(sizeof(type)); \
      *name = (type)__VA_ARGS__;

/*
#define MALLOC_STRUCT(type, name, ...) \
      type *name = malloc_or_panic(sizeof(type)); \
      *name = (type){ { &(type##_Class) }, __VA_ARGS__ };
*/
/*
#define MALLOC_STRUCT_N(type, name, OpenBrace, ...) \
      type *name = malloc_or_panic(sizeof(type)); \
      *name = (type){ OpenBrace { &(type##_Class) }, __VA_ARGS__ };
*/

#define MALLOC_STRUCT(type, name, OpenBrace, ...) \
      type *name = malloc_or_panic(sizeof(type)); \
      *name = (type) OpenBrace { &(type##_Class) }, __VA_ARGS__ ;

/*
#define MALLOC_ARRAY(type, name, numElems, ...) \
      type *name = malloc_or_panic(numElems * sizeof(type)); \
      *name = (type)__VA_ARGS__;
*/

// #define AS(type, ptr) ((type*) ptr)


// This implementation of MALLOC provides a very crude way to know 
// the type of a void-ptr at runtime
// TODO include a metadata/schema/class pointer at the beginning of every struct with this info
// TODO   this would also provide a generic place to keep track which Refs can be reach
// TODO   through a void-ptr, needed for reference-counting
/*
#define MALLOC(type, name, ...) \
      type *name##_0 = malloc_or_panic(sizeof(max_align_t) + sizeof(type)); \
      *((char**)name##_0) = #type; \
      type *name = (void*)(((char*)name##_0) + sizeof(max_align_t)); \
      *name = (type)__VA_ARGS__;
*/      

// #define AS(type, ptr) ((type*) void_ptr_as(#type, ptr))

#endif
