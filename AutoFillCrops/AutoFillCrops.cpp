#include <filesystem>
#include "AutoFillCrops.h"
#include "ReadFile.h"

UClass* MyBPLoadClass(FString* blueprintPath)
{
	static std::map<FString, UClass*> loadedClasses;
	if (loadedClasses.size() > 0)
	{
		auto it = loadedClasses.begin();
		auto end = loadedClasses.end();
		while (it != end)
		{
			if ((*it).first.Compare(*blueprintPath) == 0)
				return (*it).second;
			it++;
		}
	}
	UClass* toAdd = UVictoryCore::BPLoadClass(blueprintPath);
	if (toAdd != nullptr)
		loadedClasses[*blueprintPath] = toAdd;
	return toAdd;
}

FString LoadConfigFileContent()
{
	FString fileContent = L"";
	try
	{
		std::string configPath = "./ArkApi/Plugins/AutoFillCrops/Config.txt";
		if (std::filesystem::exists(configPath))
		{
			std::string content = ReadFile(configPath);
			if (content.empty() || content.compare("\r\n") == 0 || content.compare("\n") == 0)
				return fileContent;
			fileContent = FString(content);
			fileContent.ReplaceInline(L"\r\n", L"\n");
		}
		else
			Log::GetLog()->error("ERROR: Failed to load AutoFillCrops configuration file. File not found.");
	}
	catch (const std::exception&)
	{
		fileContent = L"";
		Log::GetLog()->error("ERROR: Failed to load AutoFillCrops configuration file. Please verify its content.");
	}
	return fileContent;
}

float GetFillCropsRadiusConfig()
{
	// Read config file content.
	FString fileContent = LoadConfigFileContent();
	if (!fileContent.IsEmpty())
	{
		std::string item;
		std::stringstream ss(fileContent.ToString());
		while (std::getline(ss, item, '\n')) {
			if (!item.empty() && item.size() > 0)
			{
				try
				{
					int radiusInt = std::atoi(item.c_str());
					if (radiusInt > 0 && radiusInt <= 21600)
					{
						//std::string toLog = "INFO: AutoFillCrops radius is set to [" + std::to_string(radiusInt) + "].";
						return (float)radiusInt;
					}
				}
				catch (const std::exception&) { }
			}
		}
	}
	// Defaults to 18000 (36 foundations) if we reach here.
	Log::GetLog()->info("WARNING: Failed to parse radius from the config file for AutoFillCrops. Using default value of 10800.");
	return 10800.0F;
}

float GetFillCropsRadius()
{
	static float _radius = -1.0F;

	if (_radius > 0.0F)
		return _radius;
	else
	{
		_radius = GetFillCropsRadiusConfig();
		return _radius;
	}
}
