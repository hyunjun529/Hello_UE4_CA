// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Hello_UE4_CAEditorTarget : TargetRules
{
	public Hello_UE4_CAEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		ExtraModuleNames.Add("Hello_UE4_CA");
	}
}
