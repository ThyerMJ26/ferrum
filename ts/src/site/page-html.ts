import * as path from "node:path"

import { assert } from "../utils/assert.js"
import { unit } from "../utils/unit.js"
import { Doc, Page } from "./page-doc.js"
import { AppDefns, AppInstances, FileMap, PageMap } from "./website-builder.js"
import { UiColor, UiStyle, UiText, uiTextToStr } from "../ui/text.js"
import { escapeText, htmlBuild } from "../ui/html.js"


export function docToHtml2(ctx: PageHtmlCtx, styles: PageStyles, lines: string[], doc: Doc): unit {

    function mod(module: string): string {
        // const srcFile = path.join(ctx.topDir, module)
        const srcFile = module
        const fileEntry = ctx.fileMap.get(srcFile)
        if (fileEntry !== undefined) {
            return JSON.stringify(fileEntry[0].urlPath)
        }
        else {
            assert.unreachable()
        }
    }

    htmlBuild(lines, styles, (b) => {

        dth(doc)

        function dth(doc: Doc): unit {

            if (typeof doc === "string") {
                lines.push(escapeText(doc))
                return
            }
            if (doc instanceof Array) {
                for (const d of doc) {
                    dth(d)
                }
                return
            }
            if ("tag" in doc) {
                switch (doc.tag) {
                    case "para":
                        b.elem("p", () => {
                            dth(doc.para)
                        })
                        break
                    case "list":
                        b.elem("ul", () => {
                            for (const item of doc.items) {
                                b.elem("li", () => {
                                    dth(item)
                                })
                            }
                        })

                        break
                    case "defns":
                        b.elem("dl", () => {
                            for (const defn of doc.defns) {
                                b.elem("dt", () => {
                                    dth(defn.term)
                                })
                                b.elem("dd", () => {
                                    dth(defn.defn)
                                })
                            }
                        })


                        break
                    case "title":
                        assert.impossible()
                    case "app-publish":

                        ctx.numApps++
                        const topDivNum = ctx.numApps
                        // TODO ? Include the instance name in the topDivId ?
                        const topDivId = `topDiv-${topDivNum}`

                        const appInstance = ctx.appInstances.get(doc.instance)
                        assert.isTrue(appInstance !== undefined)
                        const appDefn = ctx.appDefns.get(appInstance.defnName)
                        assert.isTrue(appDefn !== undefined)

                        const q = JSON.stringify

                        const h = [
                            `<div style="width:90vw;height:50vh;"><div id=${topDivId}></div></div>`,
                            "<script type='module'>",
                            `  import { initApp2 } from ${mod("/ts/gen/ui/browser.js")}`,
                            `  import { ${appDefn.fnName} as app } from ${mod(appDefn.fileName)}`,
                            `  initApp2(${q(topDivId)}, ${q(doc.instance)}, app, ${q(appInstance.args)})`,
                            "</script>",
                        ]

                        lines.push(...h)
                        break

                    case "link-page": {
                        const srcFile = path.resolve(path.dirname(ctx.srcFile), doc.page)

                        const entry = ctx.pageMap.get(srcFile)
                        if (entry !== undefined) {
                            const href = entry[0].urlPath
                            b.elemA("a", { href }, () => {
                                const text = doc.text ?? href
                                // TODO ? Use the title from the page itself ?
                                // TODO ? We'll need to generate (or have generated) each page linked to in order to render this page.
                                // TODO ? Generating precedes rendering, so this wouldn't be potentially cyclic.
                                // const text = doc.text ?? title
                                b.text(text)
                            })
                        }
                        else {
                            b.text(`(Bad Link: ${JSON.stringify(doc)})`)
                        }
                        break
                    }
                    case "link-file":
                        const srcFile = path.resolve(path.dirname(ctx.srcFile), doc.file)

                        const entry = ctx.fileMap.get(srcFile)
                        if (entry !== undefined) {
                            const href = entry[0].urlPath
                            b.elemA("a", { href }, () => {
                                const text = doc.text ?? href
                                b.text(text)
                            })
                        }
                        else {
                            b.text(`(Bad Link: ${JSON.stringify(doc)})`)
                        }
                        break
                        break
                    case "link-url":
                            const href = doc.url
                            b.elemA("a", { href }, () => {
                                b.text(doc.text ?? href)
                            })
                        break
                    default:
                        assert.noMissingCases(doc)
                }
                return
            }

            if ("style" in doc) {
                assert.isTrue(typeof doc.style === "object", "TODO Handle UiStyleNum numbers")
                const ps = mkUiStyle(styles, doc.style, null)
                lines.push(`<span class=${ps.cssName}>`)
                if ("items" in doc) {
                    assert.isTrue(doc.items instanceof Array)
                    for (const item of doc.items) {
                        dth(item)
                    }
                }
                lines.push("</span>")
            }


        }

    })

}



export type PageHtmlCtx = {
    srcFile: string // The file currently being rendered, needed for resolving relative page references.
    // TODO ? It might be simpler to include the whole of SiteFiles here, rather than repeat most of it anyway ?
    topDir: string,
    fileMap: FileMap,
    pageMap: PageMap,
    appDefns: AppDefns,
    appInstances: AppInstances
    numApps: number
}

export type PageHtmlOptions = {
    importReloadScript?: boolean
    importAppScripts?: boolean
}

type PageStyleKey = string
type PageStyle = {
    key: PageStyleKey
    uiStyle: UiStyle
    cssName: string
    cssDefn: string
}

export type PageStyles = {
    styleMap: Map<PageStyleKey, PageStyle>
}

// type PageStyles = Map<PageStyleKey, PageStyle>

// function pageStyle(styles: PageStyles, uiStyle: UiStyle): PageStyle {
//     const key = JSON.stringify(uiStyle)
//     let pageStyle = styles.styleMap.get(key)
//     if (pageStyle === undefined) {
//         const cssName = "TODO"
//         pageStyle = { key, uiStyle, cssName }
//     }
//     return pageStyle
// }

function styleEntries(uiStyle: UiStyle): [keyof UiStyle, string | number][] {
    return Object.entries(uiStyle) as [keyof UiStyle, string | number][]
}

// Ideally this would ensure all the colour names below are correct.
// Unfortuantely it evaluates to "string"
type CssColor = CSSStyleDeclaration["color"]

function bgColor(color: UiColor): CssColor {
    switch (color) {
        case "Black": return "lightgray"
        case "Red": return "pink"
        case "Green": return "lightgreen"
        case "Yellow": return "yellow"
        case "Blue": return "lightblue"
        case "Magenta": return "magenta"
        case "Cyan": return "lightcyan"
        case "White": return "white"
        default:
            assert.noMissingCases(color)
    }
}

function fgColor(color: UiColor): CssColor {
    switch (color) {
        case "Black": return "black"
        case "Red": return "red"
        case "Green": return "green"
        // case "Yellow": return "brown"
        case "Yellow": return "orange"
        // case "Yellow": return "#C09000"
        case "Blue": return "blue"
        case "Magenta": return "magenta"
        case "Cyan": return "cyan"
        case "White": return "darkgray"
        default:
            assert.noMissingCases(color)
    }
}

// TODO Share this with browser.ts.
// TODO It is (or was) a copy,
// TODO   it risks getting out of sync.
export function mkUiStyle(styles: PageStyles, s: UiStyle, nameHint: string | null): PageStyle {
    const key = JSON.stringify(s)

    if (styles.styleMap.has(key)) {
        return styles.styleMap.get(key)!
    }

    let style = ""

    // let family = "MyMono-regular"
    // let family = "sans-serif"
    let family = "monospace"
    // let family = "monospace, monospace"
    style += `font-size: 1rem;`

    // Using font-weight like this works: 
    //   -         with the default sans-serif font in Firefox and Epiphany
    //   - but not with the default sans-serif font in Chromium.
    // It seems there's no weight lighter than 400 available.
    // ( monospace seems to have a light-variant in Firefox, Chromium and Epiphany )
    // switch (s.weight) {
    //     case -1: style += "font-weight: 100;"; break
    //     case 0: style += "font-weight: 400;"; break
    //     case 1: style += "font-weight: 700;"; break
    // }
    // // Using an explicit font seems more reliable, but requires using external fonts
    // switch (s.weight) {
    //     case -1: family = "WuiFont-faint"; break
    //     case 0: family = "WuiFont-regular"; break
    //     case 1: family = "WuiFont-bold"; break
    // }
    // Alternatively, use opacity to achieve faintness (so as not to require external fonts)
    //   This also makes the strike-through appear faint, which looks better / more consistent.
    switch (s.weight) {
        case -1: style += "opacity: 0.5;"; break
        case 0: style += "font-weight: normal;"; break
        case 1: style += "font-weight: bold;"; break
    }

    if (family !== undefined) {
        style += `font-family: ${family};`
    }

    if (s.fg !== undefined) {
        style += `color: ${fgColor(s.fg)};`
    }
    if (s.bg !== undefined) {
        style += `background-color: ${bgColor(s.bg)};`
    }
    if (s.italic === 1) {
        style += "font-style: italic;"
    }
    let decor = []
    if (s.strike === 1) {
        decor.push("line-through")
    }
    if (s.under ?? 0 > 1) {
        decor.push("underline")
    }
    if (decor.length !== 0) {
        style += `text-decoration-line: ${decor.join(" ")}`
    }
    // if (ts.under === 2) {
    //     style += "text-decoration-style: double;"
    // }

    const num = styles.styleMap.size
    const nameHint2 = nameHint === null ? "" : `_${nameHint}`

    const cssName = `css${num}${nameHint2}`
    const cssDefn = style
    // const cssDefn = [
    //     `{ // ${key}`,
    //     `}`,
    // ].map(a => `  ${a}\n`).join("")


    const pageStyle: PageStyle = {
        uiStyle: s,
        key,
        cssName,
        cssDefn,
    }

    styles.styleMap.set(key, pageStyle)

    return pageStyle
}


export function pageToHtml(ctx: PageHtmlCtx, opts: PageHtmlOptions, page: Page): string[] {

    function mod(module: string): string {
        // const srcFile = path.join(ctx.topDir, module)
        // const fileEntry = ctx.srcFileMap.get(srcFile)
        const fileEntry = ctx.fileMap.get(module)
        if (fileEntry !== undefined) {
            return JSON.stringify(fileEntry[0].urlPath)
        }
        else {
            assert.unreachable()
        }
    }

    const styles: PageStyles = {
        styleMap: new Map
    }

    const docLines: string[] = []
    // docToHtml(ctx, styles, docLines, page.body)
    docToHtml2(ctx, styles, docLines, page.body)

    const styleLines: string[] = []

    for (const s of styles.styleMap.values()) {
        styleLines.push(`.${s.cssName} { ${s.cssDefn} }`)
    }

    const lines: string[] = [
        "<!DOCTYPE html>",
        "<html>",
        "<head>",
        '<meta charset="utf-8">',
        '<link rel="icon" href="data:,">',
        "<title>",
        uiTextToStr(page.head.title), // TODO html escapes
        "</title>",
        "</head>",
        "<body>",
        "<style>",
        "html { box-sizing: border-box; }",
        "*, *:before, *:after { box-sizing: inherit; }",
        // "body { margin: 0; width: 100vw; height: 100vh; font-family: sans-serif; }",
        "body { font-family: sans-serif; }",
        ...styleLines,
        "</style>",
    ]

    // By default, only include the app-scripts if there are apps within the page.
    if (opts.importAppScripts === true || opts.importAppScripts === undefined && ctx.numApps > 0) {
        lines.push(
            "<script type='module'>",
            `  import { setIo } from ${mod("/ts/gen/io/io.js")}`,
            `  import { mkIoBrowser } from ${mod("/ts/gen/io/io-browser.js")}`,
            `  setIo(mkIoBrowser(new URL('/', window.location.href)))`,
            "</script>",
        )
    }

    // By default, only include the reload-script if there are no apps within the page.
    //   Pages containing apps cannot be replaced in the same way, 
    //   as they contain <script> sections which won't get run.
    // To keep the page live, we need either:
    //   - something dumber, just reload the whole page, or
    //   - something smarter with special handling for the script sections.
    if (opts.importReloadScript === true || opts.importReloadScript === undefined && ctx.numApps === 0) {
        lines.push(
            `<script type="module">`,
            `import ${mod("/ts/gen/site/page-reload.js")}`,
            `</script>`
        )
    }

    if (typeof page.head.title === "string") {
        lines.push("<h1>")
        lines.push(page.head.title)
        lines.push("</h1>")
    }

    lines.push(...docLines)

    return lines
}


