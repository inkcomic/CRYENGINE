#include "StdAfx.h"
#include "PluginDll.h"

#include "HyperealResources.h"
#include "HyperealDevice.h"

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

namespace CryVR
{
namespace Hypereal {

	float CPlugin_Hypereal::s_hmd_quad_distance = 0.25f;
	float CPlugin_Hypereal::s_hmd_quad_width = 1.0f;
	int CPlugin_Hypereal::s_hmd_quad_absolute = 1;

CPlugin_Hypereal::~CPlugin_Hypereal()
{
	CryVR::Hypereal::Resources::Shutdown();

	if (IConsole* const pConsole = gEnv->pConsole)
	{
		pConsole->UnregisterVariable("hmd_quad_distance");
		pConsole->UnregisterVariable("hmd_quad_width");
		pConsole->UnregisterVariable("hmd_quad_absolute");
	}

	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
}

bool CPlugin_Hypereal::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

	REGISTER_CVAR2("hmd_quad_distance", &s_hmd_quad_distance, s_hmd_quad_distance, VF_NULL, "Distance between eyes and UI quad");

	REGISTER_CVAR2("hmd_quad_width", &s_hmd_quad_width, s_hmd_quad_width, VF_NULL, "Width of the UI quad in meters");

	REGISTER_CVAR2("hmd_quad_absolute", &s_hmd_quad_absolute, s_hmd_quad_absolute, VF_NULL, "Should quads be placed relative to the HMD or in absolute tracking space? (Default = 1: Absolute UI positioning)");

	return true;
}

void CPlugin_Hypereal::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_PRE_RENDERER_INIT:
		{
			// Initialize resources to make sure we query available VR devices
			CryVR::Hypereal::Resources::Init();

			if (auto *pDevice = GetDevice())
			{
				gEnv->pSystem->GetHmdManager()->RegisterDevice(GetName(), *pDevice);
			}
		}
	break;
	}
}

IHyperealDevice* CPlugin_Hypereal::CreateDevice()
{
	return GetDevice();
}

IHyperealDevice* CPlugin_Hypereal::GetDevice() const
{
	return CryVR::Hypereal::Resources::GetAssociatedDevice();
}

CRYREGISTER_SINGLETON_CLASS(CPlugin_Hypereal)
}      // namespace Hypereal
}      // namespace CryVR

#include <CryCore/CrtDebugStats.h>