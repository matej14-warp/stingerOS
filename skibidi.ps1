# define the file extensions to clean
$extensions = "*.c", "*.h", "*.s", "*.asm", "*.py", "Makefile"

foreach ($ext in $extensions) {
    Get-ChildItem -Recurse -Filter $ext | ForEach-Object {
        $content = Get-Content $_.FullName -Raw
        
        # 1. remove multi-line comments /* ... */ 
        # the (?s) allows the dot to match newlines
        $content = $content -replace '(?s)/\*.*?\*/', ''
        
        # 2. remove hash comments # ...
        # matches # and everything following it on that line
        $content = $content -replace '#.*', ''
        
        # trim trailing whitespace left behind by deleted comments
        $content = $content.Split("`n") | ForEach-Object { $_.TrimEnd() } | Out-String

        # overwrite the file
        $content | Set-Content $_.FullName
        Write-Host "purged /* and # comments from: $($_.Name)" -ForegroundColor Green
    }
}