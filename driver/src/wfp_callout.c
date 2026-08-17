#include "wfp_callout.h"
#include "kdamon_config.h"
#include "event_types.h"
#include "event_queue.h"
#include "log_writer.h"
#include "wfp_session.h"

#define KDAMON_WFP_CALLOUT_OUTBOUND_NAME        L"KDAMonitor Callout Outbound"
#define KDAMON_WFP_CALLOUT_OUTBOUND_DESCRIPTION L"KDAMonitor callout for outbound network event monitoring"
#define KDAMON_WFP_CALLOUT_INBOUND_NAME         L"KDAMonitor Callout Inbound"
#define KDAMON_WFP_CALLOUT_INBOUND_DESCRIPTION  L"KDAMonitor callout for inbound network event monitoring"

#define KDAMON_WFP_FILTER_OUTBOUND_NAME         L"KDAMonitor Filter Outbound"
#define KDAMON_WFP_FILTER_OUTBOUND_DESCRIPTION  L"KDAMonitor filter for outbound network event monitoring"
#define KDAMON_WFP_FILTER_INBOUND_NAME          L"KDAMonitor Filter Inbound"
#define KDAMON_WFP_FILTER_INBOUND_DESCRIPTION   L"KDAMonitor filter for inbound network event monitoring"

// --- Globals ---

static UINT32 g_WpsCalloutIdOutbound = 0;
static UINT32 g_WpsCalloutIdInbound = 0;
static UINT64 g_FilterIdOutbound = 0;
static UINT64 g_FilterIdInbound = 0;

// --- Classify common ---

static VOID KdaMonWfpClassifyCommon(
    _In_    const FWPS_INCOMING_VALUES0* inFixedValues,
    _In_    const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    _Inout_ FWPS_CLASSIFY_OUT0* classifyOut,
    _In_    KDAMON_NETWORK_DIRECTION               Direction,
    _In_    UINT32                                 FieldProtocol,
    _In_    UINT32                                 FieldLocalIp,
    _In_    UINT32                                 FieldLocalPort,
    _In_    UINT32                                 FieldRemoteIp,
    _In_    UINT32                                 FieldRemotePort
)
{
    KDAMON_EVENT Event = { 0 };

    Event.Type = KdaMonEventNetwork;
    KeQuerySystemTimePrecise(&Event.Timestamp);

    // --- PID ---
    Event.Data.Network.ProcessId = (HANDLE)inMetaValues->processId;

    // --- Process path ---
    if (inMetaValues->processPath &&
        inMetaValues->processPath->data &&
        inMetaValues->processPath->size > 0)
    {
        SIZE_T bytesToCopy = min(
            inMetaValues->processPath->size,
            (RTL_NUMBER_OF(Event.Data.Network.ProcessPath) - 1) * sizeof(WCHAR)
        );
        RtlCopyMemory(Event.Data.Network.ProcessPath,
            inMetaValues->processPath->data,
            bytesToCopy);
        Event.Data.Network.ProcessPath[bytesToCopy / sizeof(WCHAR)] = L'\0';
    }

    // --- Protocol ---
    Event.Data.Network.Protocol = (UINT8)inFixedValues->incomingValue[FieldProtocol].value.uint8;

    // --- IPs ---
    Event.Data.Network.LocalIp = inFixedValues->incomingValue[FieldLocalIp].value.uint32;
    Event.Data.Network.RemoteIp = inFixedValues->incomingValue[FieldRemoteIp].value.uint32;

    // --- Ports ---
    Event.Data.Network.LocalPort = inFixedValues->incomingValue[FieldLocalPort].value.uint16;
    Event.Data.Network.RemotePort = inFixedValues->incomingValue[FieldRemotePort].value.uint16;

    // --- Direction ---
    Event.Data.Network.Direction = Direction;

    KdaMonEventQueuePush(&Event);

    classifyOut->actionType = FWP_ACTION_CONTINUE;
}

// --- Outbound classify ---

static VOID KdaMonWfpClassifyFnOutbound(
    _In_        const FWPS_INCOMING_VALUES0* inFixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    _Inout_opt_ VOID* layerData,
    _In_opt_    const VOID* classifyContext,
    _In_        const FWPS_FILTER2* filter,
    _In_        UINT64                                flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0* classifyOut
)
{
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    KdaMonWfpClassifyCommon(
        inFixedValues, inMetaValues, classifyOut,
        KDAMON_NETWORK_DIRECTION_OUTBOUND,
        FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL,
        FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS,
        FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT,
        FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS,
        FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT
    );
}

// --- Inbound classify ---

static VOID KdaMonWfpClassifyFnInbound(
    _In_        const FWPS_INCOMING_VALUES0* inFixedValues,
    _In_        const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    _Inout_opt_ VOID* layerData,
    _In_opt_    const VOID* classifyContext,
    _In_        const FWPS_FILTER2* filter,
    _In_        UINT64                                flowContext,
    _Inout_     FWPS_CLASSIFY_OUT0* classifyOut
)
{
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    KdaMonWfpClassifyCommon(
        inFixedValues, inMetaValues, classifyOut,
        KDAMON_NETWORK_DIRECTION_INBOUND,
        FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL,
        FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS,
        FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT,
        FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS,
        FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT
    );
}

// --- Notify (stub) ---

static NTSTATUS KdaMonWfpNotifyFn(
    _In_    FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    _In_    const GUID* filterKey,
    _Inout_ FWPS_FILTER2* filter
)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    return STATUS_SUCCESS;
}

// --- Register / Unregister ---

NTSTATUS KdaMonWfpCalloutRegister(_In_ PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS      status;
    FWPS_CALLOUT2 callout_s = { 0 };
    FWPM_CALLOUT  callout_m = { 0 };
    FWPM_FILTER   filter = { 0 };

    // =========================================================
    // OUTBOUND — FWPM_LAYER_ALE_AUTH_CONNECT_V4
    // =========================================================

    RtlCopyMemory(&callout_s.calloutKey, &KDAMON_WFP_CALLOUT_OUTBOUND_GUID, sizeof(GUID));
    callout_s.flags = 0;
    callout_s.classifyFn = KdaMonWfpClassifyFnOutbound;
    callout_s.notifyFn = KdaMonWfpNotifyFn;
    callout_s.flowDeleteFn = NULL;

    status = FwpsCalloutRegister2(DeviceObject, &callout_s, &g_WpsCalloutIdOutbound);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: FwpsCalloutRegister2 (outbound) failed: 0x%X\n", status));
        return status;
    }

    RtlCopyMemory(&callout_m.calloutKey, &KDAMON_WFP_CALLOUT_OUTBOUND_GUID, sizeof(GUID));
    RtlCopyMemory(&callout_m.applicableLayer, &FWPM_LAYER_ALE_AUTH_CONNECT_V4, sizeof(GUID));
    callout_m.displayData.name = KDAMON_WFP_CALLOUT_OUTBOUND_NAME;
    callout_m.displayData.description = KDAMON_WFP_CALLOUT_OUTBOUND_DESCRIPTION;
    callout_m.flags = 0;

    status = FwpmCalloutAdd(g_EngineHandle, &callout_m, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: FwpmCalloutAdd (outbound) failed: 0x%X\n", status));
        KdaMonWfpCalloutUnregister();
        return status;
    }

    RtlZeroMemory(&filter, sizeof(filter));
    filter.displayData.name = KDAMON_WFP_FILTER_OUTBOUND_NAME;
    filter.displayData.description = KDAMON_WFP_FILTER_OUTBOUND_DESCRIPTION;
    filter.providerKey = (GUID*)&KDAMON_WFP_PROVIDER_GUID;
    filter.numFilterConditions = 0;
    filter.filterCondition = NULL;
    filter.action.type = FWP_ACTION_CALLOUT_INSPECTION;
    RtlCopyMemory(&filter.layerKey, &FWPM_LAYER_ALE_AUTH_CONNECT_V4, sizeof(GUID));
    RtlCopyMemory(&filter.subLayerKey, &KDAMON_WFP_SUBLAYER_GUID, sizeof(GUID));
    RtlCopyMemory(&filter.action.calloutKey, &KDAMON_WFP_CALLOUT_OUTBOUND_GUID, sizeof(GUID));

    status = FwpmFilterAdd(g_EngineHandle, &filter, NULL, &g_FilterIdOutbound);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: FwpmFilterAdd (outbound) failed: 0x%X\n", status));
        KdaMonWfpCalloutUnregister();
        return status;
    }

    KdPrint((DRIVER_TAG " [SUCCESS]: WFP outbound callout registered\n"));

    // =========================================================
    // INBOUND — FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4
    // =========================================================

    RtlZeroMemory(&callout_s, sizeof(callout_s));
    RtlCopyMemory(&callout_s.calloutKey, &KDAMON_WFP_CALLOUT_INBOUND_GUID, sizeof(GUID));
    callout_s.flags = 0;
    callout_s.classifyFn = KdaMonWfpClassifyFnInbound;
    callout_s.notifyFn = KdaMonWfpNotifyFn;
    callout_s.flowDeleteFn = NULL;

    status = FwpsCalloutRegister2(DeviceObject, &callout_s, &g_WpsCalloutIdInbound);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: FwpsCalloutRegister2 (inbound) failed: 0x%X\n", status));
        KdaMonWfpCalloutUnregister();
        return status;
    }

    RtlZeroMemory(&callout_m, sizeof(callout_m));
    RtlCopyMemory(&callout_m.calloutKey, &KDAMON_WFP_CALLOUT_INBOUND_GUID, sizeof(GUID));
    RtlCopyMemory(&callout_m.applicableLayer, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, sizeof(GUID));
    callout_m.displayData.name = KDAMON_WFP_CALLOUT_INBOUND_NAME;
    callout_m.displayData.description = KDAMON_WFP_CALLOUT_INBOUND_DESCRIPTION;
    callout_m.flags = 0;

    status = FwpmCalloutAdd(g_EngineHandle, &callout_m, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: FwpmCalloutAdd (inbound) failed: 0x%X\n", status));
        KdaMonWfpCalloutUnregister();
        return status;
    }

    RtlZeroMemory(&filter, sizeof(filter));
    filter.displayData.name = KDAMON_WFP_FILTER_INBOUND_NAME;
    filter.displayData.description = KDAMON_WFP_FILTER_INBOUND_DESCRIPTION;
    filter.providerKey = (GUID*)&KDAMON_WFP_PROVIDER_GUID;
    filter.numFilterConditions = 0;
    filter.filterCondition = NULL;
    filter.action.type = FWP_ACTION_CALLOUT_INSPECTION;
    RtlCopyMemory(&filter.layerKey, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, sizeof(GUID));
    RtlCopyMemory(&filter.subLayerKey, &KDAMON_WFP_SUBLAYER_GUID, sizeof(GUID));
    RtlCopyMemory(&filter.action.calloutKey, &KDAMON_WFP_CALLOUT_INBOUND_GUID, sizeof(GUID));

    status = FwpmFilterAdd(g_EngineHandle, &filter, NULL, &g_FilterIdInbound);
    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVER_TAG " [ERROR]: FwpmFilterAdd (inbound) failed: 0x%X\n", status));
        KdaMonWfpCalloutUnregister();
        return status;
    }

    KdPrint((DRIVER_TAG " [SUCCESS]: WFP inbound callout registered\n"));

    return STATUS_SUCCESS;
}

VOID KdaMonWfpCalloutUnregister(VOID)
{
    if (g_FilterIdInbound != 0)
    {
        FwpmFilterDeleteById(g_EngineHandle, g_FilterIdInbound);
        g_FilterIdInbound = 0;
    }

    if (g_FilterIdOutbound != 0)
    {
        FwpmFilterDeleteById(g_EngineHandle, g_FilterIdOutbound);
        g_FilterIdOutbound = 0;
    }

    if (g_WpsCalloutIdInbound != 0)
    {
        FwpmCalloutDeleteByKey(g_EngineHandle, &KDAMON_WFP_CALLOUT_INBOUND_GUID);
        FwpsCalloutUnregisterById(g_WpsCalloutIdInbound);
        g_WpsCalloutIdInbound = 0;
    }

    if (g_WpsCalloutIdOutbound != 0)
    {
        FwpmCalloutDeleteByKey(g_EngineHandle, &KDAMON_WFP_CALLOUT_OUTBOUND_GUID);
        FwpsCalloutUnregisterById(g_WpsCalloutIdOutbound);
        g_WpsCalloutIdOutbound = 0;
    }
}