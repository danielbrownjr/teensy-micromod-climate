param(
  [ValidateSet('Setup','Build','Flash','Monitor')][string]$Action='Build',
  [ValidateSet('bringup','io','display','full')][string]$Stage='full',
  [string]$Port='', [int]$Seconds=15, [string]$Commands='s',
  [ValidatePattern('^[A-Z]$')][string]$Drive='U'
)
$ErrorActionPreference='Stop'
# Short paths are required by the Windows ARM GCC multilib header search.
$workspace=[IO.Path]::GetFullPath($PSScriptRoot)
$mappedRoot="${Drive}:\"
$existing=(& subst | Where-Object { $_ -like "${Drive}:*" })
if (Test-Path $mappedRoot) {
  if (!$existing -or (($existing -replace '^.*?=>\s*','').TrimEnd('\') -ne $workspace.TrimEnd('\'))) {
    throw "$Drive`: is already used by another volume. Choose -Drive with an unused letter."
  }
} else { & subst "${Drive}:" $workspace; if($LASTEXITCODE) { throw 'Drive mapping failed' } }
$work=Join-Path $mappedRoot '.work'
$project=$mappedRoot
$cli=Join-Path $work 'arduino-cli/arduino-cli.exe'
$config=Join-Path $work 'arduino-cli.yaml'
$fqbn='teensy:avr:teensyMM:usb=serial,speed=600,opt=o2std'
function Invoke-Cli([string[]]$CliArgs) {
  & $cli --config-file $config @CliArgs
  if($LASTEXITCODE) { throw "Arduino CLI failed ($LASTEXITCODE)" }
}
if($Action -eq 'Setup') {
  New-Item -ItemType Directory -Force $work | Out-Null
  if(!(Test-Path $cli)) {
    $release='https://github.com/arduino/arduino-cli/releases/download/v1.5.1'
    $asset='arduino-cli_1.5.1_Windows_64bit.zip'
    Invoke-WebRequest "$release/$asset" -OutFile "$work/$asset"
    Invoke-WebRequest "$release/1.5.1-checksums.txt" -OutFile "$work/cli-checksums.txt"
    $expected=((Get-Content "$work/cli-checksums.txt" | Where-Object { $_ -match "\s+$([regex]::Escape($asset))$" }) -split '\s+')[0]
    if(!$expected -or (Get-FileHash "$work/$asset" -Algorithm SHA256).Hash -ne $expected) { throw 'CLI checksum mismatch' }
    Expand-Archive "$work/$asset" "$work/arduino-cli"
  }
  if(!(Test-Path $config)) { & $cli config init --dest-dir $work; if($LASTEXITCODE) {throw 'Config init failed'} }
  Invoke-Cli @('config','set','directories.data',"$work/arduino-data")
  Invoke-Cli @('config','set','directories.user',"$work/arduino-user")
  Invoke-Cli @('config','set','board_manager.additional_urls','https://www.pjrc.com/teensy/package_teensy_index.json')
  Invoke-Cli @('core','update-index')
  Invoke-Cli @('core','install','teensy:avr@1.62.0')
  exit
}
if(!(Test-Path $cli)) { throw 'Run .\Tool.ps1 Setup first.' }
# Refresh paths if a different drive letter was selected.
Invoke-Cli @('config','set','directories.data',"$work/arduino-data")
Invoke-Cli @('config','set','directories.user',"$work/arduino-user")
$build=Join-Path $work "build-$Stage"
if($Action -eq 'Build') {
  $sketch=if($Stage -eq 'bringup'){'Bringup'}else{'Playground'}
  $argsList=@('compile','--fqbn',$fqbn,'--warnings','all','--build-path',$build)
  if($Stage -ne 'bringup') {
    $level=@{io=1;display=2;full=3}[$Stage]
    # TEENSYDUINO=160 is the upstream 1.62.0 board definition, not a version override.
    $argsList+=@('--build-property',"build.flags.defs=-D__IMXRT1062__ -DTEENSYDUINO=160 -DPLAYGROUND_STAGE=$level")
  }
  $argsList+=Join-Path $project $sketch
  Invoke-Cli $argsList
  $dest=Join-Path $project "firmware/$Stage"
  New-Item -ItemType Directory -Force $dest | Out-Null
  Copy-Item "$build/$sketch.ino.hex" $dest
} elseif($Action -eq 'Flash') {
  if(!$Port) { Invoke-Cli @('board','list'); throw 'Specify -Port using the Teensy usb: path shown above.' }
  if(!(Test-Path $build)) {throw "Build $Stage first."}
  Invoke-Cli @('upload','--fqbn',$fqbn,'--port',$Port,'--input-dir',$build)
} elseif($Action -eq 'Monitor') {
  if(!$Port) {
    $found=@(Get-CimInstance Win32_PnPEntity | Where-Object { $_.PNPDeviceID -match '^USB\\VID_16C0&PID_0483&MI_00' -and $_.Name -match '\(COM\d+\)' })
    if($found.Count -ne 1) {throw 'Specify -Port COMn; expected exactly one Teensy serial device.'}
    $Port=[regex]::Match($found[0].Name,'COM\d+').Value
  }
  $serial=[IO.Ports.SerialPort]::new($Port,115200)
  try {
    $serial.Open(); $serial.Write($Commands)
    $until=[DateTime]::UtcNow.AddSeconds($Seconds)
    while([DateTime]::UtcNow -lt $until) {
      Start-Sleep -Milliseconds 100
      $text=$serial.ReadExisting(); if($text) {Write-Host $text -NoNewline}
    }
  } finally {$serial.Dispose()}
}
