using UnrealBuildTool;
using System.Collections.Generic;

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

        PrivateDependencyModuleNames.AddRange(new string[] { });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new string[]
            {
                "dinput8.lib",
                "dxguid.lib"
            });
            PublicIncludePaths.Add("$(ThirdPartySourceDir)/Windows/DirectX/include");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Microsoft.Dx11");
        }
    }
}
