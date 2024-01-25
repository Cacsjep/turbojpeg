rm out/*.jpg
$env:CGO_LDFLAGS="-LC:\libjpeg-turbo64\lib"
$env:CGO_CPPFLAGS="-IC:\libjpeg-turbo64\include"
$env:LD_LIBRARY_PATH="C:\libjpeg-turbo64\lib"
go test
