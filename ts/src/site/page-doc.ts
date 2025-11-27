

import { assert } from "../utils/assert.js"
import { unit } from "../utils/unit.js"
import { UiText } from "../ui/text.js"

export type Page = {
    head: PageAttr
    body: PageDoc
}

export type PageAttr = {
    title: UiText
}

export type PageDoc = Doc

export type Doc =
    | UiText
    | Doc[]
    | { tag: "para", para: Doc }
    | { tag: "list", items: Doc[] }
    | { tag: "defns", defns: Defn[] }
    | { tag: "title", title: UiText }
    | { tag: "app-publish", instance: string }
    | { tag: "link-url", url: string, text?: UiText }
    | { tag: "link-file", file: string, text?: UiText }
    | { tag: "link-page", page: string, text?: UiText }


export type Defn = {
    term: UiText
    defn: Doc
}


export type DocMaker = {
    title(text: UiText): Doc
    para(para: Doc): Doc
    list(items: Doc[]): Doc
    defns(defns: Defn[]): Doc
    linkFile(file: string, text?: UiText): Doc
    linkPage(page: string, text?: UiText): Doc
    linkUrl(link: string, text?: UiText): Doc
    appPublish(instance: string): Doc
}


export const mkDoc: DocMaker = {
    title: (title: UiText) => ({ tag: "title", title }),
    para: (para: Doc) => ({ tag: "para", para }),
    list: (items: Doc[]) => ({ tag: "list", items }),
    defns: (defns: Defn[]) => ({ tag: "defns", defns }),
    appPublish: (instance: string) => ({ tag: "app-publish", instance }),
    linkFile: (file: string, text?: UiText) => ({ tag: "link-file", text, file }),
    linkPage: (page: string, text?: UiText) => ({ tag: "link-page", text, page }),
    linkUrl: (url: string, text?: UiText) => ({ tag: "link-url", text, url }),
}

export type DocBuilder = {
    add(doc: Doc): unit
    para(para: Doc): unit
    // list(elems: Doc[] | ((b: DocBuilder) => unit)): unit
    list(elems: Doc[]): unit
    // list(elems: (b: DocBuilder) => unit): unit
    item(doc: Doc): unit
    // item2(doc: Doc): unit
    // item3(doc: Doc): unit
    // item(level: number, doc: Doc): unit
    // defns(defns: Defn[] | ((b: DocBuilder) => unit)): unit
    defn(text: UiText, defn: Doc): unit

    link_file(file: string, text?: UiText): unit
    link_page(page: string, text?: UiText): unit
    link_url(url: string, text?: UiText): unit

    appPublish(appInstanceName: string): unit

}

export type PageBuilder =
    & DocBuilder
    & {
        title(text: UiText): unit
    }

export function docBuild(cb: (b: DocBuilder) => unit): Doc {

    const doc: Doc[] = []

    let ul_list: Doc[] | null = null
    let dl_list: Defn[] | null = null

    const b: DocBuilder = {
        add: d => { doc.push(d) },
        para: (para: Doc): unit => {
            ul_list = null
            dl_list = null
            doc.push(mkDoc.para(para))
        },
        list: (elems: Doc[]): unit => {
            ul_list = null
            dl_list = null
            doc.push(mkDoc.list(elems))
        },
        item: (item: Doc): unit => {
            dl_list = null
            if (ul_list === null) {
                ul_list = [] // mkDoc.list([])
                doc.push(mkDoc.list(ul_list))
            }
            ul_list.push(item)
        },
        defn: (term: UiText, defn: Doc): unit => {
            ul_list = null
            if (dl_list === null) {
                dl_list = [] // mkDoc.list([])
                doc.push(mkDoc.defns(dl_list))
            }
            dl_list.push({ term, defn })
        },
        link_file(page, text) {
            doc.push(mkDoc.linkFile(page, text))
        },
        link_page(page, text) {
            doc.push(mkDoc.linkPage(page, text))
        },
        link_url(page, text) {
            doc.push(mkDoc.linkUrl(page, text))
        },
        appPublish(appInstanceName) {
            doc.push(mkDoc.appPublish(appInstanceName))
        },
    }

    cb(b)

    return doc
}
export function pageBuild(cb: (b: PageBuilder) => unit): Page {

    let title: UiText = ""

    const doc = docBuild(db => {
        const pb: PageBuilder = {
            ...db,
            title: (text: UiText) => { title = text }
        }
        cb(pb)
    })

    const page: Page = { head: { title }, body: doc }

    return page
}

