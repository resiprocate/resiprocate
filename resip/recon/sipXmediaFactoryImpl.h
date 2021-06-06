//
// Copyright (C) 2005-2010 SIPez LLC.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// Copyright (C) 2004-2008 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef _sipXmediaFactoryImpl_h_
#define _sipXmediaFactoryImpl_h_

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include "mi/CpMediaInterfaceFactoryImpl.h"
#include <rtcp/RtcpConfig.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
class MpMediaTask ;
class OsConfigDb ; 
#ifdef INCLUDE_RTCP /* [ */
struct IRTCPControl ;
#endif /* INCLUDE_RTCP ] */


/**
 *
 */
class sipXmediaFactoryImpl : public CpMediaInterfaceFactoryImpl
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
  public:

/* ============================ CREATORS ================================== */

     /// @brief Default constructor
   sipXmediaFactoryImpl(OsConfigDb* pConfigDb, 
                        uint32_t frameSizeMs,
                        uint32_t maxSamplesPerSec,
                        uint32_t defaultSamplesPerSec,
                        UtlBoolean enableLocalAudio,
                        const UtlString &inputDeviceName,
                        const UtlString &outputDeviceName);
     /**<
     *  @param pConfigDb - a configuration database to pass user-settable config
     *         parameters to sipXmediaLib.  TODO: Someone that knows more, document this better!
     *  @param frameSizeMs - This parameter is used for determining the 
     *         frame size (in milliseconds) that the media subsystem will use.
     *         It is used for initializing the size of audio buffers, and for 
     *         configuring a default value for samples per frame in device 
     *         managers (so that when devices are enabled without specifying 
     *         samples per frame, the value configured here will be used).
     *  @param maxSamplesPerSec - This is used for initializing audio buffers.
     *         Lower sample rates can indeed be used for individual media 
     *         interfaces (set later), since a lesser amount of these buffers 
     *         can be used (i.e. not fully utilized).  Higher sample rates can 
     *         be used by passing params here, but this will result in more 
     *         memory being used.  For low-memory environments that do not 
     *         require wideband support, one may wish to pass 8000kHz here, as 
     *         the default is 16000kHz.
     *  @param defaultSamplesPerSec - The sample rate that device managers and 
     *         flowgraphs will use when no sample rate is specified.
     *  @param enableLocalAudio - If TRUE, local sound card will be used to play
     *         audio and provide heartbeat for media processing. If FALSE, local
     *         sound card will not be used and high-res timer will be used for
     *         media processing heartbeat. Setting to FALSE is useful for server
     *         use case.
     *  @param inputDeviceName - Name of the audio device to use as input device
     *         during a call. Use empty string to select default (OS-dependent)
     *         device.
     *  @param outputDeviceName - Name of the audio device to use as output device
     *         during a call. Use empty string to select default (OS-dependent)
     *         device.
     */
     

   /**
    * Destructor
    */
   virtual ~sipXmediaFactoryImpl();

/* ============================ MANIPULATORS ============================== */
    virtual CpMediaInterface* createMediaInterface( const char* publicAddress,
                                                    const char* localAddress,
                                                    int numCodecs,
                                                    SdpCodec* sdpCodecArray[],
                                                    const char* locale,
                                                    int expeditedIpTos,
                                                    const char* szStunServer,
                                                    int stunOptions,
                                                    int iStunKeepAliveSecs,
                                                    const char* szTurnServer,
                                                    int iTurnPort,
                                                    const char* szTurnUsername,
                                                    const char* szTurnPassword,
                                                    int iTurnKeepAlivePeriodSecs,
                                                    UtlBoolean bEnableICE, 
                                                    uint32_t samplesPerSec,
                                                    OsMsgDispatcher* pDispatcher)
#ifdef DISABLE_DEFAULT_PHONE_MEDIA_INTERFACE_FACTORY
   = 0
#endif
      ;

    virtual OsStatus setSpeakerVolume(int iVolume) ;
    virtual OsStatus setSpeakerDevice(const UtlString& device) ;

    virtual OsStatus setMicrophoneGain(int iGain) ;
    virtual OsStatus setMicrophoneDevice(const UtlString& device) ;
    virtual OsStatus muteMicrophone(UtlBoolean bMute) ;
    virtual OsStatus setAudioAECMode(const MEDIA_AEC_MODE mode) ;
    virtual OsStatus enableAGC(UtlBoolean bEnable) ;
    virtual OsStatus setAudioNoiseReductionMode(const MEDIA_NOISE_REDUCTION_MODE mode) ;

    /// Populate the codec factory, return number of rejected codecs.
    virtual OsStatus buildCodecFactory(SdpCodecList*     pFactory, 
                                       const UtlString& sPreferences,
                                       const UtlString& sVideoPreferences,
                                       int videoFormat,
                                       int* iRejected);
    /**<
    *  Note, that current implementation ignore \p videoFormat parameter.
    */

    virtual OsStatus updateVideoPreviewWindow(void* displayContext) ;

    /**
     * Set the global video preview window 
     */ 
    virtual OsStatus setVideoPreviewDisplay(void* pDisplay);


    virtual OsStatus setVideoQuality(int quality);
    virtual OsStatus setVideoParameters(int bitRate, int frameRate);
    

/* ============================ ACCESSORS ================================= */

    virtual OsStatus getSpeakerVolume(int& iVolume) const  ;
    virtual OsStatus getSpeakerDevice(UtlString& device) const ;
    virtual OsStatus getMicrophoneGain(int& iVolume) const ;
    virtual OsStatus getMicrophoneDevice(UtlString& device) const ;

    virtual OsStatus getLocalAudioConnectionId(int& connectionId) const ;

    virtual OsStatus getVideoQuality(int& quality) const;
    virtual OsStatus getVideoBitRate(int& bitRate) const;
    virtual OsStatus getVideoFrameRate(int& frameRate) const;

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */
  protected:
    MpMediaTask*    mpMediaTask ; /**< Media task instance */
    uint32_t mFrameSizeMs; //< The size of the smallest unit of audio that we process on, in milliseconds
    uint32_t mMaxSamplesPerSec;   //< Maximum sample rate that any flowgraph may have set (used for initializing buffers)
    uint32_t mDefaultSamplesPerSec;   //< Default sample rate that flowgraphs and devices may have set
#ifdef INCLUDE_RTCP /* [ */
    IRTCPControl*   mpiRTCPControl;   /**< Realtime Control Interface */
#endif /* INCLUDE_RTCP ] */


/* //////////////////////////// PRIVATE /////////////////////////////////// */
  private:
    static int miInstanceCount;

    /// Diabled
    sipXmediaFactoryImpl(const sipXmediaFactoryImpl& refFactoryImpl);
    /// Diabled
    sipXmediaFactoryImpl& operator=(const sipXmediaFactoryImpl& refFactoryImpl);
};

/* ============================ INLINE METHODS ============================ */

#endif  // _sipXmediaFactoryImpl_h_
