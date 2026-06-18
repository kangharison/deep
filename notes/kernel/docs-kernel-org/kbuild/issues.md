# Recursion issues

> 출처(원문): https://docs.kernel.org/kbuild/issues.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Recursion issues

## issue #1

```
# Simple Kconfig recursive issue
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Test with:
#
# make KBUILD_KCONFIG=Documentation/kbuild/Kconfig.recursion-issue-01 allnoconfig
#
# This Kconfig file has a simple recursive dependency issue. In order to
# understand why this recursive dependency issue occurs lets consider what
# Kconfig needs to address. We iterate over what Kconfig needs to address
# by stepping through the questions it needs to address sequentially.
#
#  * What values are possible for CORE?
#
# CORE_BELL_A_ADVANCED selects CORE, which means that it influences the values
# that are possible for CORE. So for example if CORE_BELL_A_ADVANCED is 'y',
# CORE must be 'y' too.
#
#  * What influences CORE_BELL_A_ADVANCED?
#
# As the name implies CORE_BELL_A_ADVANCED is an advanced feature of
# CORE_BELL_A so naturally it depends on CORE_BELL_A. So if CORE_BELL_A is 'y'
# we know CORE_BELL_A_ADVANCED can be 'y' too.
#
#   * What influences CORE_BELL_A?
#
# CORE_BELL_A depends on CORE, so CORE influences CORE_BELL_A.
#
# But that is a problem, because this means that in order to determine
# what values are possible for CORE we ended up needing to address questions
# regarding possible values of CORE itself again. Answering the original
# question of what are the possible values of CORE would make the kconfig
# tools run in a loop. When this happens Kconfig exits and complains about
# the "recursive dependency detected" error.
#
# Reading the Documentation/kbuild/Kconfig.recursion-issue-01 file it may be
# obvious that an easy solution to this problem should just be the removal
# of the "select CORE" from CORE_BELL_A_ADVANCED as that is implicit already
# since CORE_BELL_A depends on CORE. Recursive dependency issues are not always
# so trivial to resolve, we provide another example below of practical
# implications of this recursive issue where the solution is perhaps not so
# easy to understand. Note that matching semantics on the dependency on
# CORE also consist of a solution to this recursive problem.

mainmenu "Simple example to demo kconfig recursive dependency issue"

config CORE
	tristate

config CORE_BELL_A
	tristate
	depends on CORE

config CORE_BELL_A_ADVANCED
	tristate
	depends on CORE_BELL_A
	select CORE
```

## issue #2

```
# Cumulative Kconfig recursive issue
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# Test with:
#
# make KBUILD_KCONFIG=Documentation/kbuild/Kconfig.recursion-issue-02 allnoconfig
#
# The recursive limitations with Kconfig has some non intuitive implications on
# kconfig semantics which are documented here. One known practical implication
# of the recursive limitation is that drivers cannot negate features from other
# drivers if they share a common core requirement and use disjoint semantics to
# annotate those requirements, ie, some drivers use "depends on" while others
# use "select". For instance it means if a driver A and driver B share the same
# core requirement, and one uses "select" while the other uses "depends on" to
# annotate this, all features that driver A selects cannot now be negated by
# driver B.
#
# A perhaps not so obvious implication of this is that, if semantics on these
# core requirements are not carefully synced, as drivers evolve features
# they select or depend on end up becoming shared requirements which cannot be
# negated by other drivers.
#
# The example provided in Documentation/kbuild/Kconfig.recursion-issue-02
# describes a simple driver core layout of example features a kernel might
# have. Let's assume we have some CORE functionality, then the kernel has a
# series of bells and whistles it desires to implement, its not so advanced so
# it only supports bells at this time: CORE_BELL_A and CORE_BELL_B. If
# CORE_BELL_A has some advanced feature CORE_BELL_A_ADVANCED which selects
# CORE_BELL_A then CORE_BELL_A ends up becoming a common BELL feature which
# other bells in the system cannot negate. The reason for this issue is
# due to the disjoint use of semantics on expressing each bell's relationship
# with CORE, one uses "depends on" while the other uses "select". Another
# more important reason is that kconfig does not check for dependencies listed
# under 'select' for a symbol, when such symbols are selected kconfig them
# as mandatory required symbols. For more details on the heavy handed nature
# of select refer to Documentation/kbuild/Kconfig.select-break
#
# To fix this the "depends on CORE" must be changed to "select CORE", or the
# "select CORE" must be changed to "depends on CORE".
#
# For an example real world scenario issue refer to the attempt to remove
# "select FW_LOADER" [0], in the end the simple alternative solution to this
# problem consisted on matching semantics with newly introduced features.
#
# [0] https://lore.kernel.org/r/1432241149-8762-1-git-send-email-mcgrof@do-not-panic.com

mainmenu "Simple example to demo cumulative kconfig recursive dependency implication"

config CORE
	tristate

config CORE_BELL_A
	tristate
	depends on CORE

config CORE_BELL_A_ADVANCED
	tristate
	select CORE_BELL_A

config CORE_BELL_B
	tristate
	depends on !CORE_BELL_A
	select CORE
```
