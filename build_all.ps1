$imageName = "milunx-all"
$hostDirectory = "./dist"  
$dockerFilePath = "BuildAll.dockerfile"  

if (-not (Test-Path -Path $hostDirectory)) {
    New-Item -ItemType Directory -Path $hostDirectory
}

Write-Host "Building Docker image..."
docker build --progress=plain -t $imageName -f $dockerFilePath .
Write-Host "Running container to copy the MilunxClient binary..."
docker run --rm -v "$(Resolve-Path $hostDirectory):/export" $imageName /bin/sh -c 'cp -r /opt/libjpeg/* /export/'
Write-Host "Milunxs binaries should now be in $hostDirectory"