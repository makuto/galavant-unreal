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

		// // TODO: mmadson: get rid of this bullshit, put it into simple libs and srcs array and iterate
		// // Galavant
		// {
		// 	var galavant_src_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/galavant/src"));
		// 	var galavant_lib_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/galavant/lib/libGalavant.a"));
			
		// 	Console.WriteLine("Galavant Src: {0}\n\tLib: {1}", galavant_src_path, galavant_lib_path);
			
		// 	PublicIncludePaths.Add(galavant_src_path);
		// 	PublicAdditionalLibraries.Add(galavant_lib_path);
		// }

		// // Galavant third party wrapper
		// {
		// 	var gala_thirdPartyWrapper_src_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/galavant/src/thirdPartyWrapper"));
		// 	var gala_thirdPartyWrapper_lib_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/galavant/lib/thirdPartyWrapper/libGalaThirdPartyWrapper.a"));
			
		// 	Console.WriteLine("Galavant ThirdPartyWrapper Src: {0}\n\tLib: {1}", 
		// 		gala_thirdPartyWrapper_src_path, gala_thirdPartyWrapper_lib_path);
			
		// 	PublicIncludePaths.Add(gala_thirdPartyWrapper_src_path);
		// 	PublicAdditionalLibraries.Add(gala_thirdPartyWrapper_lib_path);
		// }

		// // PolyVox Voxel Library
		// {
		// 	var polyvox_core_src_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/polyvox/library/PolyVoxCore/include"));
		// 	var polyvox_core_lib_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/polyvox/build/library/PolyVoxCore/libPolyVoxCore"));
		// 	var polyvox_util_src_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/polyvox/library/PolyVoxUtil/include"));
		// 	var polyvox_util_lib_path = Path.GetFullPath(Path.Combine(current_directory, 
		// 		"../../ThirdParty/polyvox/build/library/PolyVoxUtil/libPolyVoxUtil"));
			
		// 	Console.WriteLine("PolyVox Core Src: {0}\n\tLib: {1}", polyvox_core_src_path, polyvox_core_lib_path);
		// 	Console.WriteLine("PolyVox Util Src: {0}\n\tLib: {1}", polyvox_util_src_path, polyvox_util_lib_path);
			
		// 	PublicIncludePaths.Add(polyvox_core_src_path);
		// 	PublicAdditionalLibraries.Add(polyvox_core_lib_path);
		// 	PublicIncludePaths.Add(polyvox_util_src_path);
		// 	PublicAdditionalLibraries.Add(polyvox_util_lib_path);
		// }
	}
}
