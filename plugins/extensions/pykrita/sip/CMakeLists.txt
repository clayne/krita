include(SIPMacros)

message( ${SIP_VERSION} " - The version of SIP found expressed as a 6 digit hex number suitable for comparison as a string.")
message( ${SIP_VERSION_STR} " - The version of SIP found as a human readable string.")
message( ${SIP_EXECUTABLE} " - Path and filename of the SIP command line executable.")
if(${SIP_VERSION_STR} VERSION_LESS 5)
    message( ${SIP_DEFAULT_SIP_DIR} " - default SIP dir" )
    message( ${SIP_INCLUDE_DIR} " - Directory holding the SIP C++ header file.")

    set(SIP_INCLUDES
        ${SIP_DEFAULT_SIP_DIR}
        ${PYQT_SIP_DIR}
        ${PYQT_SIP_DIR_OVERRIDE}
        ./krita)

    set(SIP_CONCAT_PARTS 1)
    set(SIP_TAGS ALL WS_X11 ${PYQT_VERSION_TAG})
    set(SIP_EXTRA_OPTIONS -g -o -x PyKDE_QVector)
    IF (PYQT5_SIP_NAME)
        set(SIP_EXTRA_OPTIONS ${SIP_EXTRA_OPTIONS} -n ${PYQT5_SIP_NAME})
    ENDIF (PYQT5_SIP_NAME)

    set(PYTHON_SITE_PACKAGES_INSTALL_DIR ${KDE_INSTALL_LIBDIR}/krita-python-libs)
    file(GLOB PYKRITA_KRITA_sip_files ./krita/*.sip)
    set(SIP_EXTRA_FILES_DEPEND ${PYKRITA_KRITA_sip_files})
    add_sip_python_module(PyKrita.krita ./krita/kritamod.sip kritalibkis kritaui kritaimage kritalibbrush)
else()
    set(SIP_CONCAT_PARTS 1)
    set(SIP_MODULE ${PYQT_SIP_NAME})
    set(SIP_TAGS ${PYQT_SIP_TAGS})
    set(SIP_INCLUDES ${PYQT_SIP_DIR})
    set(PYTHON_SITE_PACKAGES_INSTALL_DIR ${KDE_INSTALL_LIBDIR}/krita-python-libs)
    set(SIP_EXTRA_FILES_DEPEND "${CMAKE_CURRENT_SOURCE_DIR}/krita")
    set(SIP_EXTRA_STUBS "${CMAKE_CURRENT_SOURCE_DIR}/extra.pyi")
    add_sip_python_module_v5(PyKrita.krita ./krita/kritamod.sip kritalibkis kritaui kritaimage kritalibbrush)
endif()
