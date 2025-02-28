﻿// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SWindow.h"

class UBlueprint;

struct FMGADataTableWindowArgs
{
	FText Title = NSLOCTEXT("IMGAEditorModule", "DefaultWindowTitle", "Create DataTable from Attribute Set BP");
	FVector2d Size = FVector2D(1280, 720);
	ESizingRule SizingRule = ESizingRule::UserSized;
	bool bSupportsMinimize = true;
	bool bSupportsMaximize = true;
	bool bIsModal = false;

	FMGADataTableWindowArgs() = default;
};

/** Module interface for scaffold module */
class IMGAEditorModule : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though. Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static IMGAEditorModule& Get()
	{
		static const FName ModuleName = "ModularGameplayAbilitiesEditor";
		return FModuleManager::LoadModuleChecked<IMGAEditorModule>(ModuleName);
	}

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() during shutdown if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		static const FName ModuleName = "ModularGameplayAbilitiesEditor";
		return FModuleManager::Get().IsModuleLoaded(ModuleName);
	}

	/**
	 *	Preloads any AttributeSets Blueprint assets to ensure Effect and GameplayAttribute details customization can list all of them.
	 *
	 *	Without this, only BP Attributes that were previously opened (and loaded into memory) or right-clicked in content browser would show up.
	 */
	virtual void PreloadAssetsByClass(UClass* InClass) const = 0;

	/** Creates and returns a new window widget to create a FAttributeMetaData DataTable asset from a given ModularAttributeSet. */
	virtual TSharedRef<SWindow> CreateDataTableWindow(const TWeakObjectPtr<UBlueprint>& InBlueprint, const FMGADataTableWindowArgs& InArgs) const = 0;
};
