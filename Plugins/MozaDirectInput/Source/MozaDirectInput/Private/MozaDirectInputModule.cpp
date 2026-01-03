#include "MozaDirectInputModule.h"
#include "MozaDirectInput.h"

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/IInputInterface.h"
#include "IInputDevice.h"
#include "InputCoreTypes.h"
#include "InputDevice.h"
#include "Misc/CoreMiscDefines.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl/client.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "FMozaDirectInputModule"

DEFINE_LOG_CATEGORY(LogMozaDirectInput);

#if PLATFORM_WINDOWS
namespace MozaDirectInput
{
    static float NormalizeAxis(LONG Value)
    {
        constexpr float Scale = 1.0f / 32767.0f;
        return FMath::Clamp(static_cast<float>(Value) * Scale, -1.0f, 1.0f);
    }

    class FMozaDirectInputDevice : public IInputDevice
    {
    public:
        FMozaDirectInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
            : MessageHandler(InMessageHandler)
            , PlatformUserId(FPlatformUserId::CreateFromInternalId(0))
            , InputDeviceId(FInputDeviceId::CreateFromInternalId(0))
            , bDeviceReady(false)
            , bHasFallbackDevice(false)
        {
            FMemory::Memzero(CurrentState);
            FMemory::Memzero(PreviousState);
            InitializeDirectInput();
        }

        ~FMozaDirectInputDevice()
        {
            if (WheelDevice)
            {
                WheelDevice->Unacquire();
                WheelDevice.Reset();
            }

            DirectInput.Reset();
        }

        virtual void Tick(float DeltaTime) override
        {
            PollState();
        }

        virtual void SendControllerEvents() override
        {
            DispatchEvents();
        }

        virtual void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override
        {
            MessageHandler = InMessageHandler;
        }

        virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
        {
            return false;
        }

        virtual void SetChannelValue(int32 PlayerId, FForceFeedbackChannelType ChannelType, float Value) override {}
        virtual void SetChannelValues(int32 PlayerId, const FForceFeedbackValues& Values) override {}

    private:
        void InitializeDirectInput()
        {
            const HRESULT HResult = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(DirectInput.GetAddressOf()), nullptr);
            if (FAILED(HResult))
            {
                UE_LOG(LogMozaDirectInput, Warning, TEXT("Failed to create DirectInput instance (%d)."), HResult);
                return;
            }

            const HRESULT EnumerateResult = DirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, &FMozaDirectInputDevice::EnumDevicesCallback, this, DIEDFL_ATTACHEDONLY);
            if (FAILED(EnumerateResult))
            {
                UE_LOG(LogMozaDirectInput, Warning, TEXT("Device enumeration failed (%d)."), EnumerateResult);
            }
            else if (!bDeviceReady)
            {
                if (bHasFallbackDevice)
                {
                    UE_LOG(LogMozaDirectInput, Log, TEXT("No Moza device found; falling back to first detected DirectInput device."));
                    TryInitializeDevice(&FallbackDevice, false /*bPreferDevice*/);
                }

                if (!bDeviceReady)
                {
                    UE_LOG(LogMozaDirectInput, Warning, TEXT("No compatible DirectInput devices were found."));
                }
            }
        }

        static BOOL CALLBACK EnumDevicesCallback(const DIDEVICEINSTANCE* Instance, VOID* Context)
        {
            FMozaDirectInputDevice* Device = static_cast<FMozaDirectInputDevice*>(Context);
            const FString ProductName(Instance->tszProductName);
            const bool bIsMozaWheel = ProductName.Contains(TEXT("MOZA"), ESearchCase::IgnoreCase);

            if (bIsMozaWheel)
            {
                return Device->TryInitializeDevice(Instance, true /*bPreferDevice*/) ? DIENUM_STOP : DIENUM_CONTINUE;
            }

            Device->RememberFallback(Instance);
            return DIENUM_CONTINUE;
        }

        void RememberFallback(const DIDEVICEINSTANCE* Instance)
        {
            if (bHasFallbackDevice || Instance == nullptr)
            {
                return;
            }

            FallbackDevice = *Instance;
            bHasFallbackDevice = true;
        }

        bool TryInitializeDevice(const DIDEVICEINSTANCE* Instance, bool bPreferDevice)
        {
            if (!DirectInput)
            {
                return false;
            }

            HRESULT HResult = DirectInput->CreateDevice(Instance->guidInstance, WheelDevice.ReleaseAndGetAddressOf(), nullptr);
            if (FAILED(HResult))
            {
                UE_LOG(LogMozaDirectInput, Warning, TEXT("Failed to create device (%d)."), HResult);
                return false;
            }

            HResult = WheelDevice->SetDataFormat(&c_dfDIJoystick2);
            if (FAILED(HResult))
            {
                UE_LOG(LogMozaDirectInput, Warning, TEXT("Failed to set data format (%d)."), HResult);
                WheelDevice.Reset();
                return false;
            }

            HResult = WheelDevice->SetCooperativeLevel(GetDesktopWindow(), DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
            if (FAILED(HResult))
            {
                UE_LOG(LogMozaDirectInput, Warning, TEXT("Failed to set cooperative level (%d)."), HResult);
                WheelDevice.Reset();
                return false;
            }

            WheelDevice->Acquire();
            bDeviceReady = true;
            if (bPreferDevice)
            {
                UE_LOG(LogMozaDirectInput, Log, TEXT("Initialized preferred Moza device: %s."), *FString(Instance->tszProductName));
            }
            else
            {
                UE_LOG(LogMozaDirectInput, Log, TEXT("Initialized fallback DirectInput device: %s."), *FString(Instance->tszProductName));
            }
            return true;
        }

        bool PollDevice()
        {
            if (!WheelDevice)
            {
                return false;
            }

            HRESULT HResult = WheelDevice->Poll();
            if (FAILED(HResult))
            {
                WheelDevice->Acquire();
                HResult = WheelDevice->Poll();
            }

            if (SUCCEEDED(HResult))
            {
                HResult = WheelDevice->GetDeviceState(sizeof(DIJOYSTATE2), &CurrentState);
            }

            return SUCCEEDED(HResult);
        }

        void PollState()
        {
            if (!bDeviceReady)
            {
                return;
            }

            if (!PollDevice())
            {
                return;
            }
        }

        void DispatchEvents()
        {
            if (!bDeviceReady)
            {
                return;
            }

            // Axis mappings
            DispatchAxis(EKeys::Gamepad_LeftX, PreviousState.lX, CurrentState.lX);
            DispatchAxis(EKeys::Gamepad_LeftY, PreviousState.lY, CurrentState.lY);
            DispatchAxis(EKeys::Gamepad_RightTrigger, PreviousState.lZ, CurrentState.lZ);
            DispatchAxis(EKeys::Gamepad_LeftTrigger, PreviousState.lRz, CurrentState.lRz);

            // Buttons
            for (int32 ButtonIndex = 0; ButtonIndex < 32; ++ButtonIndex)
            {
                const bool bWasPressed = (PreviousState.rgbButtons[ButtonIndex] & 0x80) != 0;
                const bool bIsPressed = (CurrentState.rgbButtons[ButtonIndex] & 0x80) != 0;
                if (bIsPressed != bWasPressed)
                {
                    const FKey ButtonKey = FGamepadKeyNames::FaceButtonBottom;
                    if (bIsPressed)
                    {
                        MessageHandler->OnControllerButtonPressed(ButtonKey.GetFName(), PlatformUserId, InputDeviceId, /*IsRepeat=*/false);
                    }
                    else
                    {
                        MessageHandler->OnControllerButtonReleased(ButtonKey.GetFName(), PlatformUserId, InputDeviceId, /*IsRepeat=*/false);
                    }
                }
            }

            PreviousState = CurrentState;
        }

        void DispatchAxis(const FKey& Key, LONG PreviousValue, LONG CurrentValue)
        {
            if (PreviousValue != CurrentValue)
            {
                const float Normalized = NormalizeAxis(CurrentValue);
                MessageHandler->OnControllerAnalog(Key.GetFName(), PlatformUserId, InputDeviceId, Normalized);
            }
        }

    private:
        Microsoft::WRL::ComPtr<IDirectInput8> DirectInput;
        Microsoft::WRL::ComPtr<IDirectInputDevice8> WheelDevice;
        TSharedRef<FGenericApplicationMessageHandler> MessageHandler;
        FPlatformUserId PlatformUserId;
        FInputDeviceId InputDeviceId;
        DIJOYSTATE2 PreviousState;
        DIJOYSTATE2 CurrentState;
        bool bDeviceReady;
        bool bHasFallbackDevice;
        DIDEVICEINSTANCE FallbackDevice;
    };
}
#endif // PLATFORM_WINDOWS

void FMozaDirectInputModule::StartupModule()
{
}

void FMozaDirectInputModule::ShutdownModule()
{
}

TSharedPtr<IInputDevice> FMozaDirectInputModule::CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
#if PLATFORM_WINDOWS
    return MakeShared<MozaDirectInput::FMozaDirectInputDevice>(InMessageHandler);
#else
    return nullptr;
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMozaDirectInputModule, MozaDirectInput)
