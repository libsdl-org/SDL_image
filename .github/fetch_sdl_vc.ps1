$ErrorActionPreference = "Stop"

$project_root = "$psScriptRoot\.."
Write-Output "project_root: $project_root"

$sdl3_version = "3.0.0"
$sdl3_zip = "SDL3-devel-$($sdl3_version)-VC.zip"

$sdl3_url = "https://github.com/libsdl-org/SDL/releases/download/release-$($sdl3_version)/SDL3-devel-$($sdl3_version)-VC.zip"
$sdl3_dlpath = "$($Env:TEMP)\$sdl3_zip"

$sdl3_bindir = "$($project_root)"
$sdl3_extractdir = "$($sdl3_bindir)\SDL3-$($sdl3_version)"
$sdl3_root_name = "SDL3-devel-VC"

echo "sdl3_bindir:     $sdl3_bindir"
echo "sdl3_extractdir: $sdl3_extractdir"
echo "sdl3_root_name:  $sdl3_root_name"

echo "Cleaning previous artifacts"
if (Test-Path $sdl3_extractdir) {
    Remove-Item $sdl3_extractdir -Recurse -Force
}
if (Test-Path "$($sdl3_bindir)/$sdl3_root_name") {
    Remove-Item "$($sdl3_bindir)/$sdl3_root_name" -Recurse -Force
}
if (Test-Path $sdl3_dlpath) {
    Remove-Item $sdl3_dlpath -Force
}

Write-Output "Downloading $sdl3_url"
Invoke-WebRequest -Uri $sdl3_url -OutFile $sdl3_dlpath

Write-Output "Extracting archive"
Expand-Archive $sdl3_dlpath -DestinationPath $sdl3_bindir

Write-Output "Setting up SDL3 folder"
Rename-Item $sdl3_extractdir $sdl3_root_name

Write-Output "Done"
