# Claude Code 설정 — EWE

> 🌐 [English version](README.md)

이 폴더는 git으로 추적되어 팀 전체가 동일한 Claude Code 설정을 공유합니다.
**첫 세션을 시작하기 전에 아래 설정 안내를 반드시 읽어주세요.**

---

## ⚠️ 머신별 절대 경로 — 반드시 수정해야 합니다

일부 권한 규칙에는 **작성자 컴퓨터 기준의 절대 경로**가 들어 있습니다.
Claude Code는 `Bash(...)` / `PowerShell(...)` 규칙을 **명령 문자열 글롭**으로
매칭합니다. 경로를 해석하거나 정규화하지 않기 때문에, 경로가 내 설치 위치와
정확히 일치할 때만 규칙이 작동합니다.

언리얼 엔진이 다른 위치에 설치되어 있다면 작업 시작 전에
`.claude/settings.local.json`의 아래 항목을 수정하세요.

| 규칙 | 변경할 내용 |
| :--- | :--- |
| `Bash("C:\Program Files\Epic Games\UE_5.5\...\Build.bat" EWEEditor *)` | 내 엔진 설치 경로 |
| `Bash("C:\Program Files\Epic Games\UE_5.5\...\Build.bat" EWE *)` | 내 엔진 설치 경로 |
| `PowerShell(& "C:\Program Files\Epic Games\UE_5.5\...\Build.bat" *)` | 내 엔진 설치 경로 |
| `Read(//c/Program Files/Epic Games/UE_5.5/**)` | 내 엔진 설치 경로 (POSIX 정규화 형식) |

`CLAUDE.md`의 "빌드 검증" 섹션에 있는 빌드 명령어도 함께 수정해야 합니다.

수정하지 않아도 **망가지지는 않습니다.** 다만 빌드 명령마다 승인 창이 뜨고,
자동으로 실행되지 않을 뿐입니다.

### 경로 문법 요약

`Read` / `Edit` 규칙은 gitignore 패턴 문법을 쓰며, 앵커가 4종류입니다.

| 패턴 | 기준점 |
| :--- | :--- |
| `//path` | 파일시스템 루트 (진짜 절대 경로) |
| `~/path` | 홈 디렉토리 |
| `/path` | 설정 파일의 소스 위치 — `settings.local.json`은 **Claude Code를 실행한 디렉토리** |
| `path`, `./path` | 현재 디렉토리 |

함정 두 가지:

- 슬래시 하나는 **절대 경로가 아닙니다.** `/Source/**`는 `<실행 디렉토리>/Source/**`를
  의미합니다. 진짜 절대 경로는 `//`로 시작해야 합니다.
- Windows에서는 매칭 전에 POSIX 형식으로 정규화됩니다.
  `C:\Program Files` → `/c/Program Files`

`/path`가 실행 디렉토리를 기준으로 하므로, **항상 저장소 루트에서 Claude Code를
실행하세요.** 하위 폴더에서 실행하면 `Read(/Source/**)` 같은 규칙이 엉뚱한 곳을
가리킵니다.

---

## 폴더 구성

| 파일 | 추적 | 용도 |
| :--- | :---: | :--- |
| `settings.json` | O | 공유 프로젝트 설정. 현재 `PostToolUse` 훅을 담고 있음 |
| `settings.local.json` | O | 권한 규칙. "Yes, don't ask again" 선택 시 Claude Code가 자동으로 추가함 |
| `hooks/normalize-eol.ps1` | O | `Write` / `Edit` 직후 파일을 CRLF로 재작성 |
| `skills/` | O | 프로젝트 전용 스킬 |

> **참고:** JSON은 주석을 지원하지 않고 Claude Code는 JSONC도 받지 않습니다.
> 따라서 설정 파일은 엄격한 JSON을 유지해야 하며, 모든 설명은 이 README에
> 대신 기록합니다.

---

## 권한 규칙 — 구획별 설명

규칙은 **deny → ask → allow** 순으로 평가됩니다. 먼저 일치한 규칙이 결과를
결정하며, 규칙이 얼마나 구체적인지는 순서에 영향을 주지 않습니다.
즉 **일치하는 deny는 더 구체적인 allow를 항상 이깁니다.**

### allow — 웹 접근

```
WebSearch
WebFetch(domain:dev.epicgames.com)
WebFetch(domain:forums.unrealengine.com)
WebFetch(domain:code.claude.com)
```

언리얼 엔진은 문서화가 부족한 영역이 많아 공식 문서와 커뮤니티 포럼 참조가
자주 필요합니다. 전체 `WebFetch` 허용 대신 도메인 단위로 제한했습니다.

### allow — IDE 연동

```
mcp__ide__getDiagnostics
```

전체 리빌드 없이 연결된 IDE에서 컴파일 진단 정보를 바로 가져옵니다.

### allow — 읽기

```
Read(//c/Program Files/Epic Games/UE_5.5/**)   엔진 소스 및 헤더 (머신별 경로)
Read(/Source/**)                                게임 모듈
Read(/Plugins/**)                               EasyWeaponryEnchanter 플러그인
Read(/Config/**)                                DefaultEngine.ini, DefaultGame.ini 등
Read(/Documents/**)                             CLAUDE.md가 참조하는 설계 문서
```

엔진 규칙은 생각보다 중요합니다. GAS와 Enhanced Input은 엔진 헤더를 직접
읽어야만 답이 나오는 경우가 많습니다.

### allow — 편집

```
Edit(/Source/**)
Edit(/Plugins/**)
Edit(/Documents/**)
```

`Edit` 규칙은 `Write`, `MultiEdit`을 포함한 모든 파일 편집 도구를 커버합니다.
대신 `Write(...)` 나 `MultiEdit(...)` 경로 규칙을 쓰면 **아무 효과가 없습니다.**
Claude Code가 규칙을 받아들이긴 하지만 참조하지 않고, 시작 시 경고를 냅니다.

`Config/**`는 의도적으로 **읽기만 허용**했습니다. `DefaultEngine.ini`를 잘못
수정하면 프로젝트가 아예 안 열릴 수 있습니다.

### allow — 읽기 전용 git

```
Bash(git diff:*)
Bash(git status:*)
Bash(git log:*)
Bash(git show:*)
Bash(git add:*)
```

조회와 스테이징만 허용합니다. `git commit`은 **일부러 제외**했습니다.
커밋 메시지는 항상 사람이 검토해야 합니다 — `CLAUDE.md`의 "커밋 메시지 —
영어 전용" 규칙 참조.

### allow — 빌드 검증

```
Bash("C:\Program Files\...\Build.bat" EWEEditor *)
Bash("C:\Program Files\...\Build.bat" EWE *)
PowerShell(& "C:\Program Files\...\Build.bat" *)
```

`CLAUDE.md`는 C++ 변경마다 빌드를 요구합니다. 이 규칙 덕분에 반복 빌드에서
매번 승인 창이 뜨지 않습니다. **머신별 경로 — 이 문서 최상단 참조.**

### deny — 비밀 정보

```
Read(//**/.env)
Read(~/.ssh/**)
```

프로젝트 범위가 아니라 **파일시스템 전역** 차단입니다.
`Read` deny 규칙은 같은 경로에 대한 `Edit` / `Write` 도 함께 막습니다.

### deny — 푸시 방지

```
Bash(git push:*)
```

deny는 모든 스코프에서 allow를 이기므로 `bypassPermissions` 모드에서도
유지됩니다. 푸시는 직접 하세요.

### deny — 생성물 디렉토리

```
Edit(/Binaries/**)
Edit(/Intermediate/**)
Edit(/DerivedDataCache/**)
Edit(/Saved/**)
```

빌드 산출물입니다. 편집해봐야 의미가 없고 빌드를 깨뜨릴 수 있습니다.

---

## 줄바꿈 정책

`.gitattributes`가 저장소에는 LF로 저장하고 작업 트리에는 CRLF로 체크아웃합니다.
추가로 `PostToolUse` 훅이 Claude가 건드린 파일을 CRLF로 재작성해서, 디스크
상태가 Visual Studio / Rider가 기대하는 형식과 일치하도록 맞춥니다.

훅은 실패해도 아무 동작을 하지 않습니다. 도구 호출을 막지 않습니다.

동작 확인 방법: Claude Code를 재시작한 뒤 아무 `.cpp` 파일이나 수정시키고,
파일이 여전히 CRLF인지 확인하면 됩니다.

---

## 제거된 규칙

기존 `settings.local.json`에는 누적된 규칙이 36개 있었습니다. 아래 항목을
제거했습니다.

- **일회성 탐색 명령이 규칙으로 굳어진 것** — `Bash(find '...' -name *Heal* -type f)`,
  `Bash(grep -r GetSubsystem '...')` 등. 정확히 한 문자열만 매칭되므로 다시
  발동될 일이 없습니다.
- **명령이 아닌 조각** — `Bash(grep -E '\.sh$|\.bat$|\.cmd$')`,
  `Bash(Select-Object -First 1 -ExpandProperty FullName)`.
- **도구가 잘못된 규칙** — `Bash(Get-ChildItem -Path "...")` 는 PowerShell
  cmdlet인데 Bash 규칙으로 등록되어 있었습니다.
- **존재하지 않는 대상** — `Bash(unreal-build-tool.exe ...)`,
  `Bash(./BuildAndRun.sh)`.
- **형식이 깨진 규칙** — `Bash(\"c:\Users\...\EWE.uproject)` (따옴표 깨짐),
  `Read(//d C:/Users/...)` (`//d ` 접두사 오류 + 정규화되지 않은 `C:/` 드라이브
  문자 → 매칭 자체가 불가능).
- **무제한 셸 접근** — `Bash(powershell:*)`, `Bash(cmd:*)`, `Bash(cmd.exe:*)`.
  임의 명령 실행을 허용해서 나머지 Bash 규칙을 전부 무의미하게 만들었습니다.
  위의 빌드 전용 규칙으로 대체했습니다.
