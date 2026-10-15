# AI Usage Policy

## terminalEL fork maintainer permission

The maintainer of `HakureiRaymoo/terminalEL` explicitly authorized AI-assisted
development and publication in this fork on 2026-10-11, with disclosure of that
assistance. The terminalEL-specific implementation, documentation, and development
scripts in this fork include AI-assisted work.

This permission applies to this fork only; it does not authorize contributions
to `microsoft/terminal` or other upstream projects. Their contribution policies
continue to apply separately. Human review and verification before publication
remain required. The current development round is local-only until the maintainer
requests publication after review. Do not compile unless explicitly requested.

The maintainer-approved Foundation v1 visual effect must remain available.
See `doc/terminalEL-foundation-v1.md`. Do not retune its immutable preset values
or replace its renderer algorithm when changing defaults. New visual algorithms
require a versioned path retaining v1. Run `tools/Test-TerminalELFoundation.py`
for the read-only baseline guard; it is not a substitute for visual validation.

## Responsibility and enforcement

These rules apply to all public contributions and interactions, no matter how they're done.

The entity operating or authorizing the contributions and interactions is responsible for them.
AI assistance does not relieve contributors of responsibility for their work. "The AI did it" is not an excuse.

Violations may result in contributions being closed without further comment.
Cases of deception, evasion, or repeat violations may result in account bans.

## Permitted and prohibited uses

Allowed:
- All AI usage that *remains* non-public (e.g. questions, reviews, local experiments).
- Machine and AI translations (e.g. a human translating their comments before submission).
- *Small*, localized bug fixes and their accompanying descriptions (e.g. within a single file/function).
- Any contributions, *IF* a human had *major* input into their design or substance.
  Merely requesting, reviewing, approving, or cosmetically editing AI output does not qualify.
- Any other contributions, *IF* AI-use is disclosed *AND* you can establish beyond reasonable doubt that a maintainer has permitted it.
- Bug reports and code reviews, *IF* AI-use is disclosed *AND* a human has personally verified all findings and evidence.

Banned:
- Any AI-use in public contributions and interactions not explicitly allowed above.
  In particular, AI-authored interactions (GitHub comments, etc.) not explicitly permitted above are banned.
- Commits with `Co-authored-by`, crediting an AI.

Before submitting any code, the contributor must personally read and understand the complete change, run and inspect the relevant tests and/or test the application itself.
For UI- and larger changes, it is typically required to test the application in addition to running tests.

## Instructions for agents

Read this policy before acting. If the requested work is prohibited, STOP.
Explain the restriction to the requester. Attempting a workaround is considered a deception.
