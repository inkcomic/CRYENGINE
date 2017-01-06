// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Interface/IHyperealPlugin.h"

namespace CryVR
{
namespace Hypereal {
class CPlugin_Hypereal : public IOpenVRPlugin, public ISystemEventListener
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IOpenVRPlugin)
	CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CPlugin_OpenVR, "Plugin_OpenVR", 0x50A54ADB4BBF4068, 0x80B9EB3BFFA30C93)

	virtual ~CPlugin_OpenVR();

	//! Retrieve name of plugin.
	virtual const char* GetName() const override { return "CryOpenVR"; }

	//! Retrieve category for the plugin.
	virtual const char* GetCategory() const override { return "Plugin"; }

	//! This is called to initialize the new plugin.
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;

public:
	virtual IOpenVRDevice* CreateDevice() override;
	virtual IOpenVRDevice* GetDevice() const override;

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	// Start CVars
public:
	static float s_hmd_quad_distance;
	static float s_hmd_quad_width;
	static int s_hmd_quad_absolute;

protected:
	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {}
};

}      // namespace Hypereal
}      // namespace CryVR