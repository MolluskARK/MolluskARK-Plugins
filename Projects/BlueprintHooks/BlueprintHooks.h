#pragma once

#include <API/Ark/Ark.h>

#ifdef BLUEPRINT_HOOKS_EXPORTS
#define BLUEPRINT_HOOKS_API __declspec(dllexport)
#else
#define BLUEPRINT_HOOKS_API __declspec(dllimport)
#endif

namespace BlueprintHooks
{
	// Unreal Engine types that are not defined in ArkServerApi headers
	struct FOutParmRec
	{
		UProperty*& PropertyField() { return *GetNativePointerField<UProperty**>(this, "FOutParmRec.Property"); }
		unsigned __int8*& PropAddrField() { return *GetNativePointerField<unsigned __int8**>(this, "FOutParmRec.PropAddr"); }
		FOutParmRec*& NextOutParmField() { return *GetNativePointerField<FOutParmRec**>(this, "FOutParmRec.NextOutParm"); }
	};

	struct FFrame : FOutputDevice
	{
		UFunction*& NodeField() { return *GetNativePointerField<UFunction**>(this, "FFrame.Node"); }
		UObject*& ObjectField() { return *GetNativePointerField<UObject**>(this, "FFrame.Object"); }
		unsigned __int8*& CodeField() { return *GetNativePointerField<unsigned __int8**>(this, "FFrame.Code"); }
		unsigned __int8*& LocalsField() { return *GetNativePointerField<unsigned __int8**>(this, "FFrame.Locals"); }
		UProperty*& MostRecentPropertyField() { return *GetNativePointerField<UProperty**>(this, "FFrame.MostRecentProperty"); }
		unsigned __int8*& MostRecentPropertyAddressField() { return *GetNativePointerField<unsigned __int8**>(this, "FFrame.MostRecentPropertyAddress"); }
		FFrame*& PreviousFrameField() { return *GetNativePointerField<FFrame**>(this, "FFrame.PreviousFrame"); }
		FOutParmRec*& OutParmsField() { return *GetNativePointerField<FOutParmRec**>(this, "FFrame.OutParms"); }
		UField*& PropertyChainForCompiledInField() { return *GetNativePointerField<UField**>(this, "FFrame.PropertyChainForCompiledIn"); }
		UFunction*& CurrentNativeFunctionField() { return *GetNativePointerField<UFunction**>(this, "FFrame.CurrentNativeFunction"); }
	};

	// BlueprintHooks types
	typedef void (*BlueprintHookFuncPtr)(UObject*, FFrame*);

	// Exported functions
	BLUEPRINT_HOOKS_API bool SetBlueprintPreHook(UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction);
	BLUEPRINT_HOOKS_API bool SetBlueprintPostHook(UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction);
	BLUEPRINT_HOOKS_API bool DisableBlueprintPreHook(UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction);
	BLUEPRINT_HOOKS_API bool DisableBlueprintPostHook(UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction);

	// Blueprint function input/output/local variable accessors
	template<typename T>
	T* GetFunctionLocalVar(FFrame* stack, const FString& name)
	{
		if (!stack)
			throw std::invalid_argument("null stack");
		if (!stack->NodeField())
			throw std::invalid_argument("null stack Node");

		UProperty* property = nullptr;
		for (UProperty* currProperty = stack->NodeField()->PropertyLinkField(); currProperty; currProperty = currProperty->PropertyLinkNextField())
		{
			if ((currProperty->PropertyFlagsField() & 0x001) && currProperty->NameField().ToString().Equals(name))
			{
				property = currProperty;
				break;
			}
		}
		if (!property)
			return nullptr;
		if (sizeof(T) != property->ElementSizeField())
			throw std::invalid_argument("Unexpected property size.");

		return (T*)(stack->LocalsField() + property->Offset_InternalField());
	}

	template<typename T>
	T* GetFunctionInput(FFrame* stack, const FString& name)
	{
		if (!stack)
			throw std::invalid_argument("null stack");
		if (!stack->NodeField())
			throw std::invalid_argument("null stack Node");

		UProperty* property = nullptr;
		for (UProperty* currProperty = stack->NodeField()->PropertyLinkField(); currProperty; currProperty = currProperty->PropertyLinkNextField())
		{
			if ((currProperty->PropertyFlagsField() & 0x080) && !(currProperty->PropertyFlagsField() & 0x100) && currProperty->NameField().ToString().Equals(name))
			{
				property = currProperty;
				break;
			}
		}
		if (!property)
			return nullptr;
		if (sizeof(T) != property->ElementSizeField())
			throw std::invalid_argument("Unexpected property size.");

		return (T*)(stack->LocalsField() + property->Offset_InternalField());
	}

	template<typename T>
	T* GetFunctionOutput(FFrame* stack, const FString& name)
	{
		if (!stack)
			throw std::invalid_argument("null stack");
		if (!stack->NodeField())
			throw std::invalid_argument("null stack Node");

		int outParmCount = 0;
		FOutParmRec * outParm = nullptr;
		FOutParmRec * currOutParm = stack->OutParmsField();
		for (UProperty* currProperty = stack->NodeField()->PropertyLinkField(); currProperty; currProperty = currProperty->PropertyLinkNextField())
			if ((currProperty->PropertyFlagsField() & 0x080) && (currProperty->PropertyFlagsField() & 0x100))
				outParmCount++;
		for (int i = 0; currOutParm && (i < outParmCount); ++i)
		{
			if (currOutParm->PropertyField() && currOutParm->PropertyField()->NameField().ToString().Equals(name))
			{
				outParm = currOutParm;
				break;
			}
			currOutParm = currOutParm->NextOutParmField();
		}
		if (!outParm)
			return nullptr;
		if (sizeof(T) != outParm->PropertyField()->ElementSizeField())
			throw std::invalid_argument("Unexpected property size.");

		return (T*)(outParm->PropAddrField());
	}
}