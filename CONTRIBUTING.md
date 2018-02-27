# How to contribute

## Contributors

Third-party patches are always welcome, for any purpose.

Although the repository is available from GitHub, please
use the main site for bug reporting/merges/discussions.

Please be aware that libRaptorQ is a dual-licensed software,
and thus it's required that all contributors sign a CLA agreement.
The agreement lets us relicense the code and protect us from patents,
but you will otherwise keep the rights on your code. More on that
on the CLA section.


Aside from the legal requirements, there are a few guidelines
we would like you to follow when contributing, so we can more
easily manage our project.


## Getting Started

Read the README. Code is C++11. Newer standards are not allowed,
as we want to keep compatibility with centos 7, gcc 4.8 at least.
Beside, C++14 support is still incomplete in a lot of compilers.


Please make an account on https://www.fenrirproject.org
If you already have a github account, you should be able to login with that.

Bug reporting should be handled in the main website, not on github.

To contact the developers, please use the
[#fenrirproject IRC channel on Freenode](https://freenode.net) or use
the [mailing lists](https://www.fenrirproject.org/lists)


## Git handling

The development happens on the "master" branch.
When multiple versions of the library are supported,
the latest one will be on "master", and the older ones will get a
dedicated branch.

We are trying to ensure the trust of our codebase, so the commits
should always be signed with the "-sS" git option.

As better explained in [A git Horror story](https://mikegerwitz.com/papers/git-horror-story)
lots of bad things can happen.

Only the maintainers are required to gpg-sign the commits, while
external contributors should only use the "-s" git option to
add the "Signed-off-by" line, which will indicate that the
developer accepted the CLA and is the author of the commit.


## Making changes

* Create a branch from where you want to base your work
* Make commits of logical units, do not clump together multiple patches
* Make sure your git has your information:
  * git config --global user.name "John Doe"
  * git config --global user.email johndoe@example.com
* Please use meaningful commit messages.
  * Always sign your commits with the "-s" option, to add the "Signed-off-by" line in the commit message
  * Only the main developers are required to GPG-sign the commits.
* Run the tests to make sure you have not broken something by mistake
* Please keep commits short and to the point. Nobody wants to review 2000+ lines of code that touch everything.

## Documentation

Latex is required to build the documentation, which should be aimed at
new developers. Multiple packages are requires, so you probably
want to install the full version of texlive.

The recommended editor is either texstudio or texworks, as they
provide a syntax highlighting and a quick way to jump between the preview
and the code.



## Submitting Changes

* Sign the [Contributor License Agreement](https://www.fenrirproject.org/Fenrir/Fenrir_Project/wikis/CLA).
    * This CLA comprehends also contributions to the other Fenrir Project subprojects (libFenrir, Fenrird etc..)
    * In short, you grant us (nonexclusive) relicensing rights, patent protection and keep your rights as author. Read the CLA for full information
* push changes to your repository and submit a pull request
    * use "git -s" to add the "Signed-off-by" line in the commit.
* keep in contact with the maintainer for feedback


# Additional Resources

* [Main Fenrir Project website](https://www.fenrirproject.org)
* [Bug tracker (GitLab)](https://www.fenrirproject.org/Luker/libRaptorQ/issues)
* [Contributor License Agreement](https://www.fenrirproject.org/Fenrir/Fenrir_Project/wikis/CLA)
* [General GitHub documentation](https://help.github.com/)
* [GitHub pull request documentation](https://help.github.com/send-pull-requests/)
* [libRaptorQ mailing lists](https://www.fenrirproject.org/lists)

