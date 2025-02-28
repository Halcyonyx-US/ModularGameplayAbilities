﻿// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UAttributeSet;
class UAbilitySystemComponent;

class FMGAUtilities final
{
public:
	/** Strips out any trailing "_C" from Blueprint class names */
	static MODULARGAMEPLAYABILITIES_API FString GetAttributeClassName(const UClass* Class);

	/** Strips out any trailing "_C" from Blueprint class names */
	static MODULARGAMEPLAYABILITIES_API FString GetAttributeClassName(FString ClassName);

	/** In editor, this will filter out properties with meta tag "HideInDetailsView" or equal to InFilterMetaStr. In non editor, it returns all properties */
	static MODULARGAMEPLAYABILITIES_API void GetAllAttributeProperties(TArray<FProperty*>& OutProperties, FString InFilterMetaStr = FString(), bool bInUseEditorOnlyData = true);

	/** Returns all properties in the given class that are valid Gameplay Attributes properties */
	static MODULARGAMEPLAYABILITIES_API void GetAllAttributeFromClass(const UClass* InClass, TArray<FProperty*>& OutProperties, FString InFilterMetaStr = FString(), bool bInUseEditorOnlyData = true);

	/** Checks whether the attribute set class has to be considered to generate dropdown (filters out SKEL / REINST BP Class Generated By) */
	static MODULARGAMEPLAYABILITIES_API bool IsValidAttributeClass(const UClass* Class);

	/**
	 * Returns whether given property "CPPType" is a valid type to consider for attributes
	 *
	 * Restricting to only FGameplayAttributeData and child of it (that we know of)
	 */
	static MODULARGAMEPLAYABILITIES_API bool IsValidCPPType(const FString& InCPPType);

	/**
	 * Serialize helper for AttributeSets as a static for reuse.
	 *
	 * UAttributeSets that wish to implement serialization for save game (on saving / loading) can choose to use this
	 * method to serialize all of their FGameplayAttributes marked for SaveGame (with SaveGame UPROPERTY) into the
	 * Archive on Save, and read out of the Archive on Load by calling this method with Serialize().
	 *
	 * Usage:
	 * 
	 * ```cpp
	 * void UMyAttributeSet::Serialize(FArchive& Ar)
	 * {
	 * 	Super::Serialize(Ar);
	 *
	 * 	if (Ar.IsSaveGame())
	 * 	{
	 * 		FMGAUtilities::SerializeAttributeSet(this, Ar);
	 * 	}
	 * }
	 * ```
	 */
	static MODULARGAMEPLAYABILITIES_API void SerializeAttributeSet(UAttributeSet* InAttributeSet, FArchive& InArchive);

	/**
	 * Serialize helper for AttributeSets as a static for reuse.
	 *
	 * Simply goes through each Spawned AttributeSet and calls Serialize on each AttributeSet.
	 * 
	 * Usage:
	 * 
	 * ```cpp
	 * void UMyAbilitySystemComponent::Serialize(FArchive& Ar)
	 * {
	 * 	Super::Serialize(Ar);
	 *
	 * 	if (Ar.IsSaveGame())
	 * 	{
	 * 		FMGAUtilities::SerializeAbilitySystemComponentAttributes(this, Ar);
	 * 	}
	 * }
	 * ```
	 */
	static MODULARGAMEPLAYABILITIES_API void SerializeAbilitySystemComponentAttributes(const UAbilitySystemComponent* InASC, FArchive& InArchive);
};
