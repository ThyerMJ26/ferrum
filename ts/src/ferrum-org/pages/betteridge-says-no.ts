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

        b.title("Can Russel's Paradox be resolved without throwing the baby out with the bathwater? <br> Betteridge says no, but is he right?")

        b.section("Paradox",
            b.defns([
                ["Russel's Paradox", "Does the set of all sets that don't contain themselves, contain itself?"],
                ["Betteridge's Law", "Any headline posed as a question can be answered no."],
                ["Liar Paradox", "This sentence is false."],
                ["Quine's Paradox", '"yields falsehood when preceded by its quotation" yields falsehood when preceded by its quotation.'],
            ]),

            p(
                "Russel's Paradox demonstrates that unrestricted set-theory can lead to contradictions.",
                "Similarly, unrestricted logical statements, such as the Liar Paradox, can lead to contradictions.",
                "To make set-theory consistent (free of contradictions), restrictions must be imposed.",
            ))
        b.section("Set Theory",
            p(
                "This is the problematic construction:",
                "{ x | x ∉ x }",
                "Different set-theories forbid it on different grounds.",
                // "Zermelo–Fraenkel (ZF) requires every set to be constructed with reference to an already existing set.",
                "Zermelo–Fraenkel (ZF) forbids this because 'x' is not drawn from an already existing set.",
                // `Quine's New Foundations (NF) places the restriction on the use of "x" on both sides of the negated set-membership (∉) operator.`,
                `Quine's New Foundations (NF) forbids this because "x" is used on both sides of the negated set-membership (∉) operator.`,
                "Whichever rules are used to prevent contradictions, will also forbid some otherwise non-problematic constructions.",
                "For example, NF permits the universal set, whereas ZF forbids it.",
                "(Examples of sets permited in ZF but not NF exist, but are a bit more involved.)"
            ))
        // p(
        //     "Using rules to prevent set theoretic contradictions is similar to using rules to prevent logical contradictions.",
        // ),
        b.section("Type Theory",
            p(
                "Type-theory was invented to stop philosopher's talking nonsense.",
                "At first glance, the problem with the Liar Paradox is self-reference.",
                "Quine's paradox demonstrates that contradictions can arise without self-reference.",
                "Type theory can prevent the construction of such contradictory sentences.",
                "However, as with set theory, any specific type theory will forbid meaningful terms as well as contradictory ones."
            ))

        b.section("Computation", p(
                "As well as contradictory/meaningless sets and sentences, we can have contradictory programs:",
                "function f() { return !f(); }",
                "If this function returns true, it returns false, and if it returns false, it returns true.",
                "Type systems for most programming languages don't concern themselves with termination.",
                "Type theory, on the other hand, does.",
            )
        )

        // Sound and Complete
        // Not everything can be decided, but can it even be expressed.
        //   No: meaning I'm sure it's a no.
        // vs
        //   No: meaning, I'm not sure it's a yes.

        // Should everything expressable be decidable?

        // Unspeakable truths.



        b.section("Decidability", p(
            "",
        ))

        b.section("Expressability", p(
            "",
        ))

    })
}