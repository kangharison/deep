# AI Coding Assistants

> 출처(원문): https://docs.kernel.org/process/coding-assistants.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# AI Coding Assistants

This document provides guidance for AI tools and developers using AI
assistance when contributing to the Linux kernel.

AI tools helping with Linux kernel development should follow the standard
kernel development process:

* [A guide to the Kernel Development Process](development-process.html)
* [Linux kernel coding style](coding-style.html)
* [Submitting patches: the essential guide to getting your code into the kernel](submitting-patches.html)

## Licensing and Legal Requirements

All contributions must comply with the kernel’s licensing requirements:

* All code must be compatible with GPL-2.0-only
* Use appropriate SPDX license identifiers
* See [Linux kernel licensing rules](license-rules.html) for details

## Signed-off-by and Developer Certificate of Origin

AI agents MUST NOT add Signed-off-by tags. Only humans can legally
certify the Developer Certificate of Origin (DCO). The human submitter
is responsible for:

* Reviewing all AI-generated code
* Ensuring compliance with licensing requirements
* Adding their own Signed-off-by tag to certify the DCO
* Taking full responsibility for the contribution

## Attribution

When AI tools contribute to kernel development, proper attribution
helps track the evolving role of AI in the development process.
Contributions should include an Assisted-by tag in the following format:

```
Assisted-by: AGENT_NAME:MODEL_VERSION [TOOL1] [TOOL2]
```

Where:

* `AGENT_NAME` is the name of the AI tool or framework
* `MODEL_VERSION` is the specific model version used
* `[TOOL1] [TOOL2]` are optional specialized analysis tools used
  (e.g., coccinelle, sparse, smatch, clang-tidy)

Basic development tools (git, gcc, make, editors) should not be listed.

Example:

```
Assisted-by: Claude:claude-3-opus coccinelle sparse
```
