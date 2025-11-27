import { unit } from "../../utils/unit.js"
// import { PageBuilder } from "../../site/page-builder.js"

import { definitions } from "../data/glossary-defs.js"
import { PageModule } from "../../site/page-module.js"
import { Page, pageBuild } from "../../site/page-doc.js"

const pageModule: PageModule = {
    page2 
}
export default pageModule


export function page2(): Page {
    return pageBuild(b => {
        b.title("Glossary");

        for (const def of definitions) {
            b.defn(def.name, def.text)
        }

    })

}

