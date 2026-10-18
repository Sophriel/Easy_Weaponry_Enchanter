# Normalize line endings to CRLF after Claude Code writes or edits a file.
# Wired up as a PostToolUse hook in .claude/settings.json.
# Hook payload arrives on stdin as JSON containing tool_input.file_path.

# Force UTF-8 output so Korean error messages are not mangled by the CP949 console codepage.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ErrorActionPreference = 'Stop'

try {
    $raw = [Console]::In.ReadToEnd()
    if ([string]::IsNullOrWhiteSpace($raw)) { exit 0 }

    $payload = $raw | ConvertFrom-Json
    $path = $payload.tool_input.file_path
    if (-not $path) { exit 0 }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { exit 0 }

    # Text formats only. Never touch Unreal binary assets.
    if ($path -notmatch '\.(cpp|h|hpp|inl|cs|usf|ush|uproject|uplugin|ini|json|xml|md|txt|bat|cmd|ps1)$') {
        exit 0
    }

    $text = [System.IO.File]::ReadAllText($path)

    # Collapse to LF first, then expand to CRLF. Handles mixed input safely.
    $normalized = $text -replace "`r`n", "`n" -replace "`r", "`n" -replace "`n", "`r`n"

    if ($normalized -ne $text) {
        # UTF8 without BOM - Unreal and MSVC both expect this.
        $encoding = New-Object System.Text.UTF8Encoding($false)
        [System.IO.File]::WriteAllText($path, $normalized, $encoding)
    }
}
catch {
    # Never block Claude Code on a normalization failure.
    Write-Error "normalize-eol: $($_.Exception.Message)"
    exit 0
}

exit 0
