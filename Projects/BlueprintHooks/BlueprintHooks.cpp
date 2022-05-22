#include "API/ARK/Ark.h"
#include "BlueprintHooks.h"
#include "json.hpp"

#include <fstream>
#include <unordered_map>

#pragma comment(lib, "ArkApi.lib")

#define BLUEPRINT_HOOK_USCRIPT_SIZE    12 // EX_FinalFunction + 8 address bytes + EX_Return + EX_Nothing + EX_EndOfScript

namespace BlueprintHooks
{
	typedef void (*FNativeFuncPtr)(UObject*, FFrame*, void* const);

	struct UFunction : UStruct
	{
		unsigned __int32& FunctionFlagsField() { return *GetNativePointerField<unsigned __int32*>(this, "UFunction.FunctionFlags"); }
		unsigned __int16& RepOffsetField() { return *GetNativePointerField<unsigned __int16*>(this, "UFunction.RepOffset"); }
		unsigned __int8& NumParmsField() { return *GetNativePointerField<unsigned __int8*>(this, "UFunction.NumParms"); }
		unsigned __int16& ParmsSizeField() { return *GetNativePointerField<unsigned __int16*>(this, "UFunction.ParmsSize"); }
		unsigned __int16& ReturnValueOffsetField() { return *GetNativePointerField<unsigned __int16*>(this, "UFunction.ReturnValueOffset"); }
		unsigned __int16& RPCIdField() { return *GetNativePointerField<unsigned __int16*>(this, "UFunction.RPCId"); }
		unsigned __int16& RPCResponseIdField() { return *GetNativePointerField<unsigned __int16*>(this, "UFunction.RPCResponseId"); }
		UProperty*& FirstPropertyToInitField() { return *GetNativePointerField<UProperty**>(this, "UFunction.FirstPropertyToInit"); }
		FNativeFuncPtr& FuncField() { return *GetNativePointerField<FNativeFuncPtr*>(this, "UFunction.Func"); }

		static UClass* GetPrivateStaticClass() { return NativeCall<UClass*>(nullptr, "UFunction.GetPrivateStaticClass"); }
	};

	UFunction* dispatcherUFunction = nullptr;
	std::unordered_map<::UFunction*, std::vector<BlueprintHookFuncPtr>> preHooks, postHooks;

	void BlueprintHookDispatcher(UObject* obj, FFrame* stack, void* const result)
	{
		unsigned char instructions[BLUEPRINT_HOOK_USCRIPT_SIZE];
		unsigned char* savedCode;

		Log::GetLog()->debug("Dispatcher called - " + stack->NodeField()->NameField().ToString().ToString());

		// Execute pre-hooks
		for (BlueprintHookFuncPtr preHook : preHooks[reinterpret_cast<::UFunction*>(stack->NodeField())])
			preHook(obj, stack);

		// Remove our hook dispatcher script from the original blueprint script
		for (int i = 0; i < BLUEPRINT_HOOK_USCRIPT_SIZE; ++i)
		{
			instructions[i] = stack->NodeField()->ScriptField().GetData()[0];
			stack->NodeField()->ScriptField().RemoveAt(0);
		}

		savedCode = stack->CodeField();
		stack->CodeField() = stack->NodeField()->ScriptField().GetData();

		// Process the original script with the current stack
		obj->ProcessInternal(stack, result);

		// Re-insert our hook dispatcher script
		for (int i = 0; i < BLUEPRINT_HOOK_USCRIPT_SIZE; ++i)
			stack->NodeField()->ScriptField().Insert(instructions[i], i);

		stack->CodeField() = savedCode;

		// Execute post-hooks
		for (BlueprintHookFuncPtr postHook : postHooks[reinterpret_cast<::UFunction*>(stack->NodeField())])
			postHook(obj, stack);
	}

	bool InstallDispatcherHook(::UFunction* blueprintFunction)
	{
		// Insert a new script to call our hook dispatcher into the original UFunction.
		// Keep the original script in the same 'Script' TArray so that we can execute it later through the Stack's 'Node' UFunction.
		unsigned char instructions[BLUEPRINT_HOOK_USCRIPT_SIZE] = {
			static_cast<unsigned char>(EExprToken::EX_FinalFunction),
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 0 & 0xFF,
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 8 & 0xFF,
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 16 & 0xFF,
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 24 & 0xFF,
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 32 & 0xFF,
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 40 & 0xFF,
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 48 & 0xFF,
			reinterpret_cast<DWORD64>(dispatcherUFunction) >> 56 & 0xFF,
			static_cast<unsigned char>(EExprToken::EX_Return),
			static_cast<unsigned char>(EExprToken::EX_Nothing),
			static_cast<unsigned char>(EExprToken::EX_EndOfScript)
		};
		for (int i = 0; i < BLUEPRINT_HOOK_USCRIPT_SIZE; ++i)
			blueprintFunction->ScriptField().Insert(instructions[i], i);

		Log::GetLog()->debug("Dispatcher installed - " + blueprintFunction->NameField().ToString().ToString());
		return true;
	}

	bool RemoveDispatcherHook(::UFunction* blueprintFunction)
	{
		if (blueprintFunction->ScriptField().Num() < BLUEPRINT_HOOK_USCRIPT_SIZE)
			return false;

		for (int i = 0; i < BLUEPRINT_HOOK_USCRIPT_SIZE; ++i)
			blueprintFunction->ScriptField().RemoveAt(0);

		Log::GetLog()->debug("Dispatcher removed - " + blueprintFunction->NameField().ToString().ToString());
		return true;
	}

	bool SetBlueprintPreHook(::UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction)
	{
		if (!blueprintFunction)
		{
			Log::GetLog()->error("SetBlueprintPreHook() - null blueprintFunction");
			return false;
		}
		if (!hookFunction)
		{
			Log::GetLog()->error("SetBlueprintPreHook() - null hookFunction");
			return false;
		}
		if (blueprintFunction->ScriptField().Num() == 0)
		{
			Log::GetLog()->error("SetBlueprintPreHook() - blueprintFunction is not a blueprint script function");
			return false;
		}

		// If this is the first hook being set on the blueprint function, install the dispatcher hook
		if (preHooks[blueprintFunction].empty() && postHooks[blueprintFunction].empty())
		{
			if (!InstallDispatcherHook(blueprintFunction))
			{
				Log::GetLog()->error("SetBlueprintPreHook() - Failed to install dispatcher hook");
				return false;
			}
		}
		// If pre-hooks have already been set, make sure the hook doesn't already exist
		else if (!preHooks[blueprintFunction].empty())
		{
			for (BlueprintHookFuncPtr hook : preHooks[blueprintFunction])
			{
				if (hook == hookFunction)
				{
					Log::GetLog()->error("SetBlueprintPreHook() - Hook already exists");
					return false;
				}
			}
		}

		// Add the hook
		preHooks[blueprintFunction].push_back(hookFunction);

		Log::GetLog()->debug("Pre-hook set - " + blueprintFunction->NameField().ToString().ToString() +
			" (" + std::to_string(preHooks[blueprintFunction].size()) + " pre-hooks, " + std::to_string(postHooks[blueprintFunction].size()) + " post-hooks)");
		return true;
	}

	bool SetBlueprintPostHook(::UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction)
	{
		if (!blueprintFunction)
		{
			Log::GetLog()->error("SetBlueprintPostHook() - null blueprintFunction");
			return false;
		}
		if (!hookFunction)
		{
			Log::GetLog()->error("SetBlueprintPostHook() - null hookFunction");
			return false;
		}
		if (blueprintFunction->ScriptField().Num() == 0)
		{
			Log::GetLog()->error("SetBlueprintPostHook() - blueprintFunction is not a blueprint script function");
			return false;
		}

		// If this is the first hook being set on the blueprint function, install the dispatcher hook
		if (preHooks[blueprintFunction].empty() && postHooks[blueprintFunction].empty())
		{
			if (!InstallDispatcherHook(blueprintFunction))
			{
				Log::GetLog()->error("SetBlueprintPostHook() - Failed to install dispatcher hook");
				return false;
			}
		}
		// If post-hooks have already been set, make sure the hook doesn't already exist
		else if (!postHooks[blueprintFunction].empty())
		{
			// Make sure the hook doesn't already exist
			for (BlueprintHookFuncPtr hook : postHooks[blueprintFunction])
			{
				if (hook == hookFunction)
				{
					Log::GetLog()->error("SetBlueprintPostHook() - Hook already exists");
					return false;
				}
			}
		}

		// Add the hook
		postHooks[blueprintFunction].push_back(hookFunction);

		Log::GetLog()->debug("Post-hook set - " + blueprintFunction->NameField().ToString().ToString() +
			" (" + std::to_string(preHooks[blueprintFunction].size()) + " pre-hooks, " + std::to_string(postHooks[blueprintFunction].size()) + " post-hooks)");
		return true;
	}

	bool DisableBlueprintPreHook(::UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction)
	{
		if (!blueprintFunction)
		{
			Log::GetLog()->error("DisableBlueprintPreHook() - null blueprintFunction");
			return false;
		}
		if (!hookFunction)
		{
			Log::GetLog()->error("DisableBlueprintPreHook() - null hookFunction");
			return false;
		}

		// Remove the hook
		const auto iter = std::remove(preHooks[blueprintFunction].begin(), preHooks[blueprintFunction].end(), hookFunction);
		if (iter == preHooks[blueprintFunction].end())
		{
			Log::GetLog()->error("DisableBlueprintPreHook() - Hook not found");
			return false;
		}
		preHooks[blueprintFunction].erase(iter, preHooks[blueprintFunction].end());

		// If there are no remaining hooks set on the blueprint function, remove the dispatcher hook
		if (preHooks[blueprintFunction].empty() && postHooks[blueprintFunction].empty())
		{
			if (!RemoveDispatcherHook(blueprintFunction))
			{
				Log::GetLog()->error("DisableBlueprintPreHook() - Failed to remove dispatcher hook");
				return false;
			}
		}

		Log::GetLog()->debug("Pre-hook removed - " + blueprintFunction->NameField().ToString().ToString() +
			" (" + std::to_string(preHooks[blueprintFunction].size()) + " pre-hooks, " + std::to_string(postHooks[blueprintFunction].size()) + " post-hooks)");
		return true;
	}

	bool DisableBlueprintPostHook(::UFunction* blueprintFunction, BlueprintHookFuncPtr hookFunction)
	{
		if (!blueprintFunction)
		{
			Log::GetLog()->error("DisableBlueprintPostHook() - null blueprintFunction");
			return false;
		}
		if (!hookFunction)
		{
			Log::GetLog()->error("DisableBlueprintPostHook() - null hookFunction");
			return false;
		}

		// Remove the hook
		const auto iter = std::remove(postHooks[blueprintFunction].begin(), postHooks[blueprintFunction].end(), hookFunction);
		if (iter == postHooks[blueprintFunction].end())
		{
			Log::GetLog()->error("DisableBlueprintPostHook() - Hook not found");
			return false;
		}
		postHooks[blueprintFunction].erase(iter, postHooks[blueprintFunction].end());

		// If there are no remaining hooks set on the blueprint function, remove the dispatcher hook
		if (preHooks[blueprintFunction].empty() && postHooks[blueprintFunction].empty())
		{
			if (!RemoveDispatcherHook(blueprintFunction))
			{
				Log::GetLog()->error("DisableBlueprintPostHook() - Failed to remove dispatcher hook");
				return false;
			}
		}

		Log::GetLog()->debug("Post-hook removed - " + blueprintFunction->NameField().ToString().ToString() +
			" (" + std::to_string(preHooks[blueprintFunction].size()) + " pre-hooks, " + std::to_string(postHooks[blueprintFunction].size()) + " post-hooks)");
		return true;
	}
}

void ReadConfig()
{
	nlohmann::json config;
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/" + PLUGIN_NAME + "/config.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();

	if (config["Debug"])
		Log::GetLog()->set_level(spdlog::level::debug);
}

void Load()
{
	Log::Get().Init(PLUGIN_NAME);

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	// Create the hook dispatcher UFunction
	BlueprintHooks::dispatcherUFunction = static_cast<BlueprintHooks::UFunction*>(malloc(BlueprintHooks::UFunction::GetPrivateStaticClass()->PropertiesSizeField()));
	if (!BlueprintHooks::dispatcherUFunction)
		return;

	BlueprintHooks::dispatcherUFunction->NameField() = FName(FString("BlueprintHooks_Dispatcher").ToString().c_str(), EFindName::FNAME_Add);
	BlueprintHooks::dispatcherUFunction->FunctionFlagsField() = static_cast<unsigned int>(EFunctionFlags::FUNC_Native);
	BlueprintHooks::dispatcherUFunction->FuncField() = BlueprintHooks::BlueprintHookDispatcher;
	BlueprintHooks::dispatcherUFunction->OuterField() = APrimalDinoCharacter::GetPrivateStaticClass();  // This seems to work (might have issues with UFunction::Invoke())
}

void Unload()
{
	//TODO Handle hooks still loaded

	if (BlueprintHooks::dispatcherUFunction)
		free(BlueprintHooks::dispatcherUFunction);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}