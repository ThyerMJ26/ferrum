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

        b.title("What is Ferrum?")

        p("Ferrum is the solution, when the problem is too many forms of abstraction.",
            "(or at least, that is what it aspires to be.)"
        )

        // p("Ferrum is an experiment in using a single form of abstraction in-place of multiple forms.",
        // )

        // p("Ferrum is an experiment in using a smaller number of language features.",
        // )

        p("Programming languages often have more than one form of abstraction.",
            // "Macros, templates, functions, type-constructors, staging",
            "Examples include:", l(["Macros", "templates", "functions", "type-constructors", "staging"]),
            "These can help with moving computation from run-time to compile-time.",
            "For example, a generic grammar-taking parser written using macros or templates, can perform all the grammar-specific and input-independent work at compile-time.",
        )

        p("The downside of multiple forms of abstraction, is that you may then need to write the essentially same code multiple times.",
            "Abstractions are created to avoid repetition.",
            "The act of introducing a new form of abstraction, changes the very thing we are trying to abstract over.",
            "Ferrum aims to provide a single form of abstraction that can be used for everything.",
        )



        p("Ferrum is unusual in a number of ways:", b.list([
            "Terms and types are abstracted over together,",
            "graph-reduction is performed beneath lambdas, and",
            "pattern-match failure can be handled by the caller.",
        ]))

        p("Is this all too good to be true?",
            "Well, maybe.",
            "The tension between type-system expressability and decidability means we cannot have everything desirable simultaneously."
        )

        
        
    })
}

