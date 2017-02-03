#include "StdAfx.h"

#include "HyperealController.h"

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IRenderAuxGeom.h>

namespace CryVR {
namespace Hypereal {

#define VRC_AXIS_THRESHOLD 0.5f

#define MAPSYMBOL(EKI, DEV_KEY_ID, KEY_NAME, KEY_TYPE) m_symbols[EKI - HYPEREAL_BASE] = MapSymbol(DEV_KEY_ID, EKI, KEY_NAME, KEY_TYPE, 0);

// -------------------------------------------------------------------------
Controller::Controller():m_Device(nullptr)
{
	memset(&m_previousState, 0, sizeof(m_previousState));
	memset(&m_state, 0, sizeof(m_state));

	for (int iController = 0; iController < eKI_Motion_Hypereal_NUM_SYMBOLS; iController++)
		m_controllerMapping[iController] = HY_SUBDEV_CONTROLLER_RESERVED;//max controller id
}

// -------------------------------------------------------------------------
bool Controller::Init(HyDevice *device)
{
	m_Device = device;
	// Button symbols
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_A, HyButton::HY_BUTTON_A, "hytouch_a", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_B, HyButton::HY_BUTTON_B, "hytouch_b", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_X, HyButton::HY_BUTTON_X, "hytouch_x", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TouchPad_Y, HyButton::HY_BUTTON_Y, "hytouch_y", SInputSymbol::Button);
	MAPSYMBOL(eKI_Motion_Hypereal_TriggerBtnL, HyButton::HY_BUTTON_THUMB_LEFT, "hytouch_thumbl", SInputSymbol::Trigger);
	MAPSYMBOL(eKI_Motion_Hypereal_TriggerBtnR, HyButton::HY_BUTTON_THUMB_RIGHT, "hytouch_thumbr", SInputSymbol::Trigger);
	MAPSYMBOL(eKI_Motion_Hypereal_SideTriggerBtnL, HyButton::HY_BUTTON_SHOULDER_LEFT, "hytouch_shoulderl", SInputSymbol::Trigger);
	MAPSYMBOL(eKI_Motion_Hypereal_SideTriggerBtnR, HyButton::HY_BUTTON_SHOULDER_RIGHT, "hytouch_shoulderr", SInputSymbol::Trigger);

	return true;
}

#undef MapSymbol

// -------------------------------------------------------------------------
Controller::~Controller()
{
	for (int iSymbol = 0; iSymbol < eKI_Motion_Hypereal_NUM_SYMBOLS; iSymbol++)
		SAFE_DELETE(m_symbols[iSymbol]);
}

// -------------------------------------------------------------------------
void Controller::ClearState()
{
	for (int i = 0; i < eHmdController_Hypereal_MaxHyperealControllers; i++)
		m_state[i] = SControllerState();
}

// -------------------------------------------------------------------------
SInputSymbol* Controller::MapSymbol(uint32 deviceSpecificId, EKeyId keyId, const TKeyName& name, SInputSymbol::EType type, uint32 user)
{
	SInputSymbol* pSymbol = new SInputSymbol(deviceSpecificId, keyId, name, type);
	pSymbol->deviceType = eIDT_MotionController;
	pSymbol->user = user;
	if (type == SInputSymbol::EType::Axis || type == SInputSymbol::EType::RawAxis)
		pSymbol->state = eIS_Unknown;
	else
		pSymbol->state = eIS_Pressed;
	pSymbol->value = 0.0f;
	return pSymbol;
}

// -------------------------------------------------------------------------
void Controller::Update(HySubDevice controllerId, HmdTrackingState nativeState, HmdTrackingState localState, HyInputState& vrState)
{
 	EHmdController index = eHmdController_Hypereal_MaxHyperealControllers;
 	for (int i = 0; i < eHmdController_Hypereal_MaxHyperealControllers; i++)
 	{
 		if (m_controllerMapping[i] == controllerId)
 		{
 			index = static_cast<EHmdController>(i);
 			break;
 		}
 	}
 
 	if ((index < eHmdController_Hypereal_MaxHyperealControllers)/* && (m_state[index].packetNum != vrState.unPacketNum)*/)
 	{
 		m_previousState[index] = m_state[index];
  		
 		m_state[index].buttonsPressed = vrState.m_buttons;
 
 		m_state[index].trigger = vrState.m_trigger;
		m_state[index].sideTrigger = vrState.m_sideTrigger;
		m_state[index].touchPad = Vec2(vrState.m_thumbstick.x, vrState.m_thumbstick.y);
		
 		// Send button events (if necessary)
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_A);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_B);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_X);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TouchPad_Y);
 		PostButtonIfChanged(index, eKI_Motion_Hypereal_TriggerBtnL);
		PostButtonIfChanged(index, eKI_Motion_Hypereal_TriggerBtnR);
		PostButtonIfChanged(index, eKI_Motion_Hypereal_SideTriggerBtnL);
		PostButtonIfChanged(index, eKI_Motion_Hypereal_SideTriggerBtnR);
 		// send trigger event (if necessary)
 		if (m_state[index].trigger != m_previousState[index].trigger)
 		{
 			SInputEvent event;
			SInputSymbol* pSymbol = nullptr;
			if (index == eHmdController_Hypereal_1)
				m_symbols[eKI_Motion_Hypereal_TriggerBtnL - HYPEREAL_BASE];
			else
				m_symbols[eKI_Motion_Hypereal_TriggerBtnR - HYPEREAL_BASE];
			
 			pSymbol->ChangeEvent(m_state[index].trigger);
 			pSymbol->AssignTo(event);
 			event.deviceIndex = controllerId;
 			event.deviceType = eIDT_MotionController;
 			gEnv->pInput->PostInputEvent(event);
 		}
 
		// send trigger event (if necessary)
		if (m_state[index].sideTrigger != m_previousState[index].sideTrigger)
		{
			SInputEvent event;
			SInputSymbol* pSymbol = nullptr;
			if(index== eHmdController_Hypereal_1)
				m_symbols[eKI_Motion_Hypereal_SideTriggerBtnL - HYPEREAL_BASE];
			else 
				m_symbols[eKI_Motion_Hypereal_SideTriggerBtnR - HYPEREAL_BASE];
			pSymbol->ChangeEvent(m_state[index].sideTrigger);
			pSymbol->AssignTo(event);
			event.deviceIndex = controllerId;
			event.deviceType = eIDT_MotionController;
			gEnv->pInput->PostInputEvent(event);
		}
 	}
 	m_state[index].localPose = localState;
 	m_state[index].nativePose = nativeState;
}

// -------------------------------------------------------------------------
void Controller::PostButtonIfChanged(EHmdController controllerId, EKeyId keyID)
{
	SInputSymbol* pSymbol = m_symbols[keyID - HYPEREAL_BASE];
	HyButton vrBtn = static_cast<HyButton>(pSymbol->devSpecId);

	bool wasPressed = m_previousState[controllerId].Pressed(vrBtn);
	bool isPressed = m_state[controllerId].Pressed(vrBtn);

	if (isPressed != wasPressed)
	{
		SInputEvent event;
		pSymbol->PressEvent(isPressed);
		pSymbol->AssignTo(event);
		event.deviceIndex = controllerId;
		event.deviceType = eIDT_MotionController;
		gEnv->pInput->PostInputEvent(event);
	}
}

// -------------------------------------------------------------------------
void Controller::DebugDraw(float& xPosLabel, float& yPosLabel) const
{
}

// -------------------------------------------------------------------------;
void Controller::OnControllerConnect(HySubDevice controllerId)
{
	bool added = false;
	for (int i = 0; i < eHmdController_Hypereal_MaxHyperealControllers; i++)
	{
		if ((m_controllerMapping[i] >= HY_SUBDEV_CONTROLLER_RESERVED)&&(m_controllerMapping[i]!= controllerId))
		{
			m_controllerMapping[i] = controllerId;
			m_state[i] = SControllerState();
			m_previousState[i] = SControllerState();
			added = true;
			break;
		}
	}

	if (!added)
	{
		gEnv->pLog->LogError("[HMD][Hypereal] Maximum number of controllers reached! Ignoring new controller!");
	}
}

// -------------------------------------------------------------------------
void Controller::OnControllerDisconnect(HySubDevice controllerId)
{
	for (int i = 0; i < eHmdController_Hypereal_MaxHyperealControllers; i++)
	{
		if (m_controllerMapping[i] == controllerId)
		{
			m_controllerMapping[i] = HY_SUBDEV_CONTROLLER_RESERVED;
			break;
		}
	}
}

// -------------------------------------------------------------------------
bool Controller::IsConnected(EHmdController id) const
{
	if (!m_Device)
		return false;
	bool bControllerConnected = false;
	if (eHmdController_Hypereal_1==id)
		m_Device->GetBoolValue(HY_PROPERTY_CONTROLLER0_CONNECTED_BOOL, bControllerConnected);
	else if(eHmdController_Hypereal_2==id)
		m_Device->GetBoolValue(HY_PROPERTY_CONTROLLER0_CONNECTED_BOOL, bControllerConnected);
	return bControllerConnected;
}

// -------------------------------------------------------------------------
void Controller::ApplyForceFeedback(EHmdController id, float freq, float amplitude)
{
	auto durationMicroSeconds = (ushort)clamp_tpl(freq, 0.f, 5000.f);
	m_Device->SetControllerVibration(m_controllerMapping[id], durationMicroSeconds, amplitude);
}

} // namespace Hypereal
} // namespace CryVR
