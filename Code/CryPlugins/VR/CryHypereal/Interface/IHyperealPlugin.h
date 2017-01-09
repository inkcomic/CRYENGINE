#pragma once

#include <CrySystem/ICryPlugin.h>

#include "IHmdHyperealDevice.h"

namespace CryVR
{
namespace Hypereal {
struct IHyperealPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(IHyperealPlugin, 0x7BE372D29A434E48, 0x9A3836FB8EF3EB50);

public:
	virtual IHyperealDevice* CreateDevice() = 0;
	virtual IHyperealDevice* GetDevice() const = 0;
};

}      // namespace Hypereal
}      // namespace CryVR