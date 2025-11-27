import { unit } from "../../utils/unit.js"
import { UiText, uiTextS } from "../../ui/text.js"
import { Page, Doc, pageBuild, mkDoc } from "../../site/page-doc.js"
import { PageModule } from "../../site/page-module.js"

const pageModule: PageModule = {
    page2
}
export default pageModule

function page2(): Page {

    return pageBuild(b => {
        const m = mkDoc
        const { item: i, para: p } = b

        b.title("Ferrum")

        b.defn("A language:", ["Functional with set-theoretic first-class types."])
        b.defn("An implementation:", ["A specializing translator (a specialator)."])
        b.defn("An experimental work-in-progress", [])

        p(["The repository contains details of the implementation:", m.linkUrl("https://github.com/ThyerMJ26/ferrum")])
        p("The pages on this website explain the language.")

        i(m.linkPage("./tutorial.js", "Tutorial (Work-in-Progress)"))
        i(m.linkPage("./glossary.js", "Glossary (Work-in-Progress)"))

    })

}


