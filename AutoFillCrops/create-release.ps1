$publishfolder = "publish\"
$path = "publish\AutoFillCrops"

New-Item -ItemType Directory -Force -Path $path
Copy-Item bin-x64\Release\AutoFillCrops.dll $path
Copy-Item bin-x64\Release\AutoFillCrops.pdb $path
Copy-Item Configs\Config.txt $path
Copy-Item Configs\PdbConfig.json $path
Copy-Item Configs\PluginInfo.json $path

$compress = @{
  Path = $path, "README.txt"
  CompressionLevel = "Optimal"
  DestinationPath = "AutoFillCrops-1.5.zip"
}
Compress-Archive @compress

Remove-Item $publishfolder -Force -Recurse -ErrorAction SilentlyContinue