#include "StdAfx.h"

#include "HyperealResources.h"
#include "HyperealDevice.h"


#define HY_RELEASE(p) {if(p != nullptr) p->Release(); p = nullptr;}
namespace CryVR {
namespace Hypereal {

// static member variables
Resources* Resources::ms_pRes = 0;
bool Resources::ms_libInitialized = false;

// ------------------------------------------------------------------------
// Creates the resource manager for Hypereal devices.
// If an Hypereal device is attached it will create and store an instance of the device.
Resources::Resources()
	: m_pDevice(0)
{
	//if the renderer is not Dx11, don't initialize the Hypereal
	if (strcmp(gEnv->pConsole->GetCVar("r_driver")->GetString(), "DX11") != 0)
	{
		CryLogAlways("[HMD][Hypereal] Unable to initialize Hypereal, only DX11 supported.");
		return;
	}

	CryLogAlways("[HMD][Hypereal] Initialising Resources - Using Hypereal %s", sch_HyDevice_Version);

	ms_libInitialized = true;
	HyResult hr = HyStartup();
 	if (hr == HyResult::hySuccess)
 	{
 		m_pDevice = Device::CreateInstance();
 	}
	else
 		CryLogAlways("[HMD][Hypereal] HyperealVR Failed to Startup.!");
}

// ------------------------------------------------------------------------
void Resources::PostInit()
{
//	Device* pDev = GetAssociatedDevice();
}

// ------------------------------------------------------------------------
Resources::~Resources()
{
	#if !defined(_RELEASE)
	if (m_pDevice && m_pDevice->GetRefCount() != 1)
		__debugbreak();
	#endif

	SAFE_RELEASE(m_pDevice);

	if (ms_libInitialized)
	{
		HyShutdown();
		CryLogAlways("[HMD][Hypereal] Shutdown finished");
	}
}

// ------------------------------------------------------------------------
void Resources::Init()
{
	#if !defined(_RELEASE)
	if (ms_pRes)
		__debugbreak();
	#endif

	if (!ms_pRes)
		ms_pRes = new Resources();
}

// ------------------------------------------------------------------------
void Resources::Shutdown()
{
	#if !defined(_RELEASE)
	if (ms_libInitialized && !ms_pRes)
		__debugbreak();
	#endif

	SAFE_DELETE(ms_pRes);
}

} // namespace Hypereal
} // namespace CryVR