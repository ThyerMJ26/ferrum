import { unit } from "../../utils/unit.js"
import { Page, Doc, pageBuild, mkDoc } from "../../site/page-doc.js"
import { PageModule } from "../../site/page-module.js"

const pageModule: PageModule = {
    page
}
export default pageModule


function page(): Page {
    return pageBuild((b): unit => {
        const { para: p, list: l } = b

        b.title("What is the motivation behind Ferrum?")

        p("The Ferrum programming language exists to help remove tedium in programming.",
            "Programming is all about removing tedium in other domains, ",
            "but when it comes to removing tedium in programming itself, ",
            "existing solutions always run out of steam, one way or another.")
        // TODO Stop writing blog-posts/web-pages directly in TypeScript, it's too tedious! 

        p("Many of the limits of existing solutions exist because:", l([
            "interpretive overhead is non-zero, and",
            "a fundamental tension exists between type-system expressability and decidability."
        ]))

        p("Ferrum provides:", l([
            "a way to specialize away interpretive overhead, and",
            "an expressive type-system.",
        ]))

        


    })
}

