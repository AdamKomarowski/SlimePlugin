using UnrealBuildTool;
using System.IO;

public class SlimePlugin : ModuleRules
{

    public SlimePlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivatePCHHeaderFile = "Public/SlimePluginPrivatePCH.h";

        PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "InputCore", "ProceduralMeshComponent" });
    }
}