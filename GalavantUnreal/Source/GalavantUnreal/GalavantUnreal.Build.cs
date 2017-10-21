// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;			// Console
using System.IO; 		// Path

public class GalavantUnreal : ModuleRules
{
	public GalavantUnreal(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules) 
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		// For GeneratedMeshComponent
		PrivateDependencyModuleNames.AddRange(new string[] { "RHI", "RenderCore", "ShaderCore" });

		// CustomMeshComponent, for voxel testing
		PrivateDependencyModuleNames.AddRange(new string[] {"CustomMeshComponent"});
		PrivateIncludePathModuleNames.AddRange(new string[] {"CustomMeshComponent"});

		// Make debug game builds work
		// See https://answers.unrealengine.com/questions/607072/unreal-4161-debug-game-build-error-icu-data-direct.html
		// This isn't needed if you package first!
		//Definitions.Add("UE_ENGINE_DIRECTORY=/home/macoy/Development/code/3rdParty/repositories/UnrealEngine/Engine/");
		//Definitions.Add("UE_ENGINE_DIRECTORY=/home/macoy/Development/code/repositories/galavant-unreal/GalavantUnreal/Saved/Cooked/LinuxNoEditor/Engine/");


		////////////////////////
		// External libraries //
		////////////////////////

		// Directory the file you're reading right now is in
		var current_directory = ModuleDirectory;

		// Includes and libraries relative to current_directory
		var includes = new string[] {"../../ThirdParty/galavant/src",
			"../../ThirdParty/galavant/src/thirdPartyWrapper",
			"../../ThirdParty/polyvox/library/PolyVoxCore/include",
			"../../ThirdParty/polyvox/library/PolyVoxUtil/include"};

		var libs = new string[] {"../../ThirdParty/galavant/lib/libGalavant.a",
			"../../ThirdParty/galavant/lib/thirdPartyWrapper/libGalaThirdPartyWrapper.a",
			"../../ThirdParty/galavant/lib/libGalaUtil.a",
			"../../ThirdParty/galavant/lib/libGalaEntityComponent.a",
			"../../ThirdParty/galavant/lib/libGalaAi.a",
			"../../ThirdParty/galavant/lib/libGalaWorld.a",
			"../../ThirdParty/galavant/lib/libGalaGame.a",
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
