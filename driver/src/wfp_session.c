#define INITGUID

#include "wfp_session.h"
#include "kdamon_config.h"

DEFINE_GUID(KDAMON_WFP_PROVIDER_GUID,
    0x16821234, 0xd300, 0x42f1,
    0xbc, 0xe8, 0xd2, 0x23, 0x1a, 0xf3, 0x25, 0xc3);

DEFINE_GUID(KDAMON_WFP_SUBLAYER_GUID,
    0xa141444c, 0x7f15, 0x4a05,
    0xa2, 0x95, 0x07, 0xca, 0x38, 0xc2, 0x3c, 0xb1);

#define KDAMON_WFP_PROVIDER_NAME    L"KDAMonitor Provider"
#define KDAMON_WFP_PROVIDER_DESCRIPTION L"KDAMonitor - Kernel Driver Activity Monitor"

#define KDAMON_WFP_SUBLAYER_NAME    L"KDAMonitor Sublayer"
#define KDAMON_WFP_SUBLAYER_DESCRIPTION L"KDAMonitor sublayer for network event monitoring"

static HANDLE g_EngineHandle = NULL;

NTSTATUS KdaMonWfpSessionInit(void)
{
	NTSTATUS status;
    FWPM_SESSION wfpSession = { 0 };
	FWPM_PROVIDER provider = { 0 };
	FWPM_SUBLAYER subLayer = { 0 };

	status = FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, &wfpSession, &g_EngineHandle);
	if (status != STATUS_SUCCESS)
	{
		KdPrint((DRIVER_TAG " [ERROR]: FwpmEngineOpen failed with status 0x%X\n", status));
		return STATUS_UNSUCCESSFUL;
	}

	provider.providerKey = KDAMON_WFP_PROVIDER_GUID;
	provider.displayData.name = KDAMON_WFP_PROVIDER_NAME;
	provider.displayData.description = KDAMON_WFP_PROVIDER_DESCRIPTION;
	provider.flags = 0;
	provider.serviceName = NULL;
	status = FwpmProviderAdd(g_EngineHandle, &provider, NULL);
	if (status != STATUS_SUCCESS)
	{
		KdPrint((DRIVER_TAG " [ERROR]: FwpmProviderAdd failed with status 0x%X\n", status));
		FwpmEngineClose(g_EngineHandle);
		g_EngineHandle = NULL;
		return STATUS_UNSUCCESSFUL;
	}

	subLayer.subLayerKey = KDAMON_WFP_SUBLAYER_GUID;
	GUID providerKey = KDAMON_WFP_PROVIDER_GUID;
	subLayer.providerKey = &providerKey;
	subLayer.displayData.name = KDAMON_WFP_SUBLAYER_NAME;
	subLayer.displayData.description = KDAMON_WFP_SUBLAYER_DESCRIPTION;
	subLayer.flags = 0;
	subLayer.weight = (UINT16)0;
	status = FwpmSubLayerAdd(g_EngineHandle, &subLayer, NULL);
	if (status != STATUS_SUCCESS)
	{
		KdPrint((DRIVER_TAG " [ERROR]: FwpmSubLayerAdd failed with status 0x%X\n", status));
		FwpmProviderDeleteByKey(g_EngineHandle, &KDAMON_WFP_PROVIDER_GUID);
		FwpmEngineClose(g_EngineHandle);
		g_EngineHandle = NULL;
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

VOID KdaMonWfpSessionCleanup(void)
{
	NTSTATUS status;
	if (g_EngineHandle == NULL)
		return;

	status = FwpmSubLayerDeleteByKey(g_EngineHandle, &KDAMON_WFP_SUBLAYER_GUID);
	if (status != STATUS_SUCCESS)
	{
		KdPrint((DRIVER_TAG " [ERROR]: FwpmSubLayerDeleteByKey failed with status 0x%X\n", status));
	}

	status = FwpmProviderDeleteByKey(g_EngineHandle, &KDAMON_WFP_PROVIDER_GUID);
	if (status != STATUS_SUCCESS)
	{
		KdPrint((DRIVER_TAG " [ERROR]: FwpmProviderDeleteByKey failed with status 0x%X\n", status));
	}

	status = FwpmEngineClose(g_EngineHandle);
	if (status != STATUS_SUCCESS)
	{
		KdPrint((DRIVER_TAG " [ERROR]: FwpmEngineClose failed with status 0x%X\n", status));
	}

	g_EngineHandle = NULL;

	KdPrint((DRIVER_TAG " [SUCCESS]: WFP session closed successfully\n"));
}