ScriptName PB_F4SE_BlinkManageQuestScript Extends Quest

Actor Property PlayerRef Auto

GLobalVariable Property gINIFileHotkeyEnable Auto
GLobalVariable Property gBlinkAPCost Auto
GLobalVariable Property gBlinkDuration Auto

int Property HUD_Print = 10 AutoReadOnly
int Property HUD_IconChange = 20 AutoReadOnly

int Property HUD_Type_OverWatch = 0 AutoReadOnly
int Property HUD_Type_Battery = 1 AutoReadOnly
int Property HUD_Type_Slice = 2 AutoReadOnly
int Property HUD_Type_Nest = 3 AutoReadOnly

float fScaleBaseX = 0.25
float fScaleBaseZ = 0.25

float Property fScaleLength = 1.0 Auto
float Property fChargeTime = 2.5 Auto
float Property fRange = 5000.0 Auto
float Property fScale = 1.0 Auto
int Property iHUDType = 0 Auto

float Property fPosX = 5.0 Auto
float Property fPosZ = 5.0 Auto

bool bPosChange
bool bScaleChange
bool bRangeChange
bool bChargeChange
bool bIconChange

Function scaleChange()
	float[] args = new float[2]
	args[0] = fScale * fScaleBaseX
	args[1] = fScale * fScaleBaseZ * fScaleLength
	PB_PlayerBlink_F4SE.sendInvokeAction("changeScale", args)
EndFunction

Function chargeChange()
	float[] args = new float[1]
	args[0] = fChargeTime
	PB_PlayerBlink_F4SE.sendInvokeAction("chargeChange", args)
EndFunction

function iconChange()
	float[] args = new float[1]
	args[0] = iHUDType as float
	PB_PlayerBlink_F4SE.sendInvokeAction("iconChange", args)
Endfunction

Function posChange()
	float[] args = new float[2]
	args[0] = fPosX
	args[1] = fPosZ
	PB_PlayerBlink_F4SE.sendPositionChange(args)
EndFunction

Function 	blinkIconCountChange()
	float[] args = new float[1]
	args[0] = 3.0
	PB_PlayerBlink_F4SE.sendInvokeAction("blinkIconCountChange", args)
EndFunction

Function MCMIconChange()
	if !bIconChange
		bIconChange = true
		Utility.Wait(0.03)

		iconChange()
		blinkIconCountChange()

		bIconChange = false
	Endif
EndFunction

Function MCMPosChange()
	if !bPosChange
		bPosChange = true
		Utility.Wait(0.03)

		posChange()

		bPosChange = false
	Endif
EndFunction

Function MCMScaleChange()
	if !bScaleChange
		bScaleChange = true
		Utility.Wait(0.03)

		ScaleChange()

		bScaleChange = false
	Endif
EndFunction

Function MCMChargeChange()
	if !bChargeChange
		bChargeChange = true
		Utility.Wait(0.03)
		chargeChange()

		bChargeChange = false
	Endif
EndFunction

Event OnQuestInit()
	RegisterForRemoteEvent(PlayerRef, "OnPlayerLoadGame")

	PB_PlayerBlink_F4SE.setFirstPosition()
	iconChange()
	posChange()
	blinkIconCountChange()
	chargeChange()
	scaleChange()

	if gINIFileHotkeyEnable.GetValue() == 1
		PB_PlayerBlink_F4SE.SetINIFileHotkey()
	Endif
EndEvent

Event Actor.OnPlayerLoadGame(Actor akSender)
	PB_PlayerBlink_F4SE.setFirstPosition()
	iconChange()
	posChange()
	blinkIconCountChange()
	chargeChange()
	scaleChange()

	if gINIFileHotkeyEnable.GetValue() == 1
		PB_PlayerBlink_F4SE.SetINIFileHotkey()
	Endif
EndEvent

Function FlashStart()
	PB_PlayerBlink_F4SE.tryBlink()
EndFunction


Function RegisterIniFileKey()
	bool setHotkey = PB_PlayerBlink_F4SE.SetINIFileHotkey()
	if !setHotkey
		debug.messagebox("This setting is for users who wish to use a key that cannot be configured within the MCM.\nThe 'useiniFileHotkey' value in the mod folder/F4SE/Plugins/blinkHotkey.ini file is set to 0. \nSince the value is 0, the hotkey registration is canceled. Set it to 1 to enable the hotkey.")
		Return
	else
		Debug.Messagebox("The hotkey for Blink mode has been set.")
	endif
EndFunction
