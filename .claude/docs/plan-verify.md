# Plan Verifier — 플랜 검증 파이프라인

> 상위 문서: [README.md](../README.md)

Cowork(클라우드) 세션에서 작성한 작업 플랜을, 로컬 코드베이스와 대조해 자동으로 검증하는 구조다.

## 왜 필요한가

Cowork 세션은 클라우드 컨테이너에서 돌기 때문에 이 저장소의 실제 코드를 갖고 있지 않다. 그래서 그럴듯하지만 **실재하지 않는 함수·잘못된 전제·깨진 인덱스 대응**을 담은 플랜이 나올 수 있다. 로컬에서 도는 Claude Code 세션이 그 플랜을 받아 코드와 1:1 대조하는 것이 이 파이프라인의 목적이다.

Cowork에서 재형님 컴퓨터로 **파일을 쓰는 것은 가능하지만 프로세스를 실행하는 것은 불가능**하다. 그래서 "파일 하나를 투입구로 삼고, 로컬에 상주하는 세션이 그 변경을 감지한다"는 형태가 됐다.

## 흐름

```
Cowork 세션
  플랜 작성 → device_bash 로 <저장소 루트>/plan_request.md 에 플랜 본문 쓰기
     ↓
Plan Verifier 세션 (로컬, 상시 대기)
  FileChanged 훅 발화 → on-plan-request.ps1
    · 플랜을 .claude/plan-verify/inbox/ 로 접수
    · plan_request.md 를 IDLE 로 되돌림
    · exit 2 → asyncRewake 로 유휴 세션을 깨움
     ↓
plan-verifier 서브에이전트 (Task 툴)
  CRITERIA.md 기준으로 검증
    · PASS   → 판정만 보고. 구현 여부는 사용자가 결정
    · REVISE → .claude/plans/<YYMMDD>-<작업-슬러그>/ 에 개정판(-v2, -v3) 작성
```

## 구성 요소

| 파일 | 추적 | 역할 |
| :--- | :---: | :--- |
| `<저장소 루트>/plan_request.md` | X | **투입구.** 항상 존재하며 내용이 `IDLE` 이면 대기 상태 |
| `settings.json` 의 `FileChanged` 항목 | O | `plan_request.md` 를 감시 대상으로 등록 |
| `hooks/on-plan-request.ps1` | O | 훅 본체. 접수·상태 갱신·세션 깨우기 |
| `agents/plan-verifier.md` | O | 검증 서브에이전트 정의 |
| `commands/verify-plan.md` | O | 수동 진입점 (`/verify-plan <경로>`) |
| `plan-verify/` | X | 런타임 데이터 |
| `batches/2 Launch Claude-Plan Verifier.bat` | O | Plan Verifier 세션 런처 |
| `batches/1 Launch llama Server.bat` | O | 로컬 LLM 서버 런처. 검증 세션이 이 서버를 쓴다 |

### `plan-verify/` 내부

| 항목 | 역할 |
| :--- | :--- |
| `CRITERIA.md` | 검증 기준·판정 형식·개정판 작성 규칙. 서브에이전트와 커맨드가 **공유**한다 |
| `inbox/` | 접수된 플랜 원본 보관 |
| `.state.json` | 마지막 처리 내용의 SHA256 과 시각. 중복·연쇄 발화 차단용 |
| `hook.log` | 훅 발화 기록. 진단의 유일한 근거 |

`plan-verify/` 는 `.gitignore` 처리된 로컬 전용이다. 커밋되지 않는다.

## 루프 방지 — 설계 근거

개정판 플랜을 쓰는 행위가 다시 훅을 부르면 무한 루프가 된다. 4중으로 막는다.

| # | 장치 | 막는 것 |
| :---: | :--- | :--- |
| 1 | **경로 분리** | 감시 대상은 `plan_request.md` **한 파일**. 산출물은 `.claude/plans/` — 감시 대상이 아니다. 구조적으로 재발화가 불가능하다 |
| 2 | **IDLE 판정** | 되돌린 쓰기로 훅이 다시 불려도 내용이 `IDLE` 이면 즉시 종료 |
| 3 | **해시 중복 차단** | 같은 내용이 다시 들어오면 무시 |
| 4 | **10초 쿨다운** | 연쇄 발화 차단 |

**1번이 핵심이다.** 나머지는 보험이다. 구조를 손댈 때 `plan-verify/` 아래에 검증 산출물을 쓰도록 바꾸면 1번이 무너진다. 그래서 `CRITERIA.md` 와 `agents/plan-verifier.md` 양쪽에 "`.claude/plan-verify/` 아래에 쓰지 말 것"을 금지 항목으로 명시해 두었다.

## 사용법

1. `.claude/batches/2 Launch Claude-Plan Verifier.bat` 를 실행해 Plan Verifier 세션을 띄운다. 세션은 그대로 두면 된다 — `asyncRewake` 가 유휴 상태에서도 깨운다.
2. Cowork 세션에서 `plan_request.md` 에 플랜 본문을 쓴다.
3. 로컬 세션이 깨어나 서브에이전트에 검증을 위임한다.

수동으로 특정 플랜을 검증하려면 아무 세션에서나 `/verify-plan <플랜 경로>` 를 쓴다.

### 주의

- 세션 시작 시점에 `plan_request.md` 가 **존재해야** 감시에 등록된다. 지우지 말고 `IDLE` 로 남겨둘 것
- `settings.json` 을 수정하면 세션을 새로 띄워야 반영된다
- 검증 세션이 로컬 LLM(llama 서버)을 쓰는 구성이라면 `.claude/batches/1 Launch llama Server.bat` 이 먼저 떠 있어야 한다

## 진단

`plan-verify/hook.log` 를 본다.

| 로그 | 의미 |
| :--- | :--- |
| `ACCEPTED -> ...` | 정상 접수. 세션이 깨어나 서브에이전트를 띄워야 한다 |
| `fired, IDLE` | 훅은 돌았고 할 일이 없었다. 되돌린 쓰기 직후 정상적으로 나타난다 |
| `fired, duplicate hash` / `fired, cooldown` | 중복 요청을 차단했다 |
| **아무것도 없음** | 훅이 발화하지 않았다. matcher 이름 또는 `plan_request.md` 존재 여부를 확인할 것 |

## 알려진 함정

PowerShell 5.1 의 `Get-Content` / `Set-Content` 는 인코딩을 지정하지 않으면 UTF-8 한글을 CP949로 처리해 모지바케를 만든다(`# 작업 지시서` → `# ?묒뾽 吏?쒖꽌`). 훅 스크립트의 모든 파일 입출력은 `[System.IO.File]::ReadAllText` / `WriteAllText` 와 UTF-8(BOM 없음)로 고정돼 있다. **이 부분을 `Set-Content` 로 되돌리지 말 것.**
