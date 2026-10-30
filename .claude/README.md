# Claude Code 설정 — EWE

이 폴더는 EWE 프로젝트의 Claude Code 설정을 담는다. git으로 추적되어 팀 전체가
동일한 설정을 공유한다. (예외: `plan-verify/` 는 로컬 전용 런타임 데이터로
`.gitignore` 처리돼 있다)

**첫 세션을 시작하기 전에 아래 경고와 [권한 및 경로 규칙](docs/permissions.md)을
반드시 읽을 것.**

---

## ⚠️ 첫 세션 전 필독

일부 권한 규칙에 **작성자 컴퓨터 기준의 절대 경로**가 하드코딩돼 있다.
언리얼 엔진이 다른 위치에 설치돼 있다면 `settings.local.json` 과 `CLAUDE.md` 의
빌드 명령을 수정해야 한다.

수정하지 않아도 **망가지지는 않는다.** 다만 빌드 명령마다 승인 창이 뜬다.

→ 상세: [docs/permissions.md](docs/permissions.md) 의 "머신별 절대 경로"

---

## 문서 맵

| 문서 | 내용 |
| :--- | :--- |
| `README.md` (이 문서) | 폴더 구성, 줄바꿈 정책, 문서 색인 |
| [`docs/permissions.md`](docs/permissions.md) | 경로 문법, 권한 규칙 구획별 설명, 제거된 규칙 이력 |
| [`docs/plan-verify.md`](docs/plan-verify.md) | Plan Verifier — 플랜 검증 파이프라인 |

- 문서는 **한국어로 작성**한다. 영문 문서는 유지하지 않는다
- **문서를 추가·이동·삭제하면 위 표를 함께 갱신할 것**
- 어느 문서에도 맞지 않는 내용이면 새 문서를 임의로 만들지 말고 먼저 사용자에게 물을 것

---

## 폴더 구성

| 파일 | 추적 | 용도 |
| :--- | :---: | :--- |
| `settings.json` | O | 공유 프로젝트 설정. `PostToolUse` / `FileChanged` 훅을 담는다 |
| `settings.local.json` | O | 권한 규칙. "Yes, don't ask again" 선택 시 Claude Code가 자동으로 추가한다 |
| `docs/` | O | 이 폴더의 설명 문서 |
| `batches/` | O | 세션·서버 실행용 배치 스크립트 |
| `agents/plan-verifier.md` | O | 플랜 검증 서브에이전트 정의 |
| `commands/verify-plan.md` | O | 플랜 검증 수동 진입점 (`/verify-plan <경로>`) |
| `hooks/normalize-eol.ps1` | O | `Write` / `Edit` 직후 파일을 CRLF로 재작성 |
| `hooks/on-plan-request.ps1` | O | `plan_request.md` 변경 감지 → 검증 요청 접수 |
| `skills/` | O | 프로젝트 전용 스킬 |
| `plan-verify/` | **X** | 검증 파이프라인 런타임 데이터 (기준 문서, 접수함, 상태, 로그) |

> **참고:** JSON은 주석을 지원하지 않고 Claude Code는 JSONC도 받지 않는다.
> 따라서 설정 파일은 엄격한 JSON을 유지해야 하며, 모든 설명은 이 문서들에
> 대신 기록한다.

관련 파일이 `.claude/` 밖에도 하나 있다. 저장소 루트의 `plan_request.md` 는
Plan Verifier의 투입구다. → [docs/plan-verify.md](docs/plan-verify.md)

---

## 배치 스크립트 — `batches/`

| 파일 | 용도 |
| :--- | :--- |
| `1 Launch llama Server.bat` | 로컬 LLM(llama-server) 기동. Plan Verifier 세션이 이 서버를 모델로 쓴다 |
| `2 Launch Claude-Plan Verifier.bat` | Plan Verifier 세션 실행. 프로젝트 루트로 이동한 뒤 로컬 서버를 가리키는 환경변수를 설정하고 `claude --name "Plan Verifier"` 를 띄운다 |

번호는 **실행 순서**다. llama 서버가 떠 있지 않으면 Claude 세션이 모델에 연결하지 못한다.

### ⚠️ 이 스크립트들도 머신별 경로를 갖는다

| 파일 | 수정할 항목 |
| :--- | :--- |
| `1 Launch llama Server.bat` | `LLAMA_DIR`, `MODEL_DIR` — llama.cpp 바이너리와 모델(.gguf) 위치 |
| `2 Launch Claude-Plan Verifier.bat` | `DEFAULT_DIR` — 저장소 루트. `ANTHROPIC_MODEL` — 사용하는 모델 이름 |

`2 Launch Claude-Plan Verifier.bat` 는 인자로 프로젝트 경로를 받을 수도 있다(드래그 앤 드롭 포함). 인자 없이 더블클릭하면 `DEFAULT_DIR` 을 쓴다.

Plan Verifier 세션이 무엇을 하는지는 [docs/plan-verify.md](docs/plan-verify.md) 참조.

---

## 줄바꿈 정책

`.gitattributes`가 저장소에는 LF로 저장하고 작업 트리에는 CRLF로 체크아웃합니다.
추가로 `PostToolUse` 훅이 Claude가 건드린 파일을 CRLF로 재작성해서, 디스크
상태가 Visual Studio / Rider가 기대하는 형식과 일치하도록 맞춥니다.

훅은 실패해도 아무 동작을 하지 않습니다. 도구 호출을 막지 않습니다.

동작 확인 방법: Claude Code를 재시작한 뒤 아무 `.cpp` 파일이나 수정시키고,
파일이 여전히 CRLF인지 확인하면 됩니다.
