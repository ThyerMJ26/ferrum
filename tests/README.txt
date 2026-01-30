This directory contains the tests used by the Ferrum-in-TypeScript implementation.

( There are separate tests for the Ferrum-in-Ferrum implementation, 
    which are now in /fe/fe-in-fe/tests,
  The same tests used to be used for both, 
    but keeping everything in sync while still experimenting was more of a hindrance than a help.
)


short.test.fe
    Contains short tests with no associated project file.
    Some of these tests are written from scratch to test a single feature.
    Some have been extracted from larger tests, so as to reproduce a bug in a more minimal way.

fif-short.test.fe
    This imports just the first few files from the Ferrum-in-Ferrum project.
    This mainly exists so as to check to process of importing lots of files,
      without actually importing lots of files.

fif-medium.test.fe
    This imports the full Ferrum-in-Ferrum project,
      but then only runs targetted tests.

fif-long.test.fe
    This imports the full Ferrum-in-Ferrum project,
      and then uses the test-runner written in Ferrum to run (what used to be the same) sets of tests.

examples.test.fe
    Small examples, so as to demonstrate the point of this whole endeavour.
    This needs more fleshing out.

kv-fails.test.fe
    Tests that worked in the tree-based type-checker implementation,
      but don't (yet?) work in the graph-based type-checker implementation.
    The coding style used in these tests isn't currently used in any other places.
    It's not entirely clear if the best approach to these tests is to:
      - improve the type-checker to handle the coding-style, or
      - change the coding-style to suit the type-checker.
    Neither approach needs to be pursued, and the tests could just be abandoned.


cases/*.test-case.fe
    TODO ? Separate out the individual tests so as to have one test per file.

sets/*.test-set.fe
    TODO ? Collect together sets of tests, and the modes in which they should be run.
           Currently, a number of CLI settings control how tests are run.
           There are:
             - two type-checker implementations (tree-based and graph-based Fe-in-Ts, +1 in Fe-in-Fe),
             - A number of code-generators (some have been deleted, more will be added).
             - A number of settings to control where evaluation and specialization occurs.
                 (it can be useful to toggle things on/off during development).
           These setting should be included in the test-set files to make clearer what should be run.



