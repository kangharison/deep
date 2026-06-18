# A guide to the Kernel Development Process

> 출처(원문): https://docs.kernel.org/process/development-process.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# A guide to the Kernel Development Process

The purpose of this document is to help developers (and their managers)
work with the development community with a minimum of frustration. It is
an attempt to document how this community works in a way which is
accessible to those who are not intimately familiar with Linux kernel
development (or, indeed, free software development in general). While
there is some technical material here, this is very much a process-oriented
discussion which does not require a deep knowledge of kernel programming to
understand.

Contents

* [1. Introduction](1.Intro.html)
  + [1.1. Executive summary](1.Intro.html#executive-summary)
  + [1.2. What this document is about](1.Intro.html#what-this-document-is-about)
  + [1.3. Credits](1.Intro.html#credits)
  + [1.4. The importance of getting code into the mainline](1.Intro.html#the-importance-of-getting-code-into-the-mainline)
  + [1.5. Licensing](1.Intro.html#licensing)
* [2. How the development process works](2.Process.html)
  + [2.1. The big picture](2.Process.html#the-big-picture)
  + [2.2. The lifecycle of a patch](2.Process.html#the-lifecycle-of-a-patch)
  + [2.3. How patches get into the Kernel](2.Process.html#how-patches-get-into-the-kernel)
  + [2.4. Next trees](2.Process.html#next-trees)
  + [2.5. Staging trees](2.Process.html#staging-trees)
  + [2.6. Tools](2.Process.html#tools)
  + [2.7. Mailing lists](2.Process.html#mailing-lists)
  + [2.8. Getting started with Kernel development](2.Process.html#getting-started-with-kernel-development)
* [3. Early-stage planning](3.Early-stage.html)
  + [3.1. Specifying the problem](3.Early-stage.html#specifying-the-problem)
  + [3.2. Early discussion](3.Early-stage.html#early-discussion)
  + [3.3. Who do you talk to?](3.Early-stage.html#who-do-you-talk-to)
  + [3.4. When to post?](3.Early-stage.html#when-to-post)
  + [3.5. Getting official buy-in](3.Early-stage.html#getting-official-buy-in)
* [4. Getting the code right](4.Coding.html)
  + [4.1. Pitfalls](4.Coding.html#pitfalls)
  + [4.2. Code checking tools](4.Coding.html#code-checking-tools)
  + [4.3. Documentation](4.Coding.html#documentation)
  + [4.4. Internal API changes](4.Coding.html#internal-api-changes)
* [5. Posting patches](5.Posting.html)
  + [5.1. When to post](5.Posting.html#when-to-post)
  + [5.2. Before creating patches](5.Posting.html#before-creating-patches)
  + [5.3. Patch preparation](5.Posting.html#patch-preparation)
  + [5.4. Patch formatting and changelogs](5.Posting.html#patch-formatting-and-changelogs)
  + [5.5. Sending the patch](5.Posting.html#sending-the-patch)
* [6. Followthrough](6.Followthrough.html)
  + [6.1. Working with reviewers](6.Followthrough.html#working-with-reviewers)
  + [6.2. What happens next](6.Followthrough.html#what-happens-next)
  + [6.3. Other things that can happen](6.Followthrough.html#other-things-that-can-happen)
* [7. Advanced topics](7.AdvancedTopics.html)
  + [7.1. Managing patches with git](7.AdvancedTopics.html#managing-patches-with-git)
  + [7.2. Reviewing patches](7.AdvancedTopics.html#reviewing-patches)
* [8. For more information](8.Conclusion.html)
* [9. Conclusion](8.Conclusion.html#conclusion)
