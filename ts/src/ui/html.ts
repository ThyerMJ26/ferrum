import { unit } from "../utils/unit.js"
import { assert } from "../utils/assert.js"
import { UiText } from "./text.js";
import { mkUiStyle, PageStyles } from "../site/page-html.js";


type HtmlTag = keyof HTMLElementTagNameMap

export type Html = [HtmlTag, { [_: string]: string; }, ...(Html | string)[]];

type TagType = "void" | "raw" | "escape" | "normal"

const tagTypes = {
    area: "void",
    base: "void",
    br: "void",
    col: "void",
    embed: "void",
    hr: "void",
    img: "void",
    input: "void",
    link: "void",
    meta: "void",
    source: "void",
    track: "void",
    wbr: "void",
    script: "raw",
    style: "raw",
    textarea: "escape",
    title: "escape",
} as const satisfies { [_ in HtmlTag]?: TagType }

type TagTypes = typeof tagTypes

type TagOf<T extends TagType> = keyof ({ [k in keyof TagTypes as TagTypes[k] extends T ? k : never]: T })

type VoidTag = TagOf<"void">
type RawTag = TagOf<"raw">
type EscapeTag = TagOf<"escape">
type NormalTag = Exclude<HtmlTag, keyof TagTypes>

function typeOfTag(tag: HtmlTag): TagType {
    if (tag in tagTypes) {
        return tagTypes[tag as keyof TagTypes]
    }
    return "normal"
}

export type HtmlAttrs = { [_: string]: string; }

export type Html2 =
    | [VoidTag, HtmlAttrs]
    | [RawTag, HtmlAttrs, string]
    | [EscapeTag, HtmlAttrs, string]
    | [NormalTag, HtmlAttrs, ...(Html | string)[]];


export function escapeText(text: string, tt?: TagType): string {
    switch (tt) {
        case "void":
            assert.impossible()
        case "raw":
            // This is overly cautious, but should be fine in practice.
            assert.isTrue(text.search("</") === -1)
            return text
        case "escape":
        case "normal":
        case undefined:
            return text.replace(/[<>&]/g, (c) => {
                const c2 = {
                    "<": "&lt;",
                    ">": "&gt;",
                    "&": "&amp;",
                }[c]
                assert.isTrue(c2 !== undefined)
                return c2
            })
        default:
            assert.noMissingCases(tt)
    }
}

function escapeAttrs(attrs: HtmlAttrs): string {
    const aParts: string[] = []
    for (const [k, val] of Object.entries(attrs)) {
        const val2 = val.replace(/[\"\\\n]/g, v => {
            const v2 = {
                "\"": "&quot;",
                "\\": "&Backslash;",
                "\n": "&NewLine;",
            }[v]
            assert.isTrue(v2 !== undefined)
            return v2
        })
        aParts.push(` ${k}="${val2}"`)
    }
    const aTxt = aParts.join("")
    return aTxt
}

export function htmlRender(html: Html): string[] {
    const lines: string[] = []
    function hr(html: Html): unit {
        const [tag, attrs, ...contents] = html
        const aTxt = escapeAttrs(attrs)
        // const tt = tagTypes[tag]
        const tt = typeOfTag(tag)
        switch (tt) {
            case "void":
                assert.isTrue(contents.length === 0)
                lines.push(`<${tag}${aTxt}>`)
                break
            case "raw":
            case "escape":
            case "normal":
            case undefined:
                lines.push(`<${tag}${aTxt}>`)
                for (const content of contents) {
                    switch (typeof content) {
                        case "string":
                            lines.push(escapeText(content, tt))
                            break
                        case "object":
                            hr(content)
                            break
                        default:
                            assert.noMissingCases(content)
                    }
                }
                lines.push(`</${tag}>`)
                break
            default:
                assert.noMissingCases(tt)
        }
    }

    hr(html)

    return lines
}

export type HtmlBuilder = {
    text(text: UiText): unit
    // TODO ? Take an attributes argument ?
    elem(tag: HtmlTag, cb: () => unit): unit
    elemA(tag: HtmlTag, attrs: HtmlAttrs, cb: () => unit): unit
    // TODO ? Allow a chunk of HTML to be added in one go ?
    // html(h: Html): unit
}

export function htmlBuild(output: string[], styles: PageStyles, cb: (hb: HtmlBuilder) => unit): string[] {

    // const lines: string[] = []
    const lines: string[] = output ?? []

    function textToHtml(text: UiText): unit {
        if (typeof text === "string") {
            lines.push(escapeText(text))
        }
        else {
            if ("items" in text && text.items !== undefined) {
                if ("style" in text) {
                    assert.isTrue(typeof text.style === "object", "TODO Handle UiStyleNum numbers")
                    const ps = mkUiStyle(styles, text.style, null)
                    lines.push(`<span class=${ps.cssName}>`)
                    assert.isTrue(text.items instanceof Array)
                }
                for (const item of text.items) {
                    textToHtml(item)
                }
                if ("style" in text) {
                    lines.push("</span>")
                }
            }
        }

    }

    function elemToHtml(tag: string, attrs: HtmlAttrs, cb: () => unit): unit {

    }

    cb({
        text(text) {
            textToHtml(text)
        },
        elem(tag, cb) {
            lines.push(`<${tag}>`)
            cb()
            lines.push(`</${tag}>`)
        },
        elemA(tag, attrs, cb) {
            lines.push(`<${tag}${escapeAttrs(attrs)}>`)
            cb()
            lines.push(`</${tag}>`)
        },
    })


    return lines

}