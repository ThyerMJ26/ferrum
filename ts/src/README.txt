This directory contains the TypeScript source code for:
  - the Ferrum-in-TypeScript implementation,
  - the WebIDE,
  - the web site generator,
  - the web site content,


utils
    Assertions, and other utilities

io
    An Io interface, together with client-side (browser) and server-side (NodeJs) implementations.

ui
    Platform independent UiText and Ui definitions.
    A platform independent reactive node+signals implementation.
    An HTML-specific impelementation of the Ui interface.

syntax
    Anything related to concrete syntax.
    The tokens, scanner, operator-precedence parser and recursive descent parser.
    The pretty printer.
    The parsers for the project and test-defn files.

tree
    The tree-based implementation of the type-checker.
    This was written to be thrown away, and replaced with the Ferrum-in-Ferrum implementation.
    It turns out, implementing a novel type-check for an experimental language, in that same experimental language, isn't the best idea.
    Includes an evaluator, as arbitrary expression can be evaluated by the type-checker.
    Also includes a memoization implementation to remember expensize type calculations from one run to the next.

graph
    The graph-based implementation of the type-checker.
    This hasn't yet caught up with the tree-based version, but is intended to replace it.
    This also includes the code-table, which keeps track of which operations (type-check, codegen, etc) have been perform on which files within a project.

codegen
    Code generators for Js and C.
    Also include the CodeRunner interface which provides uniform access to the code runners.

runtime
    The primitives needed by the generated Js code.
    This is split into NodeJs-specific code and NodeJs-independent code (suitable for use in the browser).

runtest
    The bulk of the run-test code, this is browser/server independent, and can run in either.

cmds
    CLI commands.
    These are typically invoked by the small bash scripts in /cmds

ide
    The IDE implementation.
    The code-itself is independent of UI implementation.
    Currently, the only Ui implementation is HTML based.
    The app-logic can either run on the server-side or the client-side.
    Running client-side, means a static file-server can be used, good for deploying simple demos.
    Running server-side reduces the client-load, and means server-side debugging tools can be used.

site
    A site-independent website server/generator.
    This serves/exports files, pages, and apps.

ferrum-org
    The site-specfic website content for ferrum.org

    