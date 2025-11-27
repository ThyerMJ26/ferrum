

import { unit } from "../utils/unit.js"
import { assert } from "../utils/assert.js"

import { Token, Pos, showPos } from "../syntax/token.js"
import { isAlpha, isAlphaUS } from "../syntax/scan.js"
import { ExprLoc, DeclLoc, eTuple, TorT } from "../syntax/expr.js"
import { LocField } from "../syntax/expr.js"
import { Loc } from "../syntax/token.js"

type ParseMode = "expr"


export type ParseExpr = ExprLoc
export type ParseDecl = DeclLoc

export class ParseState {
    input: Token[]
    pos: number

    tortStack: TorT[] = []

    constructor(input: Token[]) {
        this.input = input
        this.pos = 0
    }
    peek(): Token {
        if (!(this.pos < this.input.length)) throw new Error("past end of input")
        return this.input[this.pos]
    }
    take(): Token {
        if (!(this.pos < this.input.length)) throw new Error("past end of input")
        let token = this.input[this.pos]
        this.pos++
        return token
    }
    prev(): Token {
        if (this.pos === 0) {
            throw new Error("cannot call prev until a token has been parsed")
        }
        else {
            return this.input[this.pos - 1]
        }
    }
    eof(): boolean {
        return this.pos >= this.input.length
    }
    // srcPos(): Pos {
    //     return this.input[this.pos].start
    // }
    srcLoc(): Loc {
        return this.input[this.pos].loc
    }
    // srcPos2(): Pos {
    //     if (this.pos===0) {
    //         throw new Error("cannot call srcPos2 until a token has been parsed")
    //     }
    //     else {
    //         return this.input[this.pos].end // TODO? missing -1 on pos?
    //     }
    // }
    srcLoc2(): Loc {
        if (this.pos === 0) {
            throw new Error("cannot call srcLoc2 until a token has been parsed")
        }
        else {
            return this.input[this.pos - 1].loc
        }
    }
    tokenMatchTag(lookAhead: number, tag: string): boolean {
        if (this.pos + lookAhead >= this.input.length) {
            return false
        }
        let tok = this.input[this.pos + lookAhead]
        if (tok.tag === tag) {
            return true
        }
        return false
    }
    tokenMatch(lookAhead: number, tag: string, value: any): boolean {
        if (this.pos + lookAhead >= this.input.length) {
            return false
        }
        let tok = this.input[this.pos + lookAhead]
        if (tok.tag === tag && tok.value === value) {
            return true
        }
        return false
    }

    pushTorT(tort: TorT): unit {
        this.tortStack.push(tort)
    }
    peekTorT(): TorT {
        const result = this.tortStack.at(-1)
        assert.isTrue(result !== undefined)
        return result
    }
    popTorT(tort: TorT): unit {
        assert.isTrue(this.tortStack.length !== 0)
        assert.isTrue(this.tortStack.at(-1) === tort)
        this.tortStack.pop()
    }
}


// // This is for fe3, which is now obsolete

// export function parseKeyword(ps: ParseState, keyword: string) {
//     parseToken(ps, "keyword", keyword)
// }

// export function parseToken(ps: ParseState, tag: string, value: any): void {
//     let tok = ps.pop()
//     if (tok.tag !== tag || tok.value !== value)
//         // throw new Error(`unexpected token ${showPos(tok.start)} ${tok.tag} ${tok.value} expected ${tag} ${value}`)
//         throw new Error(`unexpected token ${token.showLoc(tok.loc)} ${tok.tag} ${tok.value} expected ${tag} ${value}`)
// }

// export function parseIdent(ps: ParseState): string {
//     let tok = ps.peek()
//     if (tok.tag === "ident") {
//         ps.pop()
//         return tok.value
//     }
//     else {
//         throw new Error(`expected identifier ${token.showLoc(tok.loc)}, not " ${tok.tag}, ${tok.value}`)
//     }
// }

// export function tryParseToken(ps: ParseState, tag: string, value: any): boolean {
//     let tok = ps.peek()
//     if (tok.tag === tag && tok.value === value) {
//         ps.pop()
//         return true
//     }
//     else {
//         return false
//     }
// }



// function parseExpr(ps: ParseState, pm: ParseMode = "expr"): ParseExpr {
//     return parseTypeAnnot(ps)
// }

// function parseTuple(ps: ParseState, pm: ParseMode = "expr"): ParseExpr {
//     let startLoc = ps.srcLoc()
//     if (tryParseToken(ps, "separator", "(")) {
//         if (tryParseToken(ps, 'separator', ')')) {
//             let loc = token.mergeLoc(startLoc, ps.srcLoc2())
//             return { tag: "EDatum", value: null, loc: token.mergeLoc(loc, ps.srcLoc2()) }
//         }
//         let exprs: ParseExpr[] = []
//         let expr = parseTypeAnnot(ps)
//         // return expr
//         exprs.push(expr)
//         while (tryParseToken(ps, "separator", ",")) {
//             let expr = parseTypeAnnot(ps)
//             exprs.push(expr)
//         }
//         parseToken(ps, 'separator', ')')
//         let loc = token.mergeLoc(startLoc, ps.srcLoc2())
//         if (exprs.length===1) {
//             return exprs[0]
//         }
//         else {
//             // return { tag: 'ETuple', loc: loc, exprs: exprs }
//             return eTuple(loc, exprs)
//         }
//     }
//     else {
//         return parseAlias(ps)
//     }
// }

// function parseTypeAnnot(ps: ParseState, pm: ParseMode = "expr"): ParseExpr {
//     let expr = parseAlias(ps)
//     if (tryParseToken(ps, "keysym", ":")) {
//         let type = parseAlias(ps)
//         return { tag: "EType", expr: expr, type: type, loc: token.mergeLoc(expr.loc, type.loc) }
//     }
//     else {
//         return expr
//     }
// }

// // function parseType(ps : ParseState) : ParseTree {

// // }

// function parseAlias(ps: ParseState, pm: ParseMode = "expr"): ParseExpr {
//     if (ps.tokenMatchTag(0, "ident") && ps.tokenMatch(1, "keysym", "@")) {
//         // let pos1 = ps.srcPos()
//         let loc1 = ps.srcLoc()
//         let name = parseIdent(ps)
//         parseToken(ps, "keysym", "@")
//         let expr = parseExpr1(ps)
//         // let pos2 = ps.srcPos()
//         let loc2 = ps.srcLoc2()
//         if (expr === null) {
//             // for type aliases
//             // TODO be more careful using the same parsing rules for expressions and types
//             // implicit "any" type aliases only make sense when parsing types, not expressions or patterns
//             expr = { tag: "EVar", name: "any", loc: token.mergeLoc(loc1, loc2) }
//         }
//         return { tag: "EAs", name: name, expr: expr, loc: token.mergeLoc(loc1, loc2) }
//     }
//     else {
//         let expr = parseApply(ps);
//         return expr
//     }
// }


// function parseApply(ps: ParseState, pm: ParseMode = "expr"): ParseExpr {
//     let expr = parseExpr1(ps)
//     if (expr === null) {
//         throw new Error(`failed to parse expression at (${JSON.stringify(ps.input[ps.pos])})`)
//     }
//     let opArgs = parseOpArgs(ps, expr)
//     let result = foldOpArgs(operatorTable, opArgs)
//     return result
// }

// function parseExpr1(ps: ParseState, pm: ParseMode = "expr"): ParseExpr | null {
//     if (ps.eof()) {
//         return null;
//     }
//     // let pos1 = ps.srcPos()
//     let loc1 = ps.srcLoc()
//     let tok = ps.peek()
//     switch (tok.tag) {
//         case "keyword":
//             switch (tok.value) {
//                 case "if":
//                     ps.pop()
//                     //                    let ifPos2 = ps.srcPos2()
//                     let ifPos2 = ps.srcLoc2()
//                     let expr1 = parseExpr(ps)
//                     // let condPos2 = ps.srcPos2()
//                     let condPos2 = ps.srcLoc2()
//                     parseKeyword(ps, "then")
//                     let expr2 = parseExpr(ps)
//                     // let truePos2 = ps.srcPos2()
//                     let truePos2 = ps.srcLoc2()
//                     parseKeyword(ps, "else")
//                     let expr3 = parseExpr(ps)
//                     // let falsePos2 = ps.srcPos2()
//                     let falsePos2 = ps.srcLoc2()
//                     let ap1: ParseExpr = { tag: "EApply", func: { tag: "EVar", name: "if", loc: token.mergeLoc(loc1, ifPos2) }, arg: expr1, loc: token.mergeLoc(loc1, condPos2), op: "" }
//                     let ap2: ParseExpr = { tag: "EApply", func: ap1, arg: expr2, loc: token.mergeLoc(loc1, truePos2), op: "" }
//                     let ap3: ParseExpr = { tag: "EApply", func: ap2, arg: expr3, loc: token.mergeLoc(loc1, falsePos2), op: "" }
//                     return ap3
//                 case "let": {
//                     ps.pop()
//                     parseToken(ps, "separator", "{")
//                     let decls = parseDecls(ps)
//                     parseToken(ps, "separator", "}")
//                     parseToken(ps, "keyword", "in")
//                     let expr = parseExpr(ps)
//                     // let pos2 = ps.srcPos2()
//                     let loc2 = ps.srcLoc2()
//                     let result: ParseExpr = { tag: "ELet", decls: decls, expr: expr, loc: token.mergeLoc(loc1, loc2) }
//                     return result
//                 }
//                 case "then":
//                 case "else":
//                     return null
//                 case "case": {
//                     ps.pop()
//                     let expr = parseExpr(ps)
//                     parseToken(ps, "keyword", "of")
//                     parseToken(ps, "separator", "{")
//                     let cases = parseCaseAlts(ps)
//                     parseToken(ps, "separator", "}")
//                     let loc2 = ps.srcLoc2()
//                     let result: ParseExpr = { tag: "ECase", expr: expr, alts: cases, loc: token.mergeLoc(loc1, loc2) }
//                     return result
//                 }
//                 case "of":
//                     return null

//                 default:
//                     throw new Error(`TODO keyword: ${JSON.stringify(tok)}`)

//             }
//         case "ident":
//             ps.pop()
//             let expr: ParseExpr = { tag: "EVar", loc: tok.loc!, name: tok.value }
//             return expr
//         case "integer":
//         case "string": {
//             ps.pop()
//             // let pos2 = ps.srcPos2()
//             let loc2 = ps.srcLoc2()
//             let litExpr: ParseExpr = { tag: "EDatum", value: tok.value, loc: token.mergeLoc(loc1, loc2) }
//             return litExpr
//         }
//         case "symbol": {
//             ps.pop()
//             // let pos2 = ps.srcPos2()
//             let loc2 = ps.srcLoc2()
//             let varExpr: ParseExpr = { tag: "EVar", name: tok.value, loc: token.mergeLoc(loc1, loc2) }
//             return varExpr
//         }
//         case "keysym":
//             switch (tok.value) {
//                 case "\\":
//                     let loc1 = tok.loc
//                     ps.pop()
//                     switch (pm) {
//                         case "expr":
//                         // case "type":
//                         {
//                             // TODO add result type back
//                             let resultType: ParseExpr | null = null
//                             let isBracketed = ps.tokenMatch(0, 'separator', '(')
//                             let arg: ParseExpr = parseTuple(ps)
//                             if (isBracketed && tryParseToken(ps, "keysym", ":")) {
//                                 resultType = parseExpr(ps)
//                             }

//                             if (tryParseToken(ps, "keysym", "->")) {
//                                 let body = parseExpr(ps)
//                                 let endLoc = ps.srcLoc2()
//                                 if (resultType !== null) {
//                                     // body = { tag: "EType", expr: body, type: resultType, loc: token.mergeLoc(body.loc, resultType.loc) }
//                                     body = { tag: "EType", expr: body, type: resultType, loc: body.loc } // TODO? revisit this?  this type location is non-local
//                                 }
//                                 return { tag: "ELambda", arg: arg, body: body, loc: token.mergeLoc(loc1, endLoc) }
//                             }
//                             else if (tryParseToken(ps, "keysym", "=>")) {
//                                 let body = parseExpr(ps)
//                                 let endLoc = ps.srcLoc2()
//                                 if (resultType !== null) {
//                                     // body = { tag: "EType", expr: body, type: resultType, loc: token.mergeLoc(body.loc, resultType.loc) }
//                                     body = { tag: "EType", expr: body, type: resultType, loc: body.loc } // TODO? revisit this?  this type location is non-local
//                                 }
//                                 return { tag: "TLambda", arg: arg, body: body, loc: token.mergeLoc(loc1, endLoc) }
//                             }
//                             else throw new Error(`expected either (->) or (=>) here ${token.showLoc(ps.srcLoc())}`)
//                         }
//                         // case "pat":
//                         //     throw new Error("lambda expressions not permitted within patterns")
//                     }
//                 default:
//                     return null
//                 //throw new Error ("unexpected symbol "+tok.value)
//             }
//         case "separator":
//             switch (tok.value) {
//                 case "(":
//                     ps.pop()
//                     if (tryParseToken(ps, "separator", ")")) {
//                         return { tag: "EDatum", value: null, loc: token.mergeLoc(loc1, ps.srcLoc2()) }
//                     }
//                     else {
//                         let elems: ParseExpr[] = []
//                         let expr, comma
//                         do {
//                             expr = parseExpr(ps)
//                             elems.push(expr)
//                             comma = tryParseToken(ps, "separator", ",")
//                         } while (comma)
//                         parseToken(ps, "separator", ")")
//                         let loc = token.mergeLoc(tok.loc, ps.srcLoc2())
//                         if (elems.length===1) {
//                             return elems[0]
//                         }
//                         else {
//                             // return { tag: 'ETuple', loc: loc, exprs: elems }
//                             return eTuple(loc, elems)
//                         }
//                     }
//                 case '[': {
//                     ps.pop()
//                     if (tryParseToken(ps, "separator", "]")) {
//                         return { tag: "EList", exprs: [], loc: token.mergeLoc(loc1, ps.srcLoc2()), tail: null }
//                     }
//                     let result: ParseExpr = { tag: 'EList', loc: tok.loc, exprs: [], tail: null }
//                     let comma
//                     do {
//                         let expr = parseExpr(ps)
//                         result.exprs.push(expr)
//                         comma = tryParseToken(ps, "separator", ",")
//                     }
//                     while (comma)

//                     // let result: ParseExpr = { tag:'PList', loc:tok.loc, exprs:[] }
//                     // let expr2: ParseExpr = expr
//                     // while (expr2.tag==='EPair') {
//                     //     result.exprs.push(expr2.hd)
//                     //     expr2 = expr2.tl
//                     // }
//                     // result.exprs.push(expr2)
//                     parseToken(ps, "separator", "]")
//                     result.loc = token.mergeLoc(result.loc, ps.srcLoc2())
//                     return result
//                 }
//                 case '{':
//                 //
//                 // TODO ? parse record expression
//                 //
//                 default:
//                     return null
//             }
//         // ( [
//         default:
//             // throw ["unexpected token", tok]
//             return null;
//     }
// }

// // function parseOp(ps: ParseState, expr: Expr): Expr {
// //     let expr2 = parseExpr1(ps)
// //     while (expr2 !== null) {
// //         expr = new EApply(expr, expr2)
// //         expr2 = parseExpr1(ps)
// //     }
// //     return expr
// // }

// function parseOpArgs(ps: ParseState, expr: ParseExpr, pm: ParseMode = "expr"): ParseExpr[] {
//     let opArgs = [expr]
//     let expr2 = parseExpr1(ps)
//     while (expr2 !== null) {
//         opArgs.push(expr2)
//         expr2 = parseExpr1(ps)
//     }
//     return opArgs
// }


// type assocT = "left" | "right" | "none"
// type fixityT = "infix" // | "prefix" | "postfix"

// class Operator {
//     constructor(public name: string, public fixity: fixityT, public assoc: assocT, public precedence: number) { }
// }

// export type OperatorTable = { [name: string]: Operator }
// let operatorTable: OperatorTable = {}

// export function addOps(operatorTable: OperatorTable, fixity: fixityT, prec: number, assoc: assocT, names: string[]) {
//     for (let name of names) {
//         operatorTable[name] = new Operator(name, fixity, assoc, prec);
//     }
// }

// addOps(operatorTable, "infix", 0, "right", ["$", "seq"]);
// addOps(operatorTable, "infix", 1, "left", [">>", ">>="]);
// addOps(operatorTable, "infix", 2, "right", ["||"]);
// addOps(operatorTable, "infix", 3, "right", ["&&"]);
// addOps(operatorTable, "infix", 4, "none", ["==", "/=", "<", "<=", ">=", ">", "elem", "notElem"]);
// addOps(operatorTable, "infix", 5, "right", [":", "++"]);
// addOps(operatorTable, "infix", 6, "left", ["+", "-"]);
// addOps(operatorTable, "infix", 7, "left", ["*", "/", "quot", "rem", "div", "mod", ":%", "%"]);
// addOps(operatorTable, "infix", 8, "right", ["^", "^^", "**"]);
// addOps(operatorTable, "infix", 9, "left", ["!!"]);
// addOps(operatorTable, "infix", 9, "right", ["."]);
// addOps(operatorTable, "infix", 11, "right", ["@"]);
// addOps(operatorTable, "infix", 5, "right", ["<|>", "<|>=", "<&>", "<&>="]);


// export function isOp(expr: ParseExpr): boolean {
//     return expr.tag === "EVar" && !isAlphaUS(expr.name[0]);
// }

// function getOperatorName(arg: ParseExpr): string {
//     if (arg.tag !== "EVar") {
//         throw new Error()
//     }
//     let name = arg.name;
//     return name;
// }

// function getOperator(operatorTable: OperatorTable, operatorName: string) {
//     if (operatorName in operatorTable) {
//         return operatorTable[operatorName]
//     }
//     else {
//         throw new Error(`unknown operator ${operatorName}`)
//         // return new Operator(operatorName, "infix", "none", -1)
//     }
// }

// function isHigherPrec(operatorTable: OperatorTable, arg1: ParseExpr, arg2: ParseExpr): boolean {
//     let op1 = getOperator(operatorTable, getOperatorName(arg1));
//     let op2 = getOperator(operatorTable, getOperatorName(arg2));

//     if (op1.precedence > op2.precedence)
//         return true;
//     if (op1.precedence < op2.precedence)
//         return false;

//     if (op1.assoc === "none")
//         throw new Error(`operator does not associate: ${op1.name}`);
//     if (op2.assoc === "none")
//         throw new Error(`operator does not associate: ${op2.name}`);
//     if (op1.assoc !== op2.assoc)
//         throw new Error(`operators associativity inconsistent: ${op1.name} ${op2.name}`);

//     return op1.assoc === "left";
// }

// export function foldOpArgs(operatorTable: OperatorTable, opArgs: ParseExpr[]): ParseExpr {
//     let expr = opArgs[0]
//     //let opStack: EVar<{}>[] = []
//     let opStack: ParseExpr[] = []
//     let argStack: ParseExpr[] = []
//     let prevArgWasAnOp = true;
//     for (let opArg of opArgs) {

//         if (opArg.tag === "EVar" && isOp(opArg)) {
//             if (prevArgWasAnOp)
//                 throw new Error(`unexpected infix operator ${JSON.stringify(opArg)}`);
//             while (opStack.length !== 0 && isHigherPrec(operatorTable, opStack[opStack.length - 1], opArg)) {
//                 let opExpr: ParseExpr = opStack.pop()!;
//                 let arg2 = argStack.pop()!;
//                 let arg1 = argStack.pop()!;
//                 argStack.push({ tag: "EApply", func: { tag: "EApply", func: opExpr, arg: arg1, loc: token.mergeLoc(opExpr.loc, arg1.loc), op: "" }, arg: arg2, loc: token.mergeLoc(opExpr.loc, arg2.loc), op: "" });
//             }
//             opStack.push(opArg);
//             prevArgWasAnOp = true;
//         }
//         else {
//             if (prevArgWasAnOp) {
//                 argStack.push(opArg);
//             }
//             else {
//                 let func = argStack.pop()!;
//                 argStack.push({ tag: "EApply", func: func, arg: opArg, loc: token.mergeLoc(func.loc, opArg.loc), op: "" });
//             }
//             prevArgWasAnOp = false;
//         }
//     }

//     while (opStack.length !== 0) {
//         let opExpr = opStack.pop()!;
//         if (argStack.length < 2)
//             throw new Error(`missing arguments for operator: ${(opExpr as any).name}`);
//         let arg2 = argStack.pop()!;
//         let arg1 = argStack.pop()!;
//         argStack.push({ tag: "EApply", func: { tag: "EApply", func: opExpr, arg: arg1, loc: token.mergeLoc(opExpr.loc, arg1.loc), op: "" }, arg: arg2, loc: token.mergeLoc(opExpr.loc, arg2.loc), op: "" });
//     }

//     return argStack[0];

// }

// function parseDecls(ps: ParseState): ParseDecl[] {
//     let decls: ParseDecl[] = []
//     do {
//         if (ps.tokenMatch(0, "separator", "}") || ps.tokenMatchTag(0, "eof")) {
//             return decls
//         }
//         let pattern = parseTypeAnnot(ps)
//         logger.log("parse", 1, "DeclPat", pattern)
//         parseToken(ps, "keysym", "=")
//         let defn = parseTypeAnnot(ps)
//         decls.push([pattern, defn])
//     } while (tryParseToken(ps, "separator", ";"))
//     return decls
// }

// function parseCaseAlts(ps: ParseState): ParseAlt[] {
//     let alts: ParseAlt[] = []
//     do {
//         if (ps.tokenMatch(0, "separator", "}") || ps.tokenMatchTag(0, "eof")) {
//             return alts
//         }
//         let pattern = parseExpr(ps)
//         logger.log("parse", 1, "DeclPat", pattern)
//         parseToken(ps, "keysym", "->")
//         let defn = parseExpr(ps)
//         alts.push([pattern, defn])
//     } while (tryParseToken(ps, "separator", ";"))
//     return alts
// }

// export { ParseState, parseExpr, parseDecls }
