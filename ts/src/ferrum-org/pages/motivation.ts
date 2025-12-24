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

        p("The Ferrum programming language exists to help reduce tedium in programming.",
            "Programming is all about reducing tedium in other domains, ",
            "but when it comes to reducing tedium in programming itself, ",
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

        // p("ΛX.λx:X.x")

        b.section("Meta-programming",
            p(
                "Many programming language have features that help support meta-programming.",
                "Examples include:",
                l([
                    "templates,",
                    "macros,",
                    "reflection/introspection."
                ])
            ),

            p(
                "Templates and macros can enable computations to be moved to compile-time.",
                "The benefit of this is faster execution at run-time.",
                "The drawback is that code now needs to be written differently depending on when it is to be run.",
                "This can be a problem if we want to use that same code at different times.",
                "If there are two implementations, they risk getting out of sync."
            ),
            p(
                "Consider a parser.",
                "A parser can be written in an interpretive style to interpret the rules in the grammar.",
                "If this interpretation is done in templates or macros, the resulting parser will run without interpretive overhead.",
                "The downside is that the parsing algorithm can now only be given a new grammar at compile-time.",
                "It would not be possible to write a diagnostic tool for use during grammar development which used the same parser implementation."                
            ),
            p(
                "The prospect of dynamic grammars may sound contrived.",
                "For a more concrete example, consider a parser for a binary format, such as an image or video file (or stream).",
                "Binary formats are often highly parameterized, for example, by colour depth.",
                "The header will contain the actualy parameters, the body then conforms to those parameters.",
                "Generating parsers for every possible parameterization may result in prohibitively large executables.",
                "Always consulting the parameters may result in prohibitively long execution time.",
                "Somewhere between the two is a tradeoff.",
                "But where may depend on external factors.",
                "For a diagnostic tool, flexability and universality is more important.",
                "For an embedded device, efficiency for a specific case is more important."
            ),
            p(
                // "For a format with a stable specification, multiple implementations is a workable solution.",

            )
        )

        // b.section("Data and Code", 
        b.section("(Code vs Data) vs (Code as Data)",
            p(

            ),


        )


        b.section("What if interpretation were free?")

        b.section("What if we could express any type?")

    })
}

function f(x: unknown): boolean {
    return typeof x === "string"
    // return x === null
}

