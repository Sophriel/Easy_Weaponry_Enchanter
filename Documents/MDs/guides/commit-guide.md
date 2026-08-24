# 커밋 가이드

> Claude / Claude Code가 이 저장소에 커밋할 때 **반드시** 따르는 절차.
> 커밋을 요청받으면 작업 전에 이 문서를 먼저 읽을 것.

---

## 0. 원칙

| 항목 | 규칙 |
|------|------|
| 커밋 메시지 | **영어 전용 — 한글 절대 금지** |
| 저자 (author/committer) | `Claude Opus 5 <noreply@anthropic.com>` |
| Co-Authored-By | `Lee Jae Hyeong <nsa24604@naver.com>` |
| 분할 단위 | 논리적 변경 1개 = 커밋 1개 |
| push | **절대 자동으로 하지 않음.** 사용자가 직접 수행 |

`git push`는 `.claude/settings.local.json`에서 deny 규칙으로 차단되어 있다.
우회하지 말 것.

> 저자 이름은 커밋을 수행한 모델명을 포함한다 (`Claude Opus 5`).
> 기존 이력의 `Co-Authored-By: Claude Opus 4.6` 표기와 같은 컨벤션이다.
> 모델이 바뀌면 그 시점 모델명을 쓴다.

---

## 1. 커밋 전 점검

커밋을 만들기 전에 아래를 확인한다. 하나라도 걸리면 **커밋하지 말고 먼저 보고**할 것.

### 1-0. 검사 대상 정하기 — 먼저 읽을 것

아래 검사들이 **무엇을 들여다봐야 하는지는 커밋 방식에 따라 달라진다.**
`git commit <pathspec>` 은 인덱스가 아니라 **워크트리**를 커밋하기 때문이다 (§4-3).

| 커밋 방식 | 검사 대상 diff |
|-----------|----------------|
| `git commit` (pathspec 없음) | `git diff --cached` |
| `git commit <paths>` (pathspec) | `git diff HEAD -- <paths>` |

**커밋 방식을 먼저 정하고, 그에 맞는 명령으로 검사한다.**
pathspec 커밋을 하면서 `--cached` 로 검사하면
**검사는 통과하고 문제는 그대로 커밋된다.** 실제로 그렇게 사고가 났다.

### 1-1. 코드에 한글이 없는가

```bash
# pathspec 없는 커밋
git diff --cached -U0 | grep -nP '^\+.*[\x{AC00}-\x{D7A3}\x{3131}-\x{318E}]'

# pathspec 커밋
git diff HEAD -U0 -- <paths> | grep -nP '^\+.*[\x{AC00}-\x{D7A3}\x{3131}-\x{318E}]'
```

`.md` 파일은 한국어 허용이므로 결과에서 제외한다.
`.cpp` / `.h` / `.cs` / `.ini` 에 한글이 잡히면 영어로 고친 뒤 커밋한다.

### 1-1b. `@@@` 마커가 남아 있지 않은가

```bash
# pathspec 없는 커밋
git diff --cached | grep -n "^+.*@@@"

# pathspec 커밋
git diff HEAD -- <paths> | grep -n "^+.*@@@"
```

마커가 잡히면 커밋을 중단하고 사용자에게 보고한다. 임의로 지우지 않는다.

### 1-2. 디버그 잔여물이 없는가

- 임시 `UE_LOG` / 람다 로거
- `TestGE` 같은 실험용 `UPROPERTY`
- 주석 처리된 실험 코드
- `BP_*_Test` 류 임시 에셋

### 1-3. 빌드가 통과하는가

C++ 변경이 포함되면 `CLAUDE.md`의 "빌드 검증" 절차를 먼저 수행한다.
빌드 실패 상태로 커밋하지 않는다.

### 1-4. 줄바꿈이 오염되지 않았는가

`.gitattributes`가 저장소에 LF로 정규화하므로 보통 문제없다.
다만 `git diff --stat`에서 **변경하지 않은 파일이 통째로 재작성된 것처럼**
보이면 EOL 오염이다. 이 경우 실제 변경만 남기고 확인한다.

```bash
# 실제 내용이 바뀐 파일만 추리기
git diff HEAD --ignore-all-space --ignore-blank-lines --stat
```

---

## 2. 분할 규칙

한 커밋에 성격이 다른 변경을 섞지 않는다.

| 구분 | 예시 |
|------|------|
| `feat` | 새 GA / AT / Function 추가 |
| `fix` | 버그 수정, 초기화 순서 수정 |
| `refactor` | 동작 변화 없는 구조 변경 |
| `chore` | 빌드 설정, `.gitattributes`, 도구 설정 |
| `docs` | `Documents/MDs/**`, `CLAUDE.md` |

특히 아래는 **반드시 분리**한다.

- 줄바꿈/포맷 정규화 → 기능 변경과 절대 섞지 않는다
- 플러그인(`Plugins/EasyWeaponryEnchanter`) 변경 → 게임 모듈(`Source/EWE`) 변경과 분리
- `.uasset` 바이너리 → 관련 C++ 변경과 함께 묶되, 무관한 에셋은 제외

---

## 3. 메시지 형식

```
<type>: <subject, 영어 소문자 시작, 마침표 없음>

<본문 — 72자 줄바꿈. 무엇을 왜 바꿨는지. 어떻게는 코드가 말한다.>

<필요 시 두 번째 문단>

Co-Authored-By: Lee Jae Hyeong <nsa24604@naver.com>
```

규칙:

- subject는 명령형 현재시제 (`add`, `fix`, `move` — `added`/`adds` 아님)
- 본문 각 줄 **72자 이내**로 수동 줄바꿈
- 문단 사이 빈 줄 1개
- trailer 앞에 **빈 줄 1개 필수** (없으면 git이 trailer로 인식하지 않음)

### 예시

```
fix(gas): move Health and Mana clamping to PostGameplayEffectExecute

Clamping resource attributes in PreAttributeChange raced with DefaultGE
initialization: MaxHealth was still 0 when Health was first applied, so
the clamp forced Health to 0 permanently.

PreAttributeChange now guards only the Max attributes. All resource
clamping happens after GE execution, where Max values are settled.

Co-Authored-By: Lee Jae Hyeong <nsa24604@naver.com>
```

---

## 4. 실행 절차

### 4-1. 락 파일 정리 (Cowork 환경 필수)

Cowork 샌드박스는 `.git` 내부 파일의 **`unlink`(삭제)만 차단**한다.
생성·덮어쓰기·rename은 허용된다.

git은 인덱스와 ref를 갱신할 때 `*.lock` 파일을 만들고 마지막에 삭제하는데,
이 삭제가 실패해 락이 남는다. 남은 락은 다음 git 명령을 전부 차단한다.

> `fatal: Unable to create '.git/index.lock': File exists.`

**삭제 대신 rename으로 치운다.** 모든 git 쓰기 명령 앞에 실행할 것.

```bash
for l in $(find .git -maxdepth 2 -name '*.lock' 2>/dev/null); do
  mv "$l" "${l%.lock}.stale.$(date +%s%N)" 2>/dev/null
done
```

`warning: unable to unlink '.git/objects/../tmp_obj_*'` 경고는 무시해도 된다.
객체는 정상 기록된 상태이며, 사용자가 나중에 `git prune`으로 정리한다.

### 4-2. 종료 코드를 정확히 읽을 것

`git ... | head` 처럼 파이프를 걸면 `$?`가 **파이프 마지막 명령의 종료 코드**가
되어 git의 실패를 놓친다. 반드시 아래 형태로 확인한다.

```bash
out=$(git add <paths> 2>&1); rc=$?
echo "$out"; echo "rc=$rc"
```

### 4-3. 부분 커밋 — 두 방식과 각각의 함정

다른 변경이 이미 staged인 상태에서 일부만 커밋하는 방법은 두 가지다.
**파일 단위냐 hunk 단위냐로 갈린다. 잘못 고르면 조용히 오염된 커밋이 만들어진다.**

#### 파일 단위 → pathspec 커밋

특정 파일 전체를 **워크트리 상태 그대로** 커밋할 때 쓴다.
`git reset`으로 스테이지를 비우지 않아도 된다.

```bash
git add <새 파일들>                    # untracked 파일만 add
git commit <paths> -m "..." -m "..."   # 해당 경로만 커밋
```

`<paths>` 에 없는 경로의 스테이징은 그대로 보존된다.

> ⚠️ **pathspec 커밋은 그 경로의 인덱스를 무시하고 워크트리를 커밋한다.**
> git 의 기본 동작(`--only`)이다. 따라서
> - 검사는 반드시 `git diff HEAD -- <paths>` 로 한다 (§1-0)
> - 그 경로에 아직 커밋하면 안 되는 변경이 섞여 있으면 이 방식을 쓰지 않는다

#### hunk 단위 → 인덱스 커밋 (pathspec 금지)

한 파일 안에서 일부 hunk 만 골라 커밋할 때 쓴다.

```bash
git apply --cached <patch>        # 또는 git add -p
git diff --cached                 # 인덱스에 의도한 것만 있는지 눈으로 확인
git diff --cached --name-only     # 무관한 파일이 섞이지 않았는지 확인
git commit -m "..."               # pathspec 없이. 인덱스 그대로 커밋된다
```

> ⚠️ 여기서 `git commit <paths>` 를 쓰면 **정밀 선택이 통째로 무효화된다.**
> 워크트리 버전이 인덱스를 덮어써서, 제외했던 hunk(`@@@` 마커 등)까지 커밋된다.

> ⚠️ 반대 방향 함정 — pathspec 없는 커밋은 **인덱스 전체**를 커밋한다.
> 무관한 파일이 staged 되어 있으면 그것까지 딸려 들어간다.
> 불필요한 것은 `git restore --staged <path>` 로 내린 뒤 커밋한다.

#### 잘못 커밋했을 때

```bash
git reset --soft HEAD~1
```

> ⚠️ **`--soft` 로 되돌려도 hunk 선택은 복구되지 않는다.**
> pathspec 커밋은 커밋 시점에 그 경로의 인덱스를 워크트리 내용으로 덮어쓴다.
> 되돌린 직후 인덱스에는 **오염된 버전이 그대로 남아 있다.**
> `git apply --cached` / `git add -p` 로 **인덱스를 처음부터 다시 구성**한 뒤
> pathspec 없는 `git commit` 으로 커밋할 것.

`git reset --hard` 는 쓰지 않는다 (§7).

### 4-4. 저자 지정

샌드박스에는 git identity가 설정되어 있지 않다. 매번 `-c`로 넘긴다.

```bash
git -c user.name="Claude Opus 5" -c user.email="noreply@anthropic.com" \
    commit <paths> -F <message-file>
```

---

## 5. 커밋 후 검증

```bash
git --no-optional-locks log --format='%h  %an <%ae>  |  %s' -3
git --no-optional-locks show -s --format='%B' HEAD
git --no-optional-locks log -1 --format='%(trailers:key=Co-Authored-By,valueonly)' HEAD
git --no-optional-locks status --short
```

확인 항목:

- author / committer가 `Claude Opus 5 <noreply@anthropic.com>` 인가
- trailer가 정상 인식되는가 (빈 줄 누락 시 인식 안 됨)
- 의도하지 않은 파일이 딸려 들어가지 않았는가
- 남은 변경이 그대로 보존되어 있는가
- **커밋본 실물에 `@@@` 마커나 한글이 없는가** — diff 가 아니라 커밋된 파일을 직접 본다

```bash
git --no-optional-locks show HEAD -- <path> | grep -n "^+.*@@@"
git --no-optional-locks show HEAD:<path> | grep -n "@@@"
```

diff 검사(§1)를 통과했어도 커밋 방식을 잘못 골랐으면 실물에는 남아 있을 수 있다.
이 확인이 마지막 안전망이다.

`git status` 같은 조회 명령도 인덱스를 갱신하며 락을 만들 수 있다.
읽기 전용 조회에는 `--no-optional-locks`를 붙인다.

---

## 6. 커밋 재작성

이미 만든 커밋의 저자나 메시지를 고쳐야 할 때, `rebase`는 락을 많이 만들어
Cowork 환경에서 실패하기 쉽다. `commit-tree`로 재구성하고 ref를 직접 쓴다.

```bash
BASE=$(git --no-optional-locks rev-parse HEAD~2)
T1=$(git --no-optional-locks rev-parse 'HEAD~1^{tree}')
T2=$(git --no-optional-locks rev-parse 'HEAD^{tree}')

export GIT_AUTHOR_NAME="Claude Opus 5" GIT_AUTHOR_EMAIL="noreply@anthropic.com"
export GIT_COMMITTER_NAME="Claude Opus 5" GIT_COMMITTER_EMAIL="noreply@anthropic.com"

C1=$(git commit-tree "$T1" -p "$BASE" < msg1.txt)
C2=$(git commit-tree "$T2" -p "$C1"  < msg2.txt)

printf '%s\n' "$C2" > .git/refs/heads/main   # unlink 불필요
```

재작성 후 **트리 해시가 동일한지 반드시 확인**한다. 내용이 바뀌면 안 된다.

```bash
git --no-optional-locks rev-parse <이전HEAD>^{tree}
git --no-optional-locks rev-parse HEAD^{tree}
```

⚠️ 이미 push된 커밋은 재작성하지 않는다. push 여부를 먼저 확인할 것.

---

## 7. 하지 않을 것

- `git push` — 차단되어 있고, 사용자가 직접 한다
- `git commit --no-verify` — 훅 우회 금지
- `git reset --hard` — 작업 손실 위험. 필요하면 사용자에게 확인받는다
- 커밋 메시지에 한글
- 한 커밋에 정규화 + 기능 변경 혼합
- 빌드 실패 상태 커밋
