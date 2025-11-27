
import { unit } from "../../utils/unit.js"
import { PageModule } from "../../site/page-module.js";
import { Page, pageBuild } from "../../site/page-doc.js";

const pm: PageModule = {
    page2
}
export default pm


export function page2(): Page {
    return pageBuild(b => {
        b.title("Examples");

        b.para("Ex")
        b.appPublish("EX-1")

        b.para("fe4-.test.fe")
        b.appPublish("IDE-1")

        b.para("fe4a.test.fe")
        b.appPublish("IDE-2")
    })

}

