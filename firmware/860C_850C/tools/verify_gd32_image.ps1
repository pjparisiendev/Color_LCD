param(
  [string]$Elf = "$PSScriptRoot\..\src\main.elf",
  [string]$Bin = "$PSScriptRoot\..\src\main.bin"
)

$ErrorActionPreference = 'Stop'
$toolBin = 'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1\bin'
$objdump = Join-Path $toolBin 'arm-none-eabi-objdump.exe'
$nm = Join-Path $toolBin 'arm-none-eabi-nm.exe'

if (!(Test-Path -LiteralPath $Elf) -or !(Test-Path -LiteralPath $Bin)) {
  throw 'Build artifacts are missing. Build APT_850C_GD32F303RET6 first.'
}

$sections = & $objdump -h $Elf
$symbols = & $nm -n $Elf
$vector = $sections | Select-String '^\s*0\s+\.isr_vector\s+\S+\s+08004000\s+08004000'
$stack = $symbols | Select-String '^20010000\s+A\s+_estack$'
$settingsStart = $symbols | Select-String '^0807f000\s+A\s+_settings_flash_start$'
$settingsEnd = $symbols | Select-String '^08080000\s+A\s+_settings_flash_end$'
$runtime = $symbols | Select-String '\sT\s+bafang_runtime_init$'
$bafangWhitelist = $symbols | Select-String '\sT\s+usart1_send_bafang_read$'
$size = (Get-Item -LiteralPath $Bin).Length

if (!$vector) { throw '.isr_vector is not at 0x08004000.' }
if (!$stack) { throw '_estack is not 0x20010000.' }
if (!$settingsStart -or !$settingsEnd) { throw 'Settings-page linker reservation is missing.' }
if (!$runtime) { throw 'Bafang runtime is absent.' }
if (!$bafangWhitelist) { throw 'Bafang read-only TX whitelist is absent.' }
if ($size -gt 503808) { throw "Application is too large for the 0x08004000-0x0807EFFF region: $size bytes." }

$hash = (Get-FileHash -LiteralPath $Bin -Algorithm SHA256).Hash
Write-Output "PASS vector=0x08004000 stack=0x20010000 settings=0x0807F000-0x0807FFFF size=$size sha256=$hash"
