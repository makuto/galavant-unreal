// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;			// Console
using System.IO; 		// Path

public class GalavantUnreal : ModuleRules
{
	public GalavantUnreal(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		// For GeneratedMeshComponent
		PrivateDependencyModuleNames.AddRange(new string[] { "RHI", "RenderCore", "ShaderCore" });

		// CustomMeshComponent, for voxel testing
		PrivateDependencyModuleNames.AddRange(new string[] {"CustomMeshComponent"});
		PrivateIncludePathModuleNames.AddRange(new string[] {"CustomMeshComponent"});

		////////////////////////
		// External libraries //
		////////////////////////

		// Directory the file you're reading right now is in
		var current_directory = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));

		// Includes and libraries relative to current_directory
		var includes = new string[] {"../../ThirdParty/galavant/src",
			"../../ThirdParty/galavant/src/thirdPartyWrapper",
			"../../ThirdParty/polyvox/library/PolyVoxCore/include",
			"../../ThirdParty/polyvox/library/PolyVoxUtil/include"};

		var libs = new string[] {"../../ThirdParty/galavant/lib/libGalavant.a",
			"../../ThirdParty/galavant/lib/thirdPartyWrapper/libGalaThirdPartyWrapper.a",
			"../../ThirdParty/galavant/lib/libGalaEntityComponent.a",
			"../../ThirdParty/polyvox/build/library/PolyVoxCore/libPolyVoxCore",
			"../../ThirdParty/polyvox/build/library/PolyVoxUtil/libPolyVoxUtil"};

		foreach (var include in includes)
		{
			Console.WriteLine("Include: {0}", include);
			PublicIncludePaths.Add(Path.Combine(current_directory, include));
		}

		foreach (var lib in libs)
		{
			Console.WriteLine("Lib: {0}", lib);
			PublicAdditionalLibraries.Add(Path.Combine(current_directory, lib));
		}
	}
}
