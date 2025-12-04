import { Page, Doc, pageBuild, mkDoc } from "../../site/page-doc.js"
import { PageModule } from "../../site/page-module.js"

const pageModule: PageModule = {
    page2
}
export default pageModule


function page2(): Page {

    return pageBuild(b => {
        b.title("Tutorial")
    })
}

