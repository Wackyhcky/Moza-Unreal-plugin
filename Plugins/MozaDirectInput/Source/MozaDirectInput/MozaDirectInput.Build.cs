using UnrealBuildTool;
using System.IO;

public class MozaDirectInput : ModuleRules
{
    public MozaDirectInput(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "InputDevice",
            "Slate",
            "SlateCore",
            "ApplicationCore"
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "dinput8.lib",
                "dxguid.lib"
            });

            // Prefer UE-bundled DirectX headers if present; otherwise rely on Windows SDK.
            string DirectXInclude = Path.Combine(EngineDirectory, "Source", "ThirdParty", "Windows", "DirectX", "include");
            if (Directory.Exists(DirectXInclude))
            {
                PublicSystemIncludePaths.Add(DirectXInclude);
            }

            
        }
    }
}