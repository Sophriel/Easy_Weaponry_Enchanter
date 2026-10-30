# ============================================================
# FileChanged 훅 — <프로젝트 루트>/plan_request.md
#
#   이 파일은 "항상 존재하는 상태 파일"이다.
#     IDLE      -> 처리할 것 없음
#     그 외 내용 -> 검증 요청 (플랜 본문)
#
#   처리하면 내용을 IDLE 로 되돌린다. 그 쓰기가 훅을 다시 부르지만
#   내용이 IDLE 이므로 즉시 exit 0 으로 끝난다 = 루프 없음.
#
#   exit 0 = 조용히 무시  /  exit 2 = 세션을 깨움(asyncRewake)
# ============================================================

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

try { $payload = ([Console]::In.ReadToEnd()) | ConvertFrom-Json } catch { $payload = $null }
$cwd = if ($payload -and $payload.cwd) { $payload.cwd } else { (Get-Location).Path }

$dir     = Join-Path $cwd '.claude\plan-verify'
$inbox   = Join-Path $dir 'inbox'
$request = Join-Path $cwd 'plan_request.md'
$state   = Join-Path $dir '.state.json'
$log     = Join-Path $dir 'hook.log'
$stamp   = Get-Date -Format 'yyyyMMdd-HHmmss'

# 발화 여부 자체를 남긴다. 진단용.
# PowerShell 5.1 의 Get/Set-Content 는 인코딩을 지정하지 않으면 CP949(한글 ANSI)로
# 처리해서 UTF-8 한글을 모지바케로 만든다. .NET API 로 UTF-8(BOM 없음)을 강제한다.
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)

function Log($m) {

    [System.IO.File]::AppendAllText($log, "[$stamp][FileChanged] $m`r`n", $Utf8NoBom)

}

New-Item -ItemType Directory -Force -Path $inbox | Out-Null
if (-not (Test-Path -LiteralPath $request)) { Log "fired, but no request file"; exit 0 }

$body = [System.IO.File]::ReadAllText($request, [Text.Encoding]::UTF8)
if ($null -eq $body) { $body = '' }

# --- IDLE 이면 할 일 없음 (되돌린 쓰기로 인한 재발화가 여기서 죽는다) ---
if ($body.Trim() -eq 'IDLE' -or $body.Trim() -eq '') { Log "fired, IDLE"; exit 0 }

# --- 루프 방지: 내용 해시 중복 + 10초 쿨다운 ---
$sha  = [System.Security.Cryptography.SHA256]::Create()
$hash = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($body))).Replace('-','')
$now  = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()

if (Test-Path -LiteralPath $state) {
    try {
        $prev = [System.IO.File]::ReadAllText($state, [Text.Encoding]::UTF8) | ConvertFrom-Json
        if ($prev.hash -eq $hash) { Log "fired, duplicate hash"; exit 0 }
        if (($now - [int64]$prev.time) -lt 10000) { Log "fired, cooldown"; exit 0 }
    } catch { }
}

[System.IO.File]::WriteAllText($state, (@{ hash = $hash; time = $now } | ConvertTo-Json -Compress), $Utf8NoBom)

# --- 요청을 inbox 로 옮기고 투입구를 IDLE 로 되돌린다 ---
$target = Join-Path $inbox "plan-$stamp.md"

# 저장소 정책상 작업 트리는 CRLF 다. Cowork 가 LF 로 보내와도 정규화한다.
$normalized = ($body -replace "`r`n", "`n") -replace "`r", "`n" -replace "`n", "`r`n"
[System.IO.File]::WriteAllText($target, $normalized, $Utf8NoBom)
[System.IO.File]::WriteAllText($request, "IDLE`r`n", $Utf8NoBom)

$rel = ".claude/plan-verify/inbox/plan-$stamp.md"
Log "ACCEPTED -> $rel"
$msg = @"

[plan-verifier] 새 플랜 검증 요청이 도착했습니다: $rel
Task 툴로 plan-verifier 서브에이전트를 띄워 이 플랜을 검증하세요.
서브에이전트에게 넘길 프롬프트: "$rel 파일의 플랜을 검증하라."

주의: 서브에이전트는 .claude/plan-verify/ 아래에 아무것도 쓰면 안 됩니다.
개정판 플랜은 Documents/MDs/plans/ 에만 작성합니다.
"@

[Console]::Error.WriteLine($msg)
exit 2