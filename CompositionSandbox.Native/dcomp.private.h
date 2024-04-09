#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <windows.ui.composition.h>
#include <dcomp.h>

DECLARE_INTERFACE_IID_(IVisualTargetPartner, IUnknown, "DBA1813C-60C5-4A42-A4D2-3380CDDCE8A1")
{
    IFACEMETHOD(GetRoot)(ABI::Windows::UI::Composition::IVisual** rootVisual) PURE;
    IFACEMETHOD(SetRoot)(ABI::Windows::UI::Composition::IVisual* rootVisual) PURE;
};

// 19041+
DECLARE_INTERFACE_IID_(ICompositorPartner, IUnknown, "9CBD9312-070D-4588-9BF3-BBF528CF3E84")
{
    IFACEMETHOD(CreateCursorVisual)(unsigned __int64, IUnknown** cursorVisual) PURE;
    IFACEMETHOD(CreateSharedTarget)(IVisualTargetPartner** target) PURE;
    IFACEMETHOD(CreateSharedVisual)(ABI::Windows::UI::Composition::IVisual** visual) PURE;
    IFACEMETHOD(HintSize)(unsigned int w, unsigned int h) PURE;
    IFACEMETHOD(OfferSurfaceResources)() PURE;
    IFACEMETHOD(OpenSharedResourceHandle)(ABI::Windows::UI::Composition::ICompositionObject* object, HANDLE* handle) PURE;
    IFACEMETHOD(OpenSharedTargetFromHandle)(HANDLE handle, IVisualTargetPartner** target) PURE;
    IFACEMETHOD(OpenSharedTargetFromVisual)(ABI::Windows::UI::Composition::IVisual* visual, IVisualTargetPartner** target) PURE;
    IFACEMETHOD(OpenSharedVisualFromHandle)(HANDLE handle, ABI::Windows::UI::Composition::IVisual** visual) PURE;
    IFACEMETHOD(OpenSharedVisualFromTarget)(IVisualTargetPartner* target, ABI::Windows::UI::Composition::IVisual** visual) PURE;
    IFACEMETHOD(OpenSharedVisualReferenceFromHandle)(HANDLE handle, IInspectable** visualRef) PURE;
};

// TODO: add interfaces for 15063-18890