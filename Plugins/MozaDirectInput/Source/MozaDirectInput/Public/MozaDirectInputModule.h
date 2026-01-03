#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "IInputDeviceModule.h"

class FMozaDirectInputModule : public IInputDeviceModule
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    virtual TSharedPtr<IInputDevice> CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;
};

