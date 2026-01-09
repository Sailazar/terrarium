# Build Script for Raylib 3D Grid Project
# PowerShell version

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Raylib 3D Grid Project - Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if test.cpp exists
if (-Not (Test-Path "test.cpp")) {
    Write-Host "ERROR: test.cpp not found!" -ForegroundColor Red
    Write-Host "Make sure you're in the project root directory." -ForegroundColor Yellow
    exit 1
}

# Try to find g++ compiler
$compilerPaths = @(
    "C:\w64devkit\bin\g++.exe",
    "C:\mingw64\bin\g++.exe",
    "C:\msys64\mingw64\bin\g++.exe",
    "C:\TDM-GCC-64\bin\g++.exe",
    "g++"  # Try system PATH
)

$compiler = $null
foreach ($path in $compilerPaths) {
    if ($path -eq "g++") {
        # Check if g++ is in PATH
        try {
            $null = Get-Command g++ -ErrorAction Stop
            $compiler = "g++"
            Write-Host "Found compiler in system PATH: g++" -ForegroundColor Green
            break
        } catch {
            continue
        }
    } elseif (Test-Path $path) {
        $compiler = $path
        Write-Host "Found compiler at: $path" -ForegroundColor Green
        break
    }
}

if ($null -eq $compiler) {
    Write-Host ""
    Write-Host "ERROR: g++ compiler not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install one of the following:" -ForegroundColor Yellow
    Write-Host "  - w64devkit (https://github.com/skeeto/w64devkit)" -ForegroundColor Yellow
    Write-Host "  - MinGW-w64 (https://www.mingw-w64.org/)" -ForegroundColor Yellow
    Write-Host "  - MSYS2 (https://www.msys2.org/)" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Or add your existing compiler to the system PATH." -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

# Build command
Write-Host ""
Write-Host "Compiling test.cpp..." -ForegroundColor Cyan

$buildCommand = @(
    $compiler,
    "test.cpp",
    "-o", "test.exe",
    "-I./include",
    "-L./lib",
    "-lraylib",
    "-lopengl32",
    "-lgdi32",
    "-lwinmm"
)

Write-Host "Command: $($buildCommand -join ' ')" -ForegroundColor Gray
Write-Host ""

# Execute build
try {
    & $buildCommand[0] $buildCommand[1..($buildCommand.Length-1)]

    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "  BUILD SUCCESSFUL!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        Write-Host "Executable created: test.exe" -ForegroundColor Green
        Write-Host ""
        Write-Host "To run the application:" -ForegroundColor Cyan
        Write-Host "  .\test.exe" -ForegroundColor White
        Write-Host ""
        Write-Host "New feature: Press [8] for Scale Sphere Mode!" -ForegroundColor Yellow
        Write-Host ""
        exit 0
    } else {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Red
        Write-Host "  BUILD FAILED!" -ForegroundColor Red
        Write-Host "========================================" -ForegroundColor Red
        Write-Host ""
        Write-Host "Check the error messages above." -ForegroundColor Yellow
        Write-Host ""
        exit 1
    }
} catch {
    Write-Host ""
    Write-Host "ERROR: Compilation failed!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ""
    exit 1
}
