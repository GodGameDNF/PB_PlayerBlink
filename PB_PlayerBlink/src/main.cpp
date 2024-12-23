#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <windows.h>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

using namespace RE;
namespace fs = std::filesystem;

PlayerCharacter* p = nullptr;
BSScript::IVirtualMachine* vm = nullptr;
TESDataHandler* DH = nullptr;
PlayerCamera* pcam = nullptr;
UI* ui = nullptr;

TESGlobal* gINIFileHotkeyEnable = nullptr;
TESGlobal* g_fRange = nullptr;
TESGlobal* g_fBlinkDuration = nullptr;
TESGlobal* g_iBlinkAPCost = nullptr;

ActorValueInfo* ActionPoints = nullptr;
TESEffectShader* blinkEffect = nullptr;

BSFixedString* scriptName = nullptr;
BSFixedString* functionName = nullptr;

uint32_t blinkKey;
std::string filePath;
std::string lootDir;

float playerX_Ready;
float playerY_Ready;
float movePerX;
float movePerY;

bool blinkRunning = false;

HMODULE hAVF = GetModuleHandleA("ActorVelocityFramework.dll");
typedef void (*SetVelocity)(std::monostate, Actor*, float, float, float, float, float, float, float);
SetVelocity fnSetVelocity = (SetVelocity)GetProcAddress(hAVF, "SetVelocity");

const static std::string UIName{ "blinkHUDUI" };
UIMessageQueue* msgQ = nullptr;

int hideCount = 0;
bool isLoading = false;
std::vector<std::string> hideMenuList = { "BarterMenu", "ContainerMenu", "CookingMenu", "CreditsMenu", "DialogueMenu", "ExamineMenu", "LevelUpMenu",
	"LockpickingMenu", "LooksMenu", "MessageBoxMenu", "PauseMenu", "PipboyMenu", "BookMenu",
	"SPECIALMenu", "TerminalHolotapeMenu", "TerminalMenu", "VATSMenu", "WorkshopMenu", "SitWaitMenu", "SleepWaitMenu",
	"F4QMWMenu" };

float FirstPosX = 0;
float FirstPosY = 0;

void Play_Effect(BSScript::IVirtualMachine* vm, uint32_t i, TESEffectShader* effect, TESObjectREFR* target, float timeDuration)
{
	using func_t = decltype(&Play_Effect);
	REL::Relocation<func_t> func{ REL::ID(262859) };
	return func(vm, i, effect, target, timeDuration);
}

template <class Ty>
Ty SafeWrite64Function(uintptr_t addr, Ty data)
{
	DWORD oldProtect;
	void* _d[2];
	memcpy(_d, &data, sizeof(data));
	size_t len = sizeof(_d[0]);

	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	Ty olddata;
	memset(&olddata, 0, sizeof(Ty));
	memcpy(&olddata, (void*)addr, len);
	memcpy((void*)addr, &_d[0], len);
	VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
	return olddata;
}

bool startBlink(std::monostate)
{

	NiPoint3 playerVelocity;               // 현재 속도 벡터
	p->GetLinearVelocity(playerVelocity);  // 속도 값 가져오기

	float cPlayerVeloX = playerVelocity.x;
	float cPlayerVeloY = playerVelocity.y;
	// 속도 벡터의 크기(길이) 계산
	float fLength = std::sqrt(cPlayerVeloX * cPlayerVeloX + cPlayerVeloY * cPlayerVeloY);

	// 방향 벡터 계산 (단위 벡터로 정규화)
	NiPoint3 direction;
	if (fLength > 0) {  // 0으로 나누지 않도록 확인
		direction.x = cPlayerVeloX / fLength;
		direction.y = cPlayerVeloY / fLength;
	} else {
		// 속도가 0일 경우 방향 벡터는 정의되지 않으므로 기본값으로 설정
		direction = NiPoint3(0, 0, 0);
	}

	float fDuration = g_fBlinkDuration->value;
	if (fDuration < 0.1) {
		fDuration = 0.1;
	}

	float fRange = g_fRange->value / fDuration;  // 이동속도 조절을 위해 지속시간을 계산
	if (fRange < 100) {
		fRange = 100;
	}

	float moveDirectionX = direction.x * fRange;
	float moveDirectionY = direction.y * fRange;

	Play_Effect(vm, 0, blinkEffect, p, 0.5);  // 0.5는 이펙트 재생시간
	fnSetVelocity(std::monostate{}, p, moveDirectionX, moveDirectionY, 0.0f, fDuration, moveDirectionX, moveDirectionY, 0.0f);

	return true;
}

class BodyPartsUI : public IMenu
{
private:
	static BodyPartsUI* instance;

public:
	BodyPartsUI() : IMenu()
	{
		if (instance) {
			delete (instance);
		}
		instance = this;
		instance->menuFlags = (UI_MENU_FLAGS)0;
		instance->UpdateFlag(UI_MENU_FLAGS::kAllowSaving, true);
		instance->UpdateFlag(UI_MENU_FLAGS::kDontHideCursorWhenTopmost, true);
		instance->UpdateFlag(UI_MENU_FLAGS::kAlwaysOpen, true);
		instance->depthPriority = UI_DEPTH_PRIORITY::kHUD;
		instance->inputEventHandlingEnabled = false;
		BSScaleformManager* sfm = BSScaleformManager::GetSingleton();
		bool succ = sfm->LoadMovieEx(*instance, "Interface/PB_BlinkInterface_F4SE.swf", "root", BSScaleformManager::ScaleModeType::kShowAll, 0.0f);
		if (succ) {
			instance->menuObj.SetMember("menuFlags", Scaleform::GFx::Value(instance->menuFlags.underlying()));
			instance->menuObj.SetMember("movieFlags", Scaleform::GFx::Value(3));
			instance->menuObj.SetMember("extendedFlags", Scaleform::GFx::Value(3));
		} else {
		}
	}
	
	void setFirstPosition()
	{
		if (uiMovie && uiMovie->asMovieRoot) {
			std::array<Scaleform::GFx::Value, 2> args;
			args[0] = FirstPosX;
			args[1] = FirstPosY;

			bool succ = uiMovie->asMovieRoot->Invoke("root.setFirstPosition", nullptr, args.data(), args.size());

			if (!succ) {
			}
		}
	}

	// UI의 이동. 미리 계산해둔 화면 크기 비율의 배율을 곱해서 백분율로 보냄
	void setPositionChange(std::vector<float> sendarray)
	{
		//logger::info("포지션 시도 시작");
		if (uiMovie && uiMovie->asMovieRoot) {
			std::array<Scaleform::GFx::Value, 2> args;
			args[0] = sendarray[0] * movePerX;
			args[1] = sendarray[1] * movePerY;

			//logger::info("변수할당");
			bool succ = uiMovie->asMovieRoot->Invoke("root.changePosition", nullptr, args.data(), args.size());
			//logger::info("인보크 발송");
			if (!succ) {
			}
		}
	}
	

	void sendInvoke(std::string functionName, std::vector<float> sendarray)
	{
		if (uiMovie && uiMovie->asMovieRoot) {
			uint32_t argSize = sendarray.size();

			std::vector<Scaleform::GFx::Value> args(argSize);

			for (int i = 0; i < argSize; ++i) {
				args[i] = sendarray[i];
			}

			bool succ = uiMovie->asMovieRoot->Invoke(("root." + functionName).c_str(), nullptr, args.data(), args.size());

			if (!succ) {
				//logger::info("{} 인보크 실패", functionName);
			}
		}
	}

	// 파피루스나 f4se에서 블링크시도를 받은다음 swf에 신호를 보내서 반환값으로 블링크 판단
	void tryBlinkAction()
	{
		if (blinkRunning) {
			return;
		}

		blinkRunning = true;
		float playerAP = p->GetActorValue(*ActionPoints);
		float costAP = g_iBlinkAPCost->value;

		if (costAP > 0) {
			if (playerAP < costAP) {
				blinkRunning = false;
				return;
			}
		}

		if (uiMovie && uiMovie->asMovieRoot) {

			//logger::info("블링크 시도");

			Scaleform::GFx::Value result;
			std::array<Scaleform::GFx::Value, 0> args;

			uiMovie->asMovieRoot->Invoke("root.tryBlink", &result, args.data(), args.size());

			if (result.IsUndefined()) {
				//logger::info("변수 받기 실패");
			} else {
				bool succ = result.GetBoolean(); // true이면 점멸의 횟수가 남음
				if (succ) {
					//logger::info("점멸 신호 {}", succ);
					if (costAP > 0) {
						p->ModActorValue(ACTOR_VALUE_MODIFIER::Damage, *ActionPoints, -costAP);
					}

					startBlink(std::monostate{});
				}
			}
		}
	
		blinkRunning = false;
	}

	static BodyPartsUI* GetSingleton()
	{
		return instance;
	}
};
BodyPartsUI* BodyPartsUI::instance = nullptr;

class InputEventReceiverOverride : public BSInputEventReceiver
{
public:
	typedef void (InputEventReceiverOverride::*FnPerformInputProcessing)(const InputEvent* a_queueHead);

	void ProcessButtonEvent(const ButtonEvent* evn)
	{
		if (gINIFileHotkeyEnable->value == 0 || evn->eventType != INPUT_EVENT_TYPE::kButton || !evn->QJustPressed()) {
			if (evn->next) {
				ProcessButtonEvent((ButtonEvent*)evn->next);
			}
			return;
		}

		uint32_t id = evn->idCode;

		if (evn->device == INPUT_DEVICE::kGamepad) {
			id += 10000;
		}
		else if (evn->device == INPUT_DEVICE::kMouse) {
			id += 1;
		}

		if (id == blinkKey) {
			BodyPartsUI* bdUI = BodyPartsUI::GetSingleton();
			if (bdUI) {
				bdUI->tryBlinkAction();
			}
		}

		if (evn->next) {
			ProcessButtonEvent((ButtonEvent*)evn->next);
		}
	}

	void HookedPerformInputProcessing(const InputEvent* a_queueHead)
	{
		if (!ui->menuMode && a_queueHead) {
			ProcessButtonEvent((ButtonEvent*)a_queueHead);
		}
		FnPerformInputProcessing fn = fnHash.at(*(uint64_t*)this);
		if (fn) {
			(this->*fn)(a_queueHead);
		}
	}

	void HookSink()
	{
		uint64_t vtable = *(uint64_t*)this;
		auto it = fnHash.find(vtable);
		if (it == fnHash.end()) {
			FnPerformInputProcessing fn = SafeWrite64Function(vtable, &InputEventReceiverOverride::HookedPerformInputProcessing);
			fnHash.insert(std::pair<uint64_t, FnPerformInputProcessing>(vtable, fn));
		}
	}

	void UnHookSink()
	{
		uint64_t vtable = *(uint64_t*)this;
		auto it = fnHash.find(vtable);
		if (it == fnHash.end())
			return;
		SafeWrite64Function(vtable, it->second);
		fnHash.erase(it);
	}

protected:
	static std::unordered_map<uint64_t, FnPerformInputProcessing> fnHash;
};
std::unordered_map<uint64_t, InputEventReceiverOverride::FnPerformInputProcessing> InputEventReceiverOverride::fnHash;

bool SetINIFileHotkey(std::monostate)
{
	if (!pcam || gINIFileHotkeyEnable->value == 0) {
		((InputEventReceiverOverride*)((uint64_t)pcam + 0x38))->UnHookSink();
		return false;
	}

	std::ifstream file(filePath);
	std::string line;
	int useIniFileHotkey = 0;
	int keycode = 0;

	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line.empty() || line[0] == '#') {
				continue;  // 다음 줄로 넘어감
			}

			if (line.find("useiniFileHotkey") != std::string::npos) {
				size_t pos = line.find("=");
				if (pos != std::string::npos) {
					useIniFileHotkey = (std::isdigit(line[pos + 1])) ? std::stoi(line.substr(pos + 1)) : 0;  // 잘못된 값은 0으로 처리
				}
			} else if (line.find("keycode") != std::string::npos) {
				size_t pos = line.find("=");
				if (pos != std::string::npos) {
					keycode = (std::isdigit(line[pos + 1])) ? std::stoi(line.substr(pos + 1)) : 0;  // 잘못된 값은 0으로 처리
				}
			}
		}
		file.close();
	}

	if (useIniFileHotkey == 1 && keycode >= 1) {
		((InputEventReceiverOverride*)((uint64_t)pcam + 0x38))->HookSink();
		blinkKey = keycode;
		return true; // 단축키를 설정했음
	}
	else {
		((InputEventReceiverOverride*)((uint64_t)pcam + 0x38))->UnHookSink();
		gINIFileHotkeyEnable->value = 0;
		return false;  // 단축키를 해제하거나 없음
	}
}

bool isMenuMatching(const BSFixedString& menuName)
{
	// 함수 내부에 static으로 선언
	static const std::vector<std::string> menuNames = {
		"BarterMenu",
		"Console",
		"ContainerMenu",
		"PipboyMenu"
	};

	// 배열에서 menuName이 존재하는지 확인
	return std::find(menuNames.begin(), menuNames.end(), menuName) != menuNames.end();
}
/*
class MenuWatcher : public BSTEventSink<MenuOpenCloseEvent>
{
	virtual BSEventNotifyControl ProcessEvent(const MenuOpenCloseEvent& evn, BSTEventSource<MenuOpenCloseEvent>* src) override
	{
		if (msgQ){
			BSFixedString menuNameString = evn.menuName; 

			if (isMenuMatching(menuNameString)) {
				if (evn.opening) {
					if (ui->GetMenuOpen(UIName)) {
						msgQ->AddMessage(UIName, RE::UI_MESSAGE_TYPE::kHide);
					}
				} else {
					if (!ui->GetMenuOpen(UIName)) {
						msgQ->AddMessage(UIName, RE::UI_MESSAGE_TYPE::kShow);
					}
				}
			} else if (menuNameString != UIName) {
				msgQ->AddMessage(UIName, RE::UI_MESSAGE_TYPE::kShow);
			}
		} 

		return BSEventNotifyControl::kContinue;
	}
};
*/



class MenuWatcher : public BSTEventSink<MenuOpenCloseEvent>
{
	virtual BSEventNotifyControl ProcessEvent(const MenuOpenCloseEvent& evn, BSTEventSource<MenuOpenCloseEvent>* src) override
	{
		UI* ui = UI::GetSingleton();
		UIMessageQueue* msgQ = UIMessageQueue::GetSingleton();
		BodyPartsUI* bpUI = BodyPartsUI::GetSingleton();
		if (msgQ && !ui->GetMenuOpen(UIName))
		{
			msgQ->AddMessage(UIName, RE::UI_MESSAGE_TYPE::kShow);
		}

		//_MESSAGE("Menu %s opening %d", evn.menuName.c_str(), evn.opening);
		if (evn.menuName == BSFixedString("LoadingMenu")) {
			if (evn.opening) {
				isLoading = true;
			} else {
				hideCount = 0;
				for (auto it = hideMenuList.begin(); it != hideMenuList.end(); ++it) {
					if (ui->GetMenuOpen(*it)) {
						++hideCount;
					}
				}
				isLoading = false;
			}
		}
		for (auto it = hideMenuList.begin(); it != hideMenuList.end(); ++it) {
			if (evn.menuName == *it) {
				if (evn.opening) {
					++hideCount;
					//_MESSAGE("%s ++ %d", evn.menuName.c_str(), hideCount);
				} else {
					--hideCount;
					//_MESSAGE("%s -- %d", evn.menuName.c_str(), hideCount);
				}
				break;
			}
		}
		if (bpUI) {
			if (hideCount  > 0) {
				if (ui->GetMenuOpen(UIName)) {
					msgQ->AddMessage(UIName, RE::UI_MESSAGE_TYPE::kHide);
				}
			} else if (hideCount == 0) {// && !isMenuOpen) {
				if (!ui->GetMenuOpen(UIName)) {
					msgQ->AddMessage(UIName, RE::UI_MESSAGE_TYPE::kShow);
				}
			}
		}
		return BSEventNotifyControl::kContinue;
	}
};

void RegisterMenu()
{
	ui = UI::GetSingleton();
	msgQ = UIMessageQueue::GetSingleton();

	if (ui) {
		ui->RegisterMenu(UIName.c_str(), [](const UIMessage&) -> IMenu* {
			BodyPartsUI* bpUI = BodyPartsUI::GetSingleton();
			if (!bpUI) {
				bpUI = new BodyPartsUI();
			}
			return bpUI;
		});
		msgQ->AddMessage(UIName, RE::UI_MESSAGE_TYPE::kShow);
	}
}

void setHUDPositionRatio()
{
	INIPrefSettingCollection* iniSetting = INIPrefSettingCollection::GetSingleton();
	Setting* gameHeight01 = iniSetting->GetSetting("iSize W:Display"sv);
	Setting* gameHeight02 = iniSetting->GetSetting("iSize H:Display"sv);

	uint32_t iResolutionH;
	uint32_t iResolutionW;

	if (gameHeight01 && gameHeight02) {
		iResolutionW = gameHeight01->GetInt();
		iResolutionH = gameHeight02->GetInt();
	} else {
		iResolutionW = 1920;
		iResolutionH = 1080;
	}

	float swf_width = 550.0f;
	float swf_height = 400.0f;

	// 타겟 화면 비율 계산
	float target_aspect = (float)iResolutionW / (float)iResolutionH;

	// SWF 가로 세로 비율 계산
	float swf_aspect = swf_width / swf_height;

	// 화면에 SWF 맞추기
	float target_width, target_height;
	if (target_aspect > swf_aspect) {
		// 화면이 더 넓을 때
		target_width = swf_height * target_aspect;
		target_height = swf_height;
	} else {
		// 화면이 더 좁을 때
		target_width = swf_width;
		target_height = swf_width / target_aspect;
	}

	// X, Y 오프셋 계산
	FirstPosX = (swf_width - target_width) / 2.0f;
	FirstPosY = (swf_height - target_height) / 2.0f;

	movePerX = target_width / 100.0f;  // X축의 1% 당 이동 거리
	movePerY = target_height / 100.0f; // Y축의 1% 당 이동 거리

}

void sendInvokeAction(std::monostate, std::string functionName, std::vector<float> args)
{
	BodyPartsUI* bdUI = BodyPartsUI::GetSingleton();
	if (bdUI) {
		bdUI->sendInvoke(functionName, args);
	}
}

void setFirstPosition(std::monostate)
{
	BodyPartsUI* bdUI = BodyPartsUI::GetSingleton();
	if (bdUI) {
		bdUI->setFirstPosition();
	}
}

void sendPositionChange(std::monostate, std::vector<float> args)
{
	BodyPartsUI* bdUI = BodyPartsUI::GetSingleton();
	if (bdUI) {
		bdUI->setPositionChange(args);
	}
}
void tryBlink(std::monostate)
{
	BodyPartsUI* bdUI = BodyPartsUI::GetSingleton();
	if (bdUI) {
		bdUI->tryBlinkAction();
	}
}

void OnF4SEMessage(F4SE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case F4SE::MessagingInterface::kGameLoaded:
		{
			DH = RE::TESDataHandler::GetSingleton();
			p = PlayerCharacter::GetSingleton();
			ui = UI::GetSingleton();
			pcam = PlayerCamera::GetSingleton();

			gINIFileHotkeyEnable = (TESGlobal*)DH->LookupForm(0x80A, "PB_PlayerBlink.esp");
			g_fRange = (TESGlobal*)DH->LookupForm(0x810, "PB_PlayerBlink.esp");
			g_fBlinkDuration = (TESGlobal*)DH->LookupForm(0x80C, "PB_PlayerBlink.esp");
			g_iBlinkAPCost = (TESGlobal*)DH->LookupForm(0x80D, "PB_PlayerBlink.esp");

			blinkEffect = (TESEffectShader*)DH->LookupForm(0x801, "PB_PlayerBlink.esp");
			ActionPoints = (ActorValueInfo*)DH->LookupForm(0x2D5, "Fallout4.esm");

			// 실행 파일 경로를 구한 후 targetDirectory에 직접 할당
			char resultBuf[256];
			uint32_t tInt = GetModuleFileNameA(GetModuleHandle(NULL), resultBuf, sizeof(resultBuf));

			lootDir = std::string(resultBuf, tInt);
			lootDir = lootDir.substr(0, lootDir.find_last_of('\\')) + "\\Data\\F4SE\\Plugins\\";
			
			std::string fileName = "blinkHotkey.ini";
			filePath = lootDir + fileName;

			scriptName = new BSFixedString("PB_PlayerBlink_F4SE");
			functionName = new BSFixedString("pressINIFileKey");

			MenuWatcher* mw = new MenuWatcher();
			UI::GetSingleton()->GetEventSource<MenuOpenCloseEvent>()->RegisterSink(mw);

			RegisterMenu();
			setHUDPositionRatio();

			break;
		}
	}
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_vm)
{
	vm = a_vm;

	a_vm->BindNativeMethod("PB_PlayerBlink_F4SE"sv, "SetINIFileHotkey"sv, SetINIFileHotkey);
	a_vm->BindNativeMethod("PB_PlayerBlink_F4SE"sv, "sendPositionChange"sv, sendPositionChange);
	a_vm->BindNativeMethod("PB_PlayerBlink_F4SE"sv, "setFirstPosition"sv, setFirstPosition);
	a_vm->BindNativeMethod("PB_PlayerBlink_F4SE"sv, "tryBlink"sv, tryBlink);
	a_vm->BindNativeMethod("PB_PlayerBlink_F4SE"sv, "sendInvokeAction"sv, sendInvokeAction);

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("Global Log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::trace);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%^%l%$] %v"s);

	logger::info("{} v{}", Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	const F4SE::PapyrusInterface* papyrus = F4SE::GetPapyrusInterface();
	if (papyrus)
		papyrus->Register(RegisterPapyrusFunctions);

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	if (message)
		message->RegisterListener(OnF4SEMessage);

	return true;
}
