// TODO ? A better (less tedious) API for building graphs ?


import { assert } from "../utils/assert.js"
import { unit } from "../utils/unit.js"
import {
    Addr, Addr_of_Prim, Addr_of_TmApply, Addr_of_TmAs, Addr_of_TmDatum, Addr_of_TmOp0, Addr_of_TmOp1, Addr_of_TmOp2,
    Addr_of_TmPair, Addr_of_TmPrim, Addr_of_TmTyAnnot, Addr_of_TmVar, Addr_of_TyPrim0, Addr_of_TySingleStr, Addr_of_TyVar,
    AddrMb, Bool, Depth, Heap, Prim, TyOp1, TyOp2, TypeAddr, TypeAddrMb, TyPrim0, TyPrim1, TyPrim1Tm, TyPrim1Ty, TyPrim2, TyPrim3,
    Datum,
} from "./graph-heap2.js"


export type GraphBuilder = {

    // TODO For simple/typical use:
    // TODO   Add convenient functions for constructing TmLam and TyFun nodes.
    // TODO   These provide a new GraphBuilder to build their bodies at a deeper depth.
    tmLam(no: Bool, yes: Bool, patBodyCb: (gb: GraphBuilder) => [Addr, Addr], type?: TypeAddr): unit
    tyFun(no: TypeAddrMb, yes: TypeAddrMb, domCodCb: (gb: GraphBuilder) => [TypeAddr, TypeAddr], type?: TypeAddr): unit

    // TODO For more advanced use:
    // TODO   Provide a generic function for constructing nodes at a deeper depth.
    deeper(): GraphBuilder
    deeper(cb: (gb: GraphBuilder) => unit): unit
    deeper<T=unit>(cb: (gb: GraphBuilder) => T): T

    // These are the constructor functions from GraphHeap, but with the "depth" parameter removed.
    // The depth can/will be specified once in the call to mkGraphBuilder, rather than repeatedly for every node.
    tyVar(depth: Depth, type?: TypeAddrMb): Addr_of_TyVar
    tyApply(func: TypeAddr, argTm: AddrMb, argTy: TypeAddr, depth?: Depth, type?: TypeAddrMb): TypeAddr
    tyApplyTyTm(func: TypeAddr, arg: Addr, depth?: Depth, type?: TypeAddrMb): TypeAddr
    tyFun(no: TypeAddrMb, yes: TypeAddrMb, dom: TypeAddr, cod: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyPrim0(name: TyPrim0, type?: TypeAddrMb): TypeAddr
    tyPrim1(name: TyPrim1Tm, arg0: Addr, type?: TypeAddrMb): TypeAddr
    tyPrim1(name: TyPrim1Ty, arg0: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyPrim1(name: TyPrim1, arg0: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyPrim2(name: TyPrim2, arg0: TypeAddr, arg1: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyPrim3(name: TyPrim3, arg0: TypeAddr, arg1: TypeAddr, arg2: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyOp1(name: TyOp1, arg0: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyOp2(name: TyOp2, arg0: TypeAddr, arg1: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyCon1(name: TyOp1, arg0: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyCon2(name: TyOp2, arg0: TypeAddr, arg1: TypeAddr, type?: TypeAddrMb): TypeAddr
    tySingleStr(name: string): Addr_of_TySingleStr
    tyPair(hd: TypeAddr, tl: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyUnion(a: TypeAddr, b: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyIntersect(a: TypeAddr, b: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyRelComp(a: TypeAddr, b: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyHead(pairTy: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyTail(pairTy: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyDom(funTy: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyCod(funTy: TypeAddr, type?: TypeAddrMb): TypeAddr
    tyElem(listTy: TypeAddr, type?: TypeAddrMb): TypeAddr

    // Generic Term/Type Prim/Op primitive/builtin construction and access
    // prim(name: TyPrim, args: Addr[], type: TypeAddr): Addr_of_TyPrim
    prim(name: Prim, args: Addr[], type: TypeAddr): Addr_of_Prim


    tyPrim0(name: TyPrim0, type?: TypeAddrMb): Addr_of_TyPrim0

    tmVar(path: number[], type: TypeAddr): Addr_of_TmVar
    tmAs(var1: Addr, pat: Addr, type: TypeAddr): Addr_of_TmAs
    tmLam(no: Bool, yes: Bool, pat: Addr, body: Addr, type: TypeAddr): Addr
    tmDatum(datum: Datum, depth?: Depth, type?: TypeAddr): Addr_of_TmDatum
    tmPair(hd: Addr, tl: Addr, type?: TypeAddr): Addr_of_TmPair
    tmApply(fun: Addr, arg: Addr, type?: TypeAddr): Addr_of_TmApply
    tmPrim(name: string, args: Addr[], type: TypeAddr): Addr_of_TmPrim

    // TODO ? A more arity-precise version ?
    // tmPrim0(name: string, type: TypeAddr): Addr_of_TmPrim0
    // tmPrim1(name: string, arg0: Addr, type: TypeAddr): Addr_of_TmPrim1
    // tmPrim2(name: string, arg0: Addr, arg1: Addr, type: TypeAddr): Addr_of_TmPrim2
    // tmPrim3(name: string, arg0: Addr, arg1: Addr, arg2: Addr, type: TypeAddr): Addr_of_TmPrim3

    tmOp0(name: string, type: TypeAddr): Addr_of_TmOp0
    tmOp1(name: string, arg0: Addr, type: TypeAddr): Addr_of_TmOp1
    tmOp2(name: string, arg0: Addr, arg1: Addr, type: TypeAddr): Addr_of_TmOp2

    tmTyAnnot(term: Addr, type: TypeAddr): Addr_of_TmTyAnnot
}



export function mkGraphBuilder(h: Heap, d: Depth): GraphBuilder {
    assert.todo()
}

// TODO ? A fully lazy variant.
// TODO ? Build nodes at the shallowest permitted depth.
// This would make it easy to experiment with different approaches 
//   and use different approaches in different places.
export function mkGraphBuilder_fullyLazy(h: Heap, d: Depth): GraphBuilder {
    assert.todo()
}



