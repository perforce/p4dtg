New-Item -ItemType Directory -Force Release\plugins
New-Item -ItemType Directory -Force Release\repl
New-Item -ItemType Directory -Force Release\help
New-Item -ItemType Directory -Force Release\config
New-Item -ItemType Directory -Force Release\log

Copy-Item -Force -Destination Release\plugins -Path ..\..\sdk\mysql\Release\mysql5.dll
Copy-Item -Force -Destination Release\plugins -Path ..\..\sdk\p4jobdt\Release\p4jobdt.dll


ls Release\plugins