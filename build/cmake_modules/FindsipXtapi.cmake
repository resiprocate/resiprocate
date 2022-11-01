
FIND_PATH(SIPXTAPI_INCLUDE_DIR CpTopologyGraphInterface.h PATH_SUFFIXES sipxtapi)

SET(SIPXTAPI_INCLUDE_DIRS ${SIPXTAPI_INCLUDE_DIR})


find_library(sipXport_LIBRARIES sipXport )
find_library(sipXsdp_LIBRARIES sipXsdp )
find_library(sipXmedia_LIBRARIES sipXmedia )
find_library(sipXmediaProcessing_LIBRARIES sipXmediaProcessing )

set(SIPXTAPI_LIBRARIES ${sipXport_LIBRARIES} ${sipXsdp_LIBRARIES} ${sipXmedia_LIBRARIES} ${sipXmediaProcessing_LIBRARIES})


FIND_PACKAGE_HANDLE_STANDARD_ARGS(sipXtapi DEFAULT_MSG
        SIPXTAPI_INCLUDE_DIR sipXport_LIBRARIES sipXsdp_LIBRARIES sipXmedia_LIBRARIES sipXmediaProcessing_LIBRARIES)

MARK_AS_ADVANCED(SIPXTAPI_INCLUDE_DIR SIPXTAPI_LIBRARY)
