#ifndef OPENVR_CAPI_GLOBAL_H_
#define OPENVR_CAPI_GLOBAL_H_

// Global entry points
extern intptr_t VR_InitInternal( EVRInitError *peError, EVRApplicationType eType );
extern void VR_ShutdownInternal();
extern bool VR_IsHmdPresent();
extern intptr_t VR_GetGenericInterface( const char *pchInterfaceVersion, EVRInitError *peError );
extern bool VR_IsRuntimeInstalled();
extern const char * VR_GetVRInitErrorAsSymbol( EVRInitError error );
extern const char * VR_GetVRInitErrorAsEnglishDescription( EVRInitError error );

#endif /* OPENVR_CAPI_GLOBAL_H_ */
