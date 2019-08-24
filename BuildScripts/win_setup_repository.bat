pushd %~dp0\..

mkdir Debug_x64
mkdir Debug_x86
mkdir Release_x64
mkdir Release_x86

mklink /H "Debug_x86/assimp-vc142-mtd.dll" "External\Assimp\x86\debug\assimp-vc142-mtd.dll"
mklink /H "Debug_x86/zlibd.dll" "External\Assimp\x86\debug\zlibd.dll"
mklink /H "Debug_x86/PhysX_32.dll" "External\PhysX\x86\debug\PhysX_32.dll"
mklink /H "Debug_x86/PhysXCommon_32.dll" "External\PhysX\x86\debug\PhysXCommon_32.dll"
mklink /H "Debug_x86/PhysXCooking_32.dll" "External\PhysX\x86\debug\PhysXCooking_32.dll"
mklink /H "Debug_x86/PhysXDevice.dll" "External\PhysX\x86\debug\PhysXDevice.dll"
mklink /H "Debug_x86/PhysXFoundation_32.dll" "External\PhysX\x86\debug\PhysXFoundation_32.dll"
mklink /H "Debug_x86/PhysXGpu_32.dll" "External\PhysX\x86\debug\PhysXGpu_32.dll"
mklink /J "Debug_x86/Assets" "ECSEngine\Assets"

mklink /H "Debug_x64/assimp-vc142-mtd.dll" "External\Assimp\x64\debug\assimp-vc142-mtd.dll"
mklink /H "Debug_x64/zlibd.dll" "External\Assimp\x64\debug\zlibd.dll"
mklink /H "Debug_x64/PhysX_64.dll" "External\PhysX\x64\debug\PhysX_64.dll"
mklink /H "Debug_x64/PhysXCommon_64.dll" "External\PhysX\x64\debug\PhysXCommon_64.dll"
mklink /H "Debug_x64/PhysXCooking_64.dll" "External\PhysX\x64\debug\PhysXCooking_64.dll"
mklink /H "Debug_x64/PhysXDevice64.dll" "External\PhysX\x64\debug\PhysXDevice64.dll"
mklink /H "Debug_x64/PhysXFoundation_64.dll" "External\PhysX\x64\debug\PhysXFoundation_64.dll"
mklink /H "Debug_x64/PhysXGpu_64.dll" "External\PhysX\x64\debug\PhysXGpu_64.dll"
mklink /J "Debug_x64/Assets" "ECSEngine\Assets"

mklink /H "Release_x86/assimp-vc142-mt.dll" "External\Assimp\x86\release\assimp-vc142-mt.dll"
mklink /H "Release_x86/zlib.dll" "External\Assimp\x86\release\zlib.dll"
mklink /H "Release_x86/PhysX_32.dll" "External\PhysX\x86\release\PhysX_32.dll"
mklink /H "Release_x86/PhysXCommon_32.dll" "External\PhysX\x86\release\PhysXCommon_32.dll"
mklink /H "Release_x86/PhysXCooking_32.dll" "External\PhysX\x86\release\PhysXCooking_32.dll"
mklink /H "Release_x86/PhysXDevice.dll" "External\PhysX\x86\release\PhysXDevice.dll"
mklink /H "Release_x86/PhysXFoundation_32.dll" "External\PhysX\x86\release\PhysXFoundation_32.dll"
mklink /H "Release_x86/PhysXGpu_32.dll" "External\PhysX\x86\release\PhysXGpu_32.dll"
mklink /J "Release_x86/Assets" "ECSEngine\Assets"

mklink /H "Release_x64/assimp-vc142-mt.dll" "External\Assimp\x64\release\assimp-vc142-mt.dll"
mklink /H "Release_x64/zlib.dll" "External\Assimp\x64\release\zlib.dll"
mklink /H "Release_x64/PhysX_64.dll" "External\PhysX\x64\release\PhysX_64.dll"
mklink /H "Release_x64/PhysXCommon_64.dll" "External\PhysX\x64\release\PhysXCommon_64.dll"
mklink /H "Release_x64/PhysXCooking_64.dll" "External\PhysX\x64\release\PhysXCooking_64.dll"
mklink /H "Release_x64/PhysXDevice64.dll" "External\PhysX\x64\release\PhysXDevice64.dll"
mklink /H "Release_x64/PhysXFoundation_64.dll" "External\PhysX\x64\release\PhysXFoundation_64.dll"
mklink /H "Release_x64/PhysXGpu_64.dll" "External\PhysX\x64\release\PhysXGpu_64.dll"
mklink /J "Release_x64/Assets" "ECSEngine\Assets"