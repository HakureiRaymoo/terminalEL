#Requires -Version 7
<#
.SYNOPSIS
    Display-only color/reading and visual calibration charts. Does not change settings.
.DESCRIPTION
    ANSI colors follow the selected Terminal scheme. True Color examples use
    explicit RGB and simulate syntax roles; they do not configure editor themes.
#>
param(
    [ValidateSet('Colors', 'Calibration', 'All')]
    [string]$Page = 'Colors'
)

$ErrorActionPreference = 'Stop'

$esc = [char]27
$reset = "$esc[0m"
function Paint([string]$Hex, [string]$Text) {
    $r = [Convert]::ToInt32($Hex.Substring(0, 2), 16)
    $g = [Convert]::ToInt32($Hex.Substring(2, 2), 16)
    $b = [Convert]::ToInt32($Hex.Substring(4, 2), 16)
    return "$esc[38;2;$r;$g;${b}m$Text$reset"
}
function Heading([string]$Text) { Write-Host "`n$reset=== $Text ===" }
function CodeLine([object[]]$Parts) {
    $line = foreach ($part in $Parts) { Paint $part[0] $part[1] }
    Write-Host ($line -join '')
}

try {
    if ($Page -in 'Calibration', 'All') {
        Heading 'Visual calibration / 目视校准 (not ICC or HDR calibration)'
        Write-Host 'Keep the same monitor, font size, DPI, opacity and window size for A/B.'
        Write-Host 'Start: native, Tone 1/1/0/1, Bloom intensity 0, panel grid 0.'
        Write-Host 'Use existing Appearance preview: Tone -> Bloom -> optional panel grid.'
        Write-Host 'Record original settings first. Save accepts; close without Save discards edits.'
        # Explicit black background isolates input levels from the selected scheme.
        # Blocks use background colors so font coverage cannot distort their values.
        foreach ($band in @(
            @{ Name = 'Black steps / 黑位'; Values = @(0, 2, 4, 8, 12, 16, 24, 32) },
            @{ Name = 'Gray steps / 灰阶'; Values = @(0, 32, 64, 96, 128, 160, 192, 224) },
            @{ Name = 'White steps / 亮部'; Values = @(224, 232, 240, 244, 248, 251, 253, 255) }
        )) {
            Heading $band.Name
            foreach ($v in $band.Values) {
                Write-Host ("$esc[0m$esc[38;2;160;160;160m{0,3} $esc[48;2;$v;$v;${v}m        $reset " -f $v) -NoNewline
            }
            Write-Host ''
        }
        Heading 'Dark text on black / 黑底细字'
        foreach ($v in 8, 16, 24, 32, 48, 64, 96, 128, 192, 255) {
            Write-Host ("$esc[0m$esc[48;2;0;0;0m$esc[38;2;$v;$v;${v}m{0,3}  il1| O0 [] {{}} != -> 细字 中文 Aa09$reset" -f $v)
        }
        Heading 'Channel clipping / 通道亮部区分'
        foreach ($channel in 0..2) {
            foreach ($v in 128, 160, 192, 224, 240, 248, 255) {
                $rgb = @(0, 0, 0)
                $rgb[$channel] = $v
                $r, $g, $b = $rgb
                Write-Host ("$esc[0m$esc[48;2;$r;$g;${b}m      $reset{0,3} " -f $v) -NoNewline
            }
            Write-Host ''
        }
        Heading 'Uniform patch / 网格均匀性 (inspect at normal reading size)'
        foreach ($i in 1..4) {
            Write-Host "$esc[0m$esc[48;2;128;128;128m                                                                $reset"
        }
        Heading 'Edges, counters and halo / 边缘、字腔和辉光'
        Write-Host "$esc[0m$esc[48;2;0;0;0m$esc[38;2;224;224;224m|||| ---- ____ ++++ HHHH MMMM 8888 O0O0 中文细笔画$reset"
        Write-Host ((Paint 'C8C8C8' 'Neutral  ') + (Paint 'FFF7F7' 'Near-gray  ') + (Paint 'FFD6D6' 'Pale red  ') + (Paint 'FFFF00' 'Yellow  ') + (Paint '0000FF' 'Blue'))
        Write-Host 'Compare grid 0 / 0.05 / 0.10 at rest and while scrolling. Stop if fine strokes break or shimmer.'
        Write-Host 'Hybrid deliberately recolors gray. Judge neutral ramps in native first; then compare all four modes.'
        Write-Host 'Use -Page Colors for ANSI16, syntax, selection and 256-color samples; -Page All prints both.'
        if ($Page -eq 'Calibration') { return }
    }
    Heading 'EL Amber Code / ANSI 16 colors (selected scheme)'
    $names = 'black','red','green','yellow','blue','purple','cyan','white'
    foreach ($bright in 0, 1) {
        foreach ($i in 0..7) {
            $code = 30 + $i + 60 * $bright
            $label = if ($bright) { "bright $($names[$i])" } else { $names[$i] }
            Write-Host ("$esc[${code}m{0,-14} Aa09 中文 {{ }} [] () != -> ███$reset" -f $label)
        }
    }
    Heading 'ANSI background blocks (bright backgrounds can emit glow)'
    foreach ($i in 0..15) {
        $bg = if ($i -lt 8) { 40 + $i } else { 100 + $i - 8 }
        Write-Host "$esc[${bg}m$esc[97m BG $i white $esc[30m black $reset " -NoNewline
        if ($i % 4 -eq 3) { Write-Host '' }
    }

    Heading 'True Color grayscale: yellow core / orange halo in hybrid mode'
    foreach ($v in 0, 16, 32, 64, 96, 128, 160, 192, 224, 255) {
        $hex = '{0:X2}{0:X2}{0:X2}' -f $v
        Write-Host (Paint $hex ("#{0}  灰阶 Gray 0123456789 细笔画 il1|[]" -f $hex))
    }
    Heading 'Near-neutral boundary and saturated colors (explicit RGB)'
    foreach ($hex in 'FFFFFF','FFF7F7','FFEBEB','FFD6D6','FFD16B','FFFF00','FF0000','00FF00','0000FF','00FFFF','FF00FF') {
        Write-Host (Paint $hex "#$hex  Color 彩色 ABC xyz 0123 ███")
    }
    Heading 'Adjacent neutral/orange glow vs real yellow/red/blue glow'
    Write-Host ((Paint 'C8C8C8' 'Neutral 中性 ') + (Paint 'FFFF00' 'Yellow 黄色 ') + (Paint 'F06478' 'Error 错误 ') + (Paint '639FEF' 'Blue 蓝色'))

    Heading 'Python (simulated True Color syntax)'
    Write-Host (Paint '829BBE' '# 注释：读取用户并显示结果 / read users')
    CodeLine @(@('BC87EC','def '),@('38BCCF','load_users'),@('C8C8C8','(path: '),@('639FEF','str'),@('C8C8C8',') -> '),@('639FEF','list'),@('C8C8C8',':'))
    CodeLine @(@('C8C8C8','    retries = '),@('DE983C','3'))
    CodeLine @(@('C8C8C8','    '),@('BC87EC','return '),@('38BCCF','read_json'),@('C8C8C8','(path, encoding='),@('41C997','"utf-8"'),@('C8C8C8',')'))

    Heading 'TypeScript (simulated True Color syntax)'
    CodeLine @(@('BC87EC','interface '),@('639FEF','User'),@('C8C8C8',' { name: '),@('639FEF','string'),@('C8C8C8','; active: '),@('639FEF','boolean'),@('C8C8C8','; }'))
    CodeLine @(@('BC87EC','const '),@('C8C8C8','user: '),@('639FEF','User'),@('C8C8C8',' = { name: '),@('41C997','"Alice"'),@('C8C8C8',', active: '),@('DE983C','true'),@('C8C8C8',' };'))
    CodeLine @(@('C8C8C8','console.'),@('38BCCF','log'),@('C8C8C8','(user.name);'))

    Heading 'C++ (simulated True Color syntax)'
    CodeLine @(@('BC87EC','constexpr '),@('639FEF','int'),@('C8C8C8',' count = '),@('DE983C','42'),@('C8C8C8',';'))
    CodeLine @(@('639FEF','std::vector<int>'),@('C8C8C8',' values{ '),@('DE983C','1, 2, 3'),@('C8C8C8',' };'))
    CodeLine @(@('BC87EC','for '),@('C8C8C8','('),@('BC87EC','const auto '),@('C8C8C8','value : values) { '),@('38BCCF','process'),@('C8C8C8','(value); }'))

    Heading 'PowerShell / JSON (simulated True Color syntax)'
    CodeLine @(@('C8C8C8','$items = '),@('38BCCF','Get-ChildItem'),@('C8C8C8',' -Path '),@('41C997','"./src"'),@('C8C8C8',' | '),@('38BCCF','Where-Object'),@('C8C8C8',' Length -gt '),@('DE983C','1024'))
    CodeLine @(@('C8C8C8','{ '),@('38BCCF','"theme"'),@('C8C8C8',': '),@('41C997','"EL Amber Code"'),@('C8C8C8',', '),@('38BCCF','"enabled"'),@('C8C8C8',': '),@('DE983C','true'),@('C8C8C8',' }'))

    Heading 'Diff / diagnostics (ANSI: follows selected scheme)'
    Write-Host "$esc[31m- const retries = 1;$reset"
    Write-Host "$esc[32m+ const retries = 3;$reset"
    Write-Host "$esc[34msrc/main.cpp:42:7$reset $esc[31merror: 未定义的标识符 'result'$reset"
    Write-Host "$esc[33mwarning: 返回值未使用 / unused return value$reset"
    Write-Host "$esc[36minfo: build completed in 1.2 s$reset"
    Write-Host "$esc[90mnote: secondary text / 次要说明$reset"

    Heading '256-color palette: cube 16..231 and grayscale 232..255'
    foreach ($i in 16..255) {
        Write-Host ("$esc[38;5;${i}m{0,3}██ $reset" -f $i) -NoNewline
        if (($i - 15) % 12 -eq 0) { Write-Host '' }
    }
    Write-Host ''
    Heading 'Reading / selection test'
    foreach ($i in 1..4) {
        Write-Host ("{0:D2}  普通正文 Default text: if (value != 0) {{ return items[index]; }}  il1| O0 [] {{}}" -f $i)
    }
    Write-Host 'Drag-select these lines to inspect selection colors. ANSI/RGB samples are simulated highlighting, not an editor theme.'
} finally {
    Write-Host $reset -NoNewline
}
