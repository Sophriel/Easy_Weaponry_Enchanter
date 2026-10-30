---
description: 플랜 파일 하나를 코드베이스와 대조 검증한다 (수동 진입점)
argument-hint: <플랜 파일 경로>
allowed-tools: Read, Grep, Glob, Write(Documents/MDs/plans/**), Bash(git diff:*), Bash(git log:*), Bash(git status:*), Bash(git show:*)
---

`$1` 플랜을 검증한다. `plan-verifier` 서브에이전트와 같은 일을, 수동으로 즉시 돌리는 진입점이다.
Task 툴로 `plan-verifier` 서브에이전트를 띄우고 다음 프롬프트를 넘겨라:

> `$1` 파일의 플랜을 검증하라.

서브에이전트가 `.claude/plan-verify/CRITERIA.md` 의 기준으로 검증하고, PASS 면 판정만 보고, REVISE 면 `Documents/MDs/plans/` 에 개정판을 작성한다.
**이 커맨드 자체는 코드를 수정하지 않는다.** 구현 여부는 검증 결과를 보고 사용자와 결정한다.
