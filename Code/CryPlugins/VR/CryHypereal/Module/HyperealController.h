#pragma once

#include <CrySystem/VR/IHMDDevice.h>
#include <CryInput/IInput.h>

#define HYPEREAL_SPECIAL (1 << 8)
#define HYPEREAL_BASE    eKI_Motion_Hypereal_TouchPad_A

namespace CryVR
{
namespace Hypereal
{
class Controller : public IHmdController
{
public:
	// IHmdController
	virtual bool                    IsConnected(EHmdController id) const override;
	virtual bool                    IsButtonPressed(EHmdController controllerId, EKeyId id) const override { return (id < HYPEREAL_BASE || id > eKI_Motion_Hypereal_SideTriggerBtnR) ? false : (m_state[controllerId].buttonsPressed /*& vr::ButtonMaskFromId((vr::EVRButtonId)(m_symbols[id - HYPEREAL_BASE]->devSpecId & (~HYPEREAL_SPECIAL)))*/) > 0; }
	virtual bool                    IsButtonTouched(EHmdController controllerId, EKeyId id) const override { return false; }
	virtual bool                    IsGestureTriggered(EHmdController controllerId, EKeyId id) const override { return false; }                           // Hypereal does not have gesture support (yet?)
	virtual float                   GetTriggerValue(EHmdController controllerId, EKeyId id) const override { return m_state[controllerId].trigger; }   // we only have one trigger => ignore trigger id
	virtual Vec2                    GetThumbStickValue(EHmdController controllerId, EKeyId id)const override { return m_state[controllerId].touchPad; }  // we only have one 'stick' (/the touch pad) => ignore thumb stick id

	virtual const HmdTrackingState& GetNativeTrackingState(EHmdController controller) const override { return m_state[controller].nativePose; }
	virtual const HmdTrackingState& GetLocalTrackingState(EHmdController controller) const override { return m_state[controller].localPose; }

	virtual void                    ApplyForceFeedback(EHmdController id, float freq, float amplitude) override;
	virtual void                    SetLightColor(EHmdController id, TLightColor color) override {}
	virtual TLightColor             GetControllerColor(EHmdController id) const override { return 0; }
	virtual uint32                  GetCaps(EHmdController id) const override { return (eCaps_Buttons | eCaps_Tracking | eCaps_Sticks | eCaps_Capacitors); }
	// ~IHmdController

private:
	friend class Device;

	Controller();
	virtual ~Controller() override;

	struct SControllerState
	{
		SControllerState()
			:buttonsPressed(0)
			, trigger(0.0f)
			, sideTrigger(0.0f)
		{
		}

		HmdTrackingState nativePose;
		HmdTrackingState localPose;
		uint64           buttonsPressed;		
		float            trigger;
		float            sideTrigger;
		Vec2             touchPad;

		inline bool      Pressed(HyButton btn)
		{
			return (buttonsPressed & (HyButton)(btn)) > 0;
		}

	};

	bool          Init(HyDevice *device);
	void          Update(HySubDevice controllerId, HmdTrackingState nativeState, HmdTrackingState localState, HyInputState& vrState);
	void          DebugDraw(float& xPosLabel, float& yPosLabel) const;
	SInputSymbol* MapSymbol(uint32 deviceSpecificId, EKeyId keyId, const TKeyName& name, SInputSymbol::EType type, uint32 user);
	void          OnControllerConnect(HySubDevice controllerId);
	void          OnControllerDisconnect(HySubDevice controllerId);
	void          PostButtonIfChanged(EHmdController controllerId, EKeyId keyID);
	void          ClearState();

	SInputSymbol*            m_symbols[eKI_Motion_Hypereal_NUM_SYMBOLS];
	SControllerState         m_state[eHmdController_Hypereal_MaxHyperealControllers],
	                         m_previousState[eHmdController_Hypereal_MaxHyperealControllers];
	HySubDevice m_controllerMapping[eHmdController_Hypereal_MaxHyperealControllers];

	HyDevice *		m_Device;
};
}
}
