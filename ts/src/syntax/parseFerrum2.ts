import { unit } from "../utils/unit.js"
import { assert } from "../utils/assert.js"
import { Loc, Token, showLoc, locMerge, showPos, locMatch } from "../syntax/token.js"
import {
    ExprLoc, eLambda, eLambdaMaybe, eLambdaNo, eLambdaYes, eLambdaOption,
    eAs, eApply, eVar, ePrim, eTypeAnnot, eTermBrackets, eTypeBrackets, DeclLoc,
    eList, DeclTypeBidir, showExp, eTypeAs,
    Decl,
    Expr,
    exprTransform,
    exprTransform2PairList,
    TorT
} from "../syntax/expr.js"
import { ParseState } from "../syntax/parse.js"
import { lookupOpDefn } from "./operator.js"

// type ParseState = { input: Token[], pos: number }
// type ParseState = parse.ParseState

function psEof(ps: ParseState): boolean {
    return ps.pos >= ps.input.length
}

function psPeek(ps: ParseState): Token {
    if (psEof(ps)) {
        throw new Error("past end of input")
    }
    let token = ps.input[ps.pos]
    return token
}

function psTake(ps: ParseState): Token {
    let tok = psPeek(ps)
    ps.pos++
    return tok
}

function parseToken(ps: ParseState, tag: string, val: any, seeAlsoToken: Token | null): unit {
    let tok = psTake(ps)
    if (tok.tag !== tag || tok.value !== val) {
        let seeAlsoMsg = ""
        if (seeAlsoToken !== null) {
            // seeAlsoMsg = `, see also ${JSON.stringify(seeAlsoToken)}`
            seeAlsoMsg = `, see also (${JSON.stringify(seeAlsoToken.value)}) (${showLoc(seeAlsoToken.loc)})`
            // seeAlsoMsg = `, see also (${JSON.stringify(seeAlsoToken.value)}) (${showPos(seeAlsoToken.loc.range.start)})`
        }
        throw new Error(`unexpected token at (${showLoc(tok.loc)}), got (${tok.tag} ${JSON.stringify(tok.value)}), expected (${tag} ${JSON.stringify(val)}) ${seeAlsoMsg}`)
    }
}

function tryParseToken(ps: ParseState, tag: string, val: any): boolean {
    let tok = psPeek(ps)
    if (tok.tag !== tag || tok.value !== val) {
        return false
    }
    else {
        ps.pos++
        return true
    }
}


type Fixity = "Prefix" | "Infix" | "Postfix"
type OpDefn = { name: string, fixity: Fixity }
type OpCompare = "Left" | "Right" | "None"

function compareOperators(l: OpDefn, r: OpDefn): OpCompare {
    // TODO sensible precedence comparison / lookup operator definitions 
    if (r.fixity !== "Infix") {
        throw new Error(`expected right operator (${r}) to be infix`)
    }
    switch (l.fixity) {
        case "Prefix":
            if (l.name === "->") {
                return "Right"
            }
            return "None"
        case "Infix":
            if (l.name === "@") {
                return "Left"
            }
            if (r.name === "@" || r.name === "->" || r.name === "|->" || r.name === "|=>" || r.name === "<|" || r.name === "::") {
                return "Right"
            }
            if (l.name === "->" || l.name === "|->" || l.name === "|=>" || l.name === "<|") {
                return "Right"
            }
            if (l.name === r.name) {
                switch (l.name) {
                    case "<|":
                    case "::":
                        return "Right"
                    default:
                }        return "Left"
            }
            return "None"
        default:
            throw new Error("missing case")
    }
}

// TODO define the known operators

function lookupOp(name: string): OpDefn {
    // TODO lookup up op in a list of valid ops
    let opDefn: OpDefn = { name: name, fixity: "Infix" }
    return opDefn
}

function lookupPrefixOp(name: string): OpDefn {
    // TODO lookup up op in a list of valid ops
    let opDefn: OpDefn = { name: name, fixity: "Prefix" }
    return opDefn
}


// const USE_EOPS = false
const USE_EOPS = true

function mkApplyOp(tort: TorT, name: string, loc: Loc, arg1: ExprLoc, arg2: ExprLoc): ExprLoc {
    let show = (a: any) => JSON.stringify(a)
    if (arg1 === undefined) {
        throw new Error(`mkApplyOp, undefined arg1, ${show(name)}, ${show(loc)}`)
    }
    let loc2 = locMerge(arg1.loc, arg2.loc)
    const locA = { loc: loc2 }
    switch (name) {
        case "->":
            return eLambda(locA, arg1, arg2)
        case "=>": // lambda-yes
            return eLambdaYes(locA, arg1, arg2)
        case "|->": // |-> lambda-no
            return eLambdaNo(locA, arg1, arg2)
        case "|=>": // |=> lambda-maybe
            return eLambdaMaybe(locA, arg1, arg2)
        case ":":
            // if (arg2.tag === "EAs") {
            //     return eTypeAs(loc2, arg1, arg2.name, arg2.expr)
            // }
            return eTypeAnnot(locA, arg1, arg2)
        case "@": {
            if (arg1.tag === "EVar") {
                return eAs(locA, arg1.name, arg2)
            }
            else {
                throw new Error(`expected identifier to left of @ as-symbol at (${showLoc(loc)})`)
            }
        }
        case "|>":
            return eApply(locA, arg2, arg1, "|>")
        case "<|":
            return eApply(locA, arg1, arg2, "<|")
        // TODO remove this (::), the double-comma in tuple-brackets is better
        case "::": {
            assert.impossible("Invalid syntax")
            let loc = locMerge(arg1.loc, arg2.loc)
            // if (arg2.tag==="EList") {
            //     return eList(loc, [arg1, ...arg2.exprs], arg2.tail)
            // }
            // else 
            {
                return eList(locA, [arg1], arg2)
            }
        }
        default: {
            if (USE_EOPS) {
                let loc3 = { loc: locMerge(arg1.loc, arg2.loc) }
                const nameC = name
                const op = lookupOpDefn(nameC, "Infix", tort)
                assert.isTrue(op !== null)
                const nameA = op.nameA
                let arg3 = ePrim(loc3, nameA, [arg1, arg2])
                return arg3
            }
            else {
                let loc3 = locMerge(loc, arg1.loc)
                let arg3 = eApply({ loc: loc2 }, eApply({ loc: loc3 }, eVar({ loc }, name), arg1), arg2)
                return arg3
            }
        }
    }
}
function mkApplyOpPrefix(name: string, loc: Loc, arg1: ExprLoc): ExprLoc {
    switch (name) {
        case "->": {
            // let unitArg: Expr = { tag: "EDatum", value: null, loc: loc }
            let unitArg: ExprLoc = { tag: "EList", exprs: [], tail: null, loc: loc }
            const loc2 = locMerge(loc, arg1.loc)
            return eLambda({ loc: loc2 }, unitArg, arg1)
        }
        default: {
            assert.impossible("Do we ever get here?")
            if (USE_EOPS) {
                let loc3 = locMerge(loc, arg1.loc)
                let arg3 = ePrim({ loc: loc3 }, name, [arg1])
                return arg3
            }
            else {
                let arg3 = eApply({ loc: loc }, eVar({ loc: loc }, name), arg1)
                return arg3
            }
        }
    }
}

function mkApplyOpPostfix(name: string, loc: Loc, arg1: ExprLoc): ExprLoc {
    assert.impossible("Do we ever get here?")
    switch (name) {
        default: {
            if (USE_EOPS) {
                let loc2 = locMerge(loc, arg1.loc)
                let arg2 = ePrim({ loc: loc2 }, name, [arg1])
                return arg2
            }
            else {
                let arg2 = eApply({ loc: arg1.loc }, eVar({ loc }, name), arg1)
                return arg2
            }
        }
    }
}

function foldOpArgs2(tort: TorT, opStack: [ExprLoc, OpDefn][], argStack: ExprLoc[], opDefn: OpDefn | null, juxtaposedApplyPossible: boolean): unit {
    let precedence = "Left"
    while (opStack.length !== 0 && precedence === "Left") {
        let [prevOpExpr, prevOpDefn] = opStack[opStack.length - 1]
        precedence = opDefn === null ? "Left" : compareOperators(prevOpDefn, opDefn)
        // if (juxtaposedApplyPossible) {
        // }
        // else {
        //     precedence = "Left"
        // }
        switch (precedence) {
            case "Left": {
                switch (prevOpDefn.fixity) {
                    case "Prefix": {
                        let arg1 = argStack.pop()!
                        opStack.pop()
                        let arg3 = mkApplyOpPrefix(prevOpDefn.name, prevOpExpr.loc, arg1)
                        argStack.push(arg3)
                        break
                    }
                    case "Infix": {
                        let arg2 = argStack.pop()!
                        let arg1 = argStack.pop()!
                        opStack.pop()
                        let arg3 = mkApplyOp(tort, prevOpDefn.name, prevOpExpr.loc, arg1, arg2)
                        argStack.push(arg3)
                        break
                    }
                    case "Postfix":
                        throw new Error("impossible")
                    default:
                        throw new Error("missing case")
                }
            }
            case "Right":
                break
            case "None":
                throw new Error(`no precedence relationship exists between operators (${prevOpDefn.name}) and (${opDefn!.name}) at ${showLoc(prevOpExpr.loc)}`)
            default:
                throw new Error("missing case")
        }
    }
}

function foldOpArgs(tort: TorT, opArgs: ExprLoc[]): ExprLoc {
    let opStack: [ExprLoc, OpDefn][] = []
    let argStack: ExprLoc[] = []
    let juxtaposedApplyPossible = false
    opArgs.forEach(opArg => {
        if (opArg.tag === "ESym") {
            // if (argStack.length === 0) {
            if (!juxtaposedApplyPossible) {
                let opDefn = lookupPrefixOp(opArg.name)
                opStack.push([opArg, opDefn])
            }
            else {
                let opDefn = lookupOp(opArg.name)
                switch (opDefn.fixity) {
                    case "Prefix":
                        throw new Error("impossible")
                    case "Infix": {
                        foldOpArgs2(tort, opStack, argStack, opDefn, juxtaposedApplyPossible)
                        opStack.push([opArg, opDefn])
                        juxtaposedApplyPossible = false
                        break
                    }
                    case "Postfix": {
                        if (argStack.length === 0) {
                            throw new Error(`postfix operator (${opDefn.name}) not permitted here (${showLoc(opArg.loc)})`)
                        }
                        let arg1 = argStack.pop()!
                        let arg2 = mkApplyOpPostfix(opDefn.name, opArg.loc, arg1)
                        argStack.push(arg2)
                        break
                    }
                    default:
                        throw new Error("missing case")

                }
            }
        }
        else { // opArg.tag !== "ESym"
            if (juxtaposedApplyPossible) {
                let arg1 = argStack.pop()!
                let loc = locMerge(arg1.loc, opArg.loc)
                let arg2: ExprLoc = { tag: "EApply", loc: loc, func: arg1, arg: opArg, op: "" }
                argStack.push(arg2)
            }
            else {
                argStack.push(opArg)
                juxtaposedApplyPossible = true
            }
        }
    })

    foldOpArgs2(tort, opStack, argStack, null, juxtaposedApplyPossible)

    if (argStack.length === 1 && opStack.length === 0) {
        let result = argStack.pop()!
        return result
    }
    else {
        opArgs.forEach(opArg => {
            if (opArg.tag === "ESym") {
                console.log("OpArg", opArg.name)
            }
            else {
                console.log("OpArg", opArg.tag)
            }
        })
        let showOpArgs = JSON.stringify(opArgs.map(oa => oa.tag === "ESym" ? oa.name : oa.tag))
        throw new Error(`failed to fold op-args (${showLoc(opArgs[0].loc)}) (${showOpArgs})`)
    }
}

function parseExpr(ps: ParseState, seeAlsoToken: Token | null = null): ExprLoc {
    const allowLet = true
    let expr = tryParseExprPart(ps, allowLet)

    let seeAlsoMsg = ""
    if (seeAlsoToken !== null) {
        // seeAlsoMsg = `, see also ${JSON.stringify(seeAlsoToken)}`
        seeAlsoMsg = `, see also (${JSON.stringify(seeAlsoToken.value)}) (${showLoc(seeAlsoToken.loc)})`
        // seeAlsoMsg = `, see also (${JSON.stringify(seeAlsoToken.value)}) (${showPos(seeAlsoToken.loc.range.start)})`
    }

    if (expr === null) {
        const tok = psPeek(ps)
        throw new Error(`failed to parse expression at (${showLoc(tok.loc)}) got (${tok.tag} ${JSON.stringify(tok.value)}) ${seeAlsoMsg}`)
    }

    const opArgs = []
    do {
        opArgs.push(expr)
        const allowLet = expr.tag === "ESym"
        expr = tryParseExprPart(ps, allowLet)
    }
    while (expr !== null)

    const tort = ps.peekTorT()
    const result = foldOpArgs(tort, opArgs)
    return result
}

function tryParseExprPart(ps: ParseState, allowLet: boolean): ExprLoc | null {
    if (psEof(ps)) {
        return null
    }
    let tok = psPeek(ps)
    switch (tok.tag) {
        case "integer":
        case "string":
            psTake(ps)
            return { tag: "EDatum", value: tok.value, loc: tok.loc }
        case "ident": {
            psTake(ps)
            let expr1: ExprLoc = { tag: "EVar", name: tok.value, loc: tok.loc }
            return expr1
        }
        case "keyword": {
            switch (tok.value) {
                case "let": {
                    if (!allowLet) {
                        return null
                    }
                    let decls: DeclLoc[] = []
                    let loc1 = ps.srcLoc()
                    while (tryParseToken(ps, "keyword", "let")) {
                        let letToken = ps.prev()
                        let pat = parseExpr(ps, tok)
                        parseToken(ps, "keysym", "=", letToken)
                        let defn = parseExpr(ps, tok)
                        parseToken(ps, "separator", ";", letToken)
                        decls.push([pat, defn])
                    }
                    let body = parseExpr(ps, tok)
                    let loc2 = ps.srcLoc()
                    return { tag: "ELet", decls: decls, expr: body, loc: locMerge(loc1, loc2) }
                }
                default:
                    throw new Error("missing case")
            }
        }
        case "separator":
            switch (tok.value) {
                case "(": {
                    psTake(ps)
                    ps.pushTorT("Term")
                    let loc1 = ps.srcLoc2()
                    let expr = parseExpr(ps)
                    parseToken(ps, "separator", ")", tok)
                    ps.popTorT("Term")
                    let loc2 = ps.srcLoc2()
                    return eTermBrackets({ loc: locMerge(loc1, loc2) }, expr)
                }
                case "{": {
                    psTake(ps)
                    ps.pushTorT("Type")
                    let loc1 = ps.srcLoc2()
                    let expr = parseExpr(ps)
                    parseToken(ps, "separator", "}", tok)
                    ps.popTorT("Type")
                    let loc2 = ps.srcLoc2()
                    let typeBrackets = eTypeBrackets({ loc: locMerge(loc1, loc2) }, expr)
                    return typeBrackets
                }
                case "[": {
                    psTake(ps)
                    let elems: ExprLoc[] = []
                    let tail: ExprLoc | null = null
                    let loc1 = ps.srcLoc2()
                    if (tryParseToken(ps, "separator", "]")) {
                        let loc2 = ps.srcLoc2()
                        return { tag: "EList", exprs: [], tail: null, loc: locMerge(loc1, loc2) }
                    }
                    while (true) {
                        let expr = parseExpr(ps)
                        elems.push(expr)
                        if (tryParseToken(ps, "separator", "]")) {
                            let loc2 = ps.srcLoc2()
                            return { tag: "EList", exprs: elems, tail: tail, loc: locMerge(loc1, loc2) }
                        }
                        else if (tryParseToken(ps, "separator", ",,")) {
                            let tail = parseExpr(ps)
                            parseToken(ps, "separator", "]", tok)
                            let loc2 = ps.srcLoc2()
                            return { tag: "EList", exprs: elems, tail: tail, loc: locMerge(loc1, loc2) }
                        }
                        else {
                            parseToken(ps, "separator", ",", tok)
                            if (tryParseToken(ps, "keysym", "...")) {
                                let tail = parseExpr(ps)
                                parseToken(ps, "separator", "]", tok)
                                let loc2 = ps.srcLoc2()
                                return { tag: "EList", exprs: elems, tail: tail, loc: locMerge(loc1, loc2) }
                            }
                        }
                    }
                }
                default:
                    return null
            }
        case "keysym":
            return null
        case "symbol": {
            psTake(ps)
            let expr1: ExprLoc = { tag: "ESym", name: tok.value, loc: tok.loc }
            return expr1
        }
        case "eof":
            return null
        default:
            throw new Error(`missing case (${tok.tag})`)
    }
}

function parseDecls(ps: ParseState): DeclLoc[] {
    let decls: DeclLoc[] = []
    while (tryParseToken(ps, "keyword", "let")) {
        let letToken = ps.prev()
        let pat = parseExpr(ps)
        parseToken(ps, "keysym", "=", letToken)
        let defn = parseExpr(ps)
        parseToken(ps, "separator", ";", letToken)
        decls.push([pat, defn])
    }
    return decls
}

function convertTokens(tokens: Token[]) {
    let fakeOperators = ["->", "=>", "@", ":", "|->", "|=>"]
    tokens.forEach(tok => {
        if (tok.tag === "keysym" && fakeOperators.indexOf(tok.value) !== -1) {
            tok.tag = "symbol"
        }
    })
}

// const transformTypeBrackets = (exp: Expr): Expr => {
//     if (exp.tag === "ETypeBrackets") {
//         const exp2 = convertTypeBrackets(exp.loc, exp.expr, false)
//         // console.log(showExp(exp2))
//         return exp2
//     }
//     else {
//         return exp
//     }
// }

// const convertTypeBracketsExpr = (exp: Expr): Expr => {
//     const exp2 = exprTransform(exp, e => transformTypeBrackets(e))
//     return exp2
// }

// const convertTypeBracketsDecls = (decls: Decl[]): Decl[] => {
//     let decls2: Decl[] = []
//     for (const [pat, defn] of decls) {
//         const pat2 = exprTransform(pat, e => transformTypeBrackets(e))
//         const defn2 = exprTransform(defn, e => transformTypeBrackets(e))
//         decls2.push([pat2, defn2])
//     }
//     return decls2
// }

export function parseFile(ps: ParseState, language: string | null): DeclLoc[] {
    switch (language) {
        case "Ferrum/0.1":
        case "ferrum/0.1": {
            convertTokens(ps.input)
            ps.pushTorT("Term")
            let decls = parseDecls(ps)
            ps.popTorT("Term")
            decls = stripNonSemanticBrackets_decls(decls)
            if (!tryParseToken(ps, "eof", null)) {
                // if (!ps.eof()) {
                // throw new Error(`failed to parse to end of file, reached ${showLoc(ps.srcLoc())}`)
                throw new Error(`failed to parse to end of file, reached ${showLoc(psPeek(ps).loc)}`)
            }
            return decls
        }
        default:
            throw new Error(`unknown language ${language}`)
    }
}


// export 
function parseExp(ps: ParseState, language: string | null, tort: TorT): ExprLoc {
    switch (language) {
        case "Ferrum/0.1":
        case "ferrum/0.1":
        case "ferrum/test/0.1":
        case "ferrum/proj/0.1": {
            convertTokens(ps.input)
            ps.pushTorT(tort)
            let exp = parseExpr(ps)
            ps.popTorT(tort)
            if (tort) {
                exp = stripNonSemanticBrackets_expr(exp, tort)
            }
            return exp
        }
        default:
            throw new Error(`unknown language ${language}`)
    }
}

export const parseTerm = (ps: ParseState, language: string | null) => {
    const result = parseExp(ps, language, "Term")
    return result
}
export const parseType = (ps: ParseState, language: string | null) => parseExp(ps, language, "Type")


export function stripNonSemanticBrackets_expr<T>(expr: Expr<T>, tort: TorT = "Term"): Expr<T> {

    // return expr

    switch (tort) {
        case "Term":
            return stripTerm(expr)
        case "Type":
            return stripType(expr)
    }

    function stripTerm(expr: Expr<T>): Expr<T> {
        switch (expr.tag) {
            case "ETermBrackets":
                return stripTerm(expr.expr)
            case "ETypeBrackets":
                return exprTransform(expr, { expr: stripType })
            default:
                return exprTransform(expr, { expr: stripTerm })
        }
    }

    function stripType(expr: Expr<T>): Expr<T> {
        switch (expr.tag) {
            case "ETermBrackets":
                return exprTransform(expr, { expr: stripTerm })
            case "ETypeBrackets":
                return stripType(expr.expr)
            default:
                return exprTransform(expr, { expr: stripType })
        }
    }

}

// If let-bindings are permitted within type-brackets,
//   then this function will need to take and pass on a TorT argument too.
export function stripNonSemanticBrackets_decls<T>(decls: Decl<T>[]): Decl<T>[] {
    return exprTransform2PairList(decls, { expr: stripNonSemanticBrackets_expr })
}

