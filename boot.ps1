# PowerShell script to create bootable floppy and launch QEMU

$projectPath = "C:\Users\DeLL\Desktop\zebx"
$buildPath = "$projectPath\build"
$floppyPath = "$buildPath\boot.img"

# Create a 1.44MB floppy disk image (filled with zeros)
Write-Host "[*] Creating 1.44MB floppy disk image..."
$floppySize = 1474560  # 1.44MB in bytes
$zeros = New-Object byte[] $floppySize
[System.IO.File]::WriteAllBytes($floppyPath, $zeros)
Write-Host "[+] Created: $floppyPath"

# Copy kernel binary to floppy
if (Test-Path "$buildPath\kernel.bin") {
    Write-Host "[*] Copying kernel.bin to floppy..."
    $kernelData = [System.IO.File]::ReadAllBytes("$buildPath\kernel.bin")
    
    # Write kernel data at offset 0 (boot sector)
    $fs = [System.IO.File]::Open($floppyPath, [System.IO.FileMode]::Open)
    $fs.Write($kernelData, 0, $kernelData.Length)
    $fs.Close()
    
    Write-Host "[+] Kernel copied to floppy"
} else {
    Write-Host "[!] kernel.bin not found!"
    exit 1
}

# Boot in QEMU
Write-Host ""
Write-Host "[*] Launching ZEBX OS in QEMU..."
Write-Host ""

& "C:\Program Files\qemu\qemu-system-i386.exe" -fda $floppyPath -m 256 -name "ZEBX OS"

Write-Host ""
Write-Host "[*] QEMU closed"
