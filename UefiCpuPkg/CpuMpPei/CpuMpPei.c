/** @file
  CPU PEI Module installs CPU Multiple Processor PPI.

  Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuMpPei.h"

extern EDKII_PEI_MP_SERVICES2_PPI  mMpServices2Ppi;

//
// CPU MP PPI to be installed
//
EFI_PEI_MP_SERVICES_PPI  mMpServicesPpi = {
  PeiGetNumberOfProcessors,
  PeiGetProcessorInfo,
  PeiStartupAllAPs,
  PeiStartupThisAP,
  PeiSwitchBSP,
  PeiEnableDisableAP,
  PeiWhoAmI,
  PeiStartupThisAPNonBlocking,
};

EFI_PEI_PPI_DESCRIPTOR  mPeiCpuMpPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEdkiiPeiMpServices2PpiGuid,
    &mMpServices2Ppi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMpServicesPpiGuid,
    &mMpServicesPpi
  }
};

/**
  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  This function is used to retrieve the following information:
    - The number of logical processors that are present in the system.
    - The number of enabled logical processors in the system at the instant
      this call is made.

  Because MP Service Ppi provides services to enable and disable processors
  dynamically, the number of enabled logical processors may vary during the
  course of a boot session.

  If this service is called from an AP, then EFI_DEVICE_ERROR is returned.
  If NumberOfProcessors or NumberOfEnabledProcessors is NULL, then
  EFI_INVALID_PARAMETER is returned. Otherwise, the total number of processors
  is returned in NumberOfProcessors, the number of currently enabled processor
  is returned in NumberOfEnabledProcessors, and EFI_SUCCESS is returned.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                Pointer to this instance of the PPI.
  @param[out] NumberOfProcessors  Pointer to the total number of logical processors in
                                  the system, including the BSP and disabled APs.
  @param[out] NumberOfEnabledProcessors
                                  Number of processors in the system that are enabled.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL.
                                  NumberOfEnabledProcessors is NULL.
**/
EFI_STATUS
EFIAPI
PeiGetNumberOfProcessors (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  OUT UINTN                    *NumberOfProcessors,
  OUT UINTN                    *NumberOfEnabledProcessors
  )
{
  if ((NumberOfProcessors == NULL) || (NumberOfEnabledProcessors == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return MpInitLibGetNumberOfProcessors (
           NumberOfProcessors,
           NumberOfEnabledProcessors
           );
}

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  This service retrieves detailed MP-related information about any processor
  on the platform. Note the following:
    - The processor information may change during the course of a boot session.
    - The information presented here is entirely MP related.

  Information regarding the number of caches and their sizes, frequency of operation,
  slot numbers is all considered platform-related information and is not provided
  by this service.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                Pointer to this instance of the PPI.
  @param[in]  ProcessorNumber     Pointer to the total number of logical processors in
                                  the system, including the BSP and disabled APs.
  @param[out] ProcessorInfoBuffer Number of processors in the system that are enabled.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
**/
EFI_STATUS
EFIAPI
PeiGetProcessorInfo (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI    *This,
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
  )
{
  return MpInitLibGetProcessorInfo (ProcessorNumber, ProcessorInfoBuffer, NULL);
}

/**
  This service executes a caller provided function on all enabled APs. APs can
  run either simultaneously or one at a time in sequence. This service supports
  both blocking requests only. This service may only
  be called from the BSP.

  This function is used to dispatch all the enabled APs to the function specified
  by Procedure.  If any enabled AP is busy, then EFI_NOT_READY is returned
  immediately and Procedure is not started on any AP.

  If SingleThread is TRUE, all the enabled APs execute the function specified by
  Procedure one by one, in ascending order of processor handle number. Otherwise,
  all the enabled APs execute the function specified by Procedure simultaneously.

  If the timeout specified by TimeoutInMicroSeconds expires before all APs return
  from Procedure, then Procedure on the failed APs is terminated. All enabled APs
  are always available for further calls to EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
  and EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If FailedCpuList is not NULL, its
  content points to the list of processor handle numbers in which Procedure was
  terminated.

  Note: It is the responsibility of the consumer of the EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
  to make sure that the nature of the code that is executed on the BSP and the
  dispatched APs is well controlled. The MP Services Ppi does not guarantee
  that the Procedure function is MP-safe. Hence, the tasks that can be run in
  parallel are limited to certain independent tasks and well-controlled exclusive
  code. PEI services and Ppis may not be called by APs unless otherwise
  specified.

  In blocking execution mode, BSP waits until all APs finish or
  TimeoutInMicroSeconds expires.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] SingleThread         If TRUE, then all the enabled APs execute the function
                                  specified by Procedure one by one, in ascending order
                                  of processor handle number. If FALSE, then all the
                                  enabled APs execute the function specified by Procedure
                                  simultaneously.
  @param[in] TimeoutInMicroSeconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If the
                                  timeout expires in blocking mode, BSP returns
                                  EFI_TIMEOUT.
  @param[in] ProcedureArgument    The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before the
                                  timeout expired.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_STARTED         No enabled APs exist in the system.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before all
                                  enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
PeiStartupAllAPs (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  IN  EFI_AP_PROCEDURE         Procedure,
  IN  BOOLEAN                  SingleThread,
  IN  UINTN                    TimeoutInMicroSeconds,
  IN  VOID                     *ProcedureArgument      OPTIONAL
  )
{
  return MpInitLibStartupAllAPs (
           Procedure,
           SingleThread,
           NULL,
           TimeoutInMicroSeconds,
           ProcedureArgument,
           NULL
           );
}

/**
  This service lets the caller get one enabled AP to execute a caller-provided
  function. The caller can request the BSP to wait for the completion
  of the AP. This service may only be called from the BSP.

  This function is used to dispatch one enabled AP to the function specified by
  Procedure passing in the argument specified by ProcedureArgument.
  The execution is in blocking mode. The BSP waits until the AP finishes or
  TimeoutInMicroSecondss expires.

  If the timeout specified by TimeoutInMicroseconds expires before the AP returns
  from Procedure, then execution of Procedure by the AP is terminated. The AP is
  available for subsequent calls to EFI_PEI_MP_SERVICES_PPI.StartupAllAPs() and
  EFI_PEI_MP_SERVICES_PPI.StartupThisAP().

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] TimeoutInMicroseconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If the
                                  timeout expires in blocking mode, BSP returns
                                  EFI_TIMEOUT.
  @param[in] ProcedureArgument    The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS             In blocking mode, specified AP finished before the
                                  timeout expires.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before the
                                  specified AP has finished.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
PeiStartupThisAP (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  IN  EFI_AP_PROCEDURE         Procedure,
  IN  UINTN                    ProcessorNumber,
  IN  UINTN                    TimeoutInMicroseconds,
  IN  VOID                     *ProcedureArgument      OPTIONAL
  )
{
  return MpInitLibStartupThisAP (
           Procedure,
           ProcessorNumber,
           NULL,
           TimeoutInMicroseconds,
           ProcedureArgument,
           NULL
           );
}

// MU_CHANGE - Add basic support for non-blocking AP dispatch in PEI.

/**
  This service lets the caller get one enabled AP to execute a caller-provided
  function. This service may only be called from the BSP.

  This function is used to dispatch one enabled AP to the function specified by
  Procedure passing in the argument specified by ProcedureArgument.
  The execution is in non-blocking mode. The BSP continues executing immediately
  after starting the AP.

  If an attempt is made to dispatch a blocking or non-blcoking task on the AP
  while it is running a non-blocking task, that dispatch will block until the
  AP completes the current task.

  No timeout is specified - failure of the AP to complete the task is fatal. If
  the AP crashes or fails to return from Procedure, then the next attempt to
  dispatch blocking or non-blocking tasks on the AP will hang waiting on the AP.
  No attempt is made to reset or recover the AP in this state.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] ProcedureArgument    The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS             Indicates that the procedure was successfully
                                  started on the AP
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
PeiStartupThisAPNonBlocking (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  IN  EFI_AP_PROCEDURE         Procedure,
  IN  UINTN                    ProcessorNumber,
  IN  VOID                     *ProcedureArgument      OPTIONAL
  )
{
  // Note: MpLib uses WaitEvent!=NULL as a trigger to execute in non-blocking
  // mode but delegates actual usage of it as an event to the DxeMpLib wrapper.
  // So we set it to '1' to allow startup of the AP in non-blocking mode, but
  // in the PEI implementation its role is simply as a boolean flag indicating
  // non-blocking mode - it is not an actual "Event" in the DXE sense.
  return MpInitLibStartupThisAP (
           Procedure,
           ProcessorNumber,
           (EFI_EVENT)1,
           0,
           ProcedureArgument,
           NULL
           );
}

// MU_CHANGE - End Add basic support for non-blocking AP dispatch in PEI.

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes.   This call can only be performed
  by the current BSP.

  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. The new BSP can take over the
  execution of the old BSP and continue seamlessly from where the old one left
  off.

  If the BSP cannot be switched prior to the return from this service, then
  EFI_UNSUPPORTED must be returned.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] EnableOldBSP         If TRUE, then the old BSP will be listed as an enabled
                                  AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to this
                                  service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or a disabled
                                  AP.
  @retval EFI_NOT_READY           The specified AP is busy.
**/
EFI_STATUS
EFIAPI
PeiSwitchBSP (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  IN  UINTN                    ProcessorNumber,
  IN  BOOLEAN                  EnableOldBSP
  )
{
  return MpInitLibSwitchBSP (ProcessorNumber, EnableOldBSP);
}

/**
  This service lets the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.

  This service allows the caller enable or disable an AP from this point onward.
  The caller can optionally specify the health status of the AP by Health. If
  an AP is being disabled, then the state of the disabled AP is implementation
  dependent. If an AP is enabled, then the implementation must guarantee that a
  complete initialization sequence is performed on the AP, so the AP is in a state
  that is compatible with an MP operating system.

  If the enable or disable AP operation cannot be completed prior to the return
  from this service, then EFI_UNSUPPORTED must be returned.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] EnableAP             Specifies the new state for the processor for enabled,
                                  FALSE for disabled.
  @param[in] HealthFlag           If not NULL, a pointer to a value that specifies the
                                  new health status of the AP. This flag corresponds to
                                  StatusFlag defined in EFI_PEI_MP_SERVICES_PPI.GetProcessorInfo().
                                  Only the PROCESSOR_HEALTH_STATUS_BIT is used. All other
                                  bits are ignored. If it is NULL, this parameter is
                                  ignored.

  @retval EFI_SUCCESS             The specified AP was enabled or disabled successfully.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP cannot be completed prior
                                  to this service returning.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           Processor with the handle specified by ProcessorNumber
                                  does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
**/
EFI_STATUS
EFIAPI
PeiEnableDisableAP (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  IN  UINTN                    ProcessorNumber,
  IN  BOOLEAN                  EnableAP,
  IN  UINT32                   *HealthFlag OPTIONAL
  )
{
  return MpInitLibEnableDisableAP (ProcessorNumber, EnableAP, HealthFlag);
}

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  This service returns the processor handle number for the calling processor.
  The returned value is in the range from 0 to the total number of logical
  processors minus 1. The total number of logical processors can be retrieved
  with EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors(). This service may be
  called from the BSP and APs. If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[out] ProcessorNumber     The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned in
                                  ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
**/
EFI_STATUS
EFIAPI
PeiWhoAmI (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  OUT UINTN                    *ProcessorNumber
  )
{
  return MpInitLibWhoAmI (ProcessorNumber);
}

//
// Structure for InitializeSeparateExceptionStacks
//
typedef struct {
  VOID          *Buffer;
  UINTN         BufferSize;
  EFI_STATUS    Status;
} EXCEPTION_STACK_SWITCH_CONTEXT;

/**
  Initializes CPU exceptions handlers for the sake of stack switch requirement.

  This function is a wrapper of InitializeSeparateExceptionStacks. It's mainly
  for the sake of AP's init because of EFI_AP_PROCEDURE API requirement.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
InitializeExceptionStackSwitchHandlers (
  IN OUT VOID  *Buffer
  )
{
  EXCEPTION_STACK_SWITCH_CONTEXT  *SwitchStackData;
  UINTN                           Index;

  MpInitLibWhoAmI (&Index);
  SwitchStackData = (EXCEPTION_STACK_SWITCH_CONTEXT *)Buffer;
  //
  // This function may be called twice for each Cpu. Only run InitializeSeparateExceptionStacks
  // if this is the first call or the first call failed because of size too small.
  //
  if ((SwitchStackData[Index].Status == EFI_NOT_STARTED) || (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL)) {
    SwitchStackData[Index].Status = InitializeSeparateExceptionStacks (SwitchStackData[Index].Buffer, &SwitchStackData[Index].BufferSize);
  }
}

/**
  Initializes MP exceptions handlers for the sake of stack switch requirement.

  This function will allocate required resources required to setup stack switch
  and pass them through SwitchStackData to each logic processor.

**/
VOID
InitializeMpExceptionStackSwitchHandlers (
  VOID
  )
{
  UINTN                           Index;
  UINTN                           NumberOfProcessors;
  EXCEPTION_STACK_SWITCH_CONTEXT  *SwitchStackData;
  UINTN                           BufferSize;
  EFI_STATUS                      Status;
  UINT8                           *Buffer;

  // MU_CHANGE START
  // if (!PcdGetBool (PcdCpuStackGuard)) {
  //  return;
  // }
  // MU_CHANGE END

  // MU_CHANGE [BEGIN] - CodeQL change
  Status = MpInitLibGetNumberOfProcessors (&NumberOfProcessors, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_ERROR, "%a - Failed to get number of processors.  Status = %r\n", __FUNCTION__, Status));
    return;
  }

  // MU_CHANGE [END] - CodeQL change

  SwitchStackData = AllocatePages (EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT)));
  // MU_CHANGE [BEGIN] - CodeQL change
  if (SwitchStackData == NULL) {
    ASSERT (SwitchStackData != NULL);
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate Switch Stack pages.\n", __FUNCTION__));
    return;
  }

  // MU_CHANGE [END] - CodeQL change
  ZeroMem (SwitchStackData, NumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT));
  for (Index = 0; Index < NumberOfProcessors; ++Index) {
    //
    // Because the procedure may runs multiple times, use the status EFI_NOT_STARTED
    // to indicate the procedure haven't been run yet.
    //
    SwitchStackData[Index].Status = EFI_NOT_STARTED;
  }

  Status = MpInitLibStartupAllCPUs (
             InitializeExceptionStackSwitchHandlers,
             0,
             SwitchStackData
             );
  ASSERT_EFI_ERROR (Status);

  BufferSize = 0;
  for (Index = 0; Index < NumberOfProcessors; ++Index) {
    if (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL) {
      ASSERT (SwitchStackData[Index].BufferSize != 0);
      BufferSize += SwitchStackData[Index].BufferSize;
    } else {
      ASSERT (SwitchStackData[Index].Status == EFI_SUCCESS);
      ASSERT (SwitchStackData[Index].BufferSize == 0);
    }
  }

  if (BufferSize != 0) {
    Buffer = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
    // MU_CHANGE [BEGIN] - CodeQL change
    if (Buffer == NULL) {
      ASSERT (Buffer != NULL);
      DEBUG ((DEBUG_ERROR, "%a - Failed to allocate Buffer pages.\n", __FUNCTION__));
      return;
    }

    // MU_CHANGE [END] - CodeQL change
    BufferSize = 0;
    for (Index = 0; Index < NumberOfProcessors; ++Index) {
      if (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL) {
        SwitchStackData[Index].Buffer = (VOID *)(&Buffer[BufferSize]);
        BufferSize                   += SwitchStackData[Index].BufferSize;
        DEBUG ((
          DEBUG_INFO,
          "Buffer[cpu%lu] for InitializeExceptionStackSwitchHandlers: 0x%lX with size 0x%lX\n",
          (UINT64)(UINTN)Index,
          (UINT64)(UINTN)SwitchStackData[Index].Buffer,
          (UINT64)(UINTN)SwitchStackData[Index].BufferSize
          ));
      }
    }

    Status = MpInitLibStartupAllCPUs (
               InitializeExceptionStackSwitchHandlers,
               0,
               SwitchStackData
               );
    ASSERT_EFI_ERROR (Status);
    for (Index = 0; Index < NumberOfProcessors; ++Index) {
      ASSERT (SwitchStackData[Index].Status == EFI_SUCCESS);
    }
  }

  FreePages (SwitchStackData, EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT)));
}

/**
  Initializes MP and exceptions handlers.

  @param  PeiServices                The pointer to the PEI Services Table.

  @retval EFI_SUCCESS     MP was successfully initialized.
  @retval others          Error occurred in MP initialization.

**/
EFI_STATUS
InitializeCpuMpWorker (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_VECTOR_HANDOFF_INFO          *VectorInfo;
  EFI_PEI_VECTOR_HANDOFF_INFO_PPI  *VectorHandoffInfoPpi;

  //
  // Get Vector Hand-off Info PPI
  //
  VectorInfo = NULL;
  Status     = PeiServicesLocatePpi (
                 &gEfiVectorHandoffInfoPpiGuid,
                 0,
                 NULL,
                 (VOID **)&VectorHandoffInfoPpi
                 );
  if (Status == EFI_SUCCESS) {
    VectorInfo = VectorHandoffInfoPpi->Info;
  }

  //
  // Initialize default handlers
  //
  Status = InitializeCpuExceptionHandlers (VectorInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MpInitLibInitialize ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Special initialization for the sake of Stack Guard
  //
  InitializeMpExceptionStackSwitchHandlers ();

  //
  // Update and publish CPU BIST information
  //
  CollectBistDataFromPpi (PeiServices);

  //
  // Install CPU MP PPI
  //
  Status = PeiServicesInstallPpi (mPeiCpuMpPpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The Entry point of the MP CPU PEIM.

  This function will wakeup APs and collect CPU AP count and install the
  Mp Service Ppi.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   MpServicePpi is installed successfully.

**/
EFI_STATUS
EFIAPI
CpuMpPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  //
  // For the sake of special initialization needing to be done right after
  // memory discovery.
  //
  Status = PeiServicesNotifyPpi (&mPostMemNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
