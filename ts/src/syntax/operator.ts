
// TODO Define operator syntax and precedence.

import { unit } from "../utils/unit.js";
import { TorT } from "./expr.js";

// These are the operators that mean (almost) the same in Ferrum and JS/C.
//   ( The "&&"" and "||"" operators are sometimes unconventionally strict in Ferrum and will be replaced with "and" and "or". )
// This doesn't have anything to do with the primitives defined above.
// This should be moved into the syntax or codegen directory.

// TODO Switch to a better way of using operators directly.
// TODO The new (+)-like names break the current mechanism.
// export function isPrimOp(name: string) {
//     let infixOps = ["+", "-", "*", "==", ">", ">=", "<", "<=", "&&", "||"];
//     return infixOps.indexOf(name) !== -1;
// }

export function concreteJsOpName(name: string): null | string {
    switch (name) {
        case "(+)": return "+"
        case "(-)": return "-"
        case "(*)": return "*"
        case "(==)": return "=="
        case "(>)": return ">"
        case "(>=)": return ">="
        case "(<)": return "<"
        case "(<=)": return "<="
        case "(&&)": return "&&"
        case "(||)": return "||"
        default:
            return null
    }
}

type Fixity = "Prefix" | "Infix" | "Postfix"

type OpDefn = {
    nameA: string // Abstract name
    nameC: string // Concrete name
    tort: TorT
    fixity: Fixity
}

// const operators: OpDefn[] = []
// function tmInOp(name: string): unit {
//     operators.push({nameA: '', nameC: name, tort: "Term", fixity: "Infix"})
// }
// tmInOp("+")

function tmOpIn(name: string): OpDefn {
    return { nameA: `(${name})`, nameC: name, tort: "Term", fixity: "Infix" }
}

function tmOpPre(name: string): OpDefn {
    return { nameA: `(${name}_)`, nameC: name, tort: "Term", fixity: "Prefix" }
}

function tmOpPost(name: string): OpDefn {
    return { nameA: `(_${name})`, nameC: name, tort: "Term", fixity: "Postfix" }
}

function tyOpIn(name: string): OpDefn {
    return { nameA: `{${name}}`, nameC: name, tort: "Type", fixity: "Infix" }
}

function tyOpPre(name: string): OpDefn {
    return { nameA: `{${name}_}`, nameC: name, tort: "Type", fixity: "Prefix" }
}

function tyOpPost(name: string): OpDefn {
    return { nameA: `{_${name}}`, nameC: name, tort: "Type", fixity: "Postfix" }
}

const operators: OpDefn[] = [
    tmOpIn("+"),
    tmOpIn("-"),
    tmOpIn("*"),
    tmOpIn("=="),
    tmOpIn(">"),
    tmOpIn(">="),
    tmOpIn("<"),
    tmOpIn("<="),
    tmOpIn("&&"),
    tmOpIn("||"),

    tmOpIn("<$"),

    tmOpIn("|-"),
    tmOpIn("|="),

    tyOpIn("&"),
    tyOpIn("|"),
    tyOpIn("\\"),
]



export function lookupOpDefn(nameC: string, fixity: Fixity, tort: TorT): null | OpDefn {
    for (const op of operators) {
        if (op.nameC === nameC && op.fixity === fixity && op.tort === tort) {
            return op
        }
    }
    return null
}


