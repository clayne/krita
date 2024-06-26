# Find Python
# ~~~~~~~~~~~
# Find the Python interpreter and related Python directories.
#
# This file defines the following variables:
#
# PYTHON_EXECUTABLE - The path and filename of the Python interpreter.
#
# PYTHON_SHORT_VERSION - The version of the Python interpreter found,
#     excluding the patch version number. (e.g. 2.5 and not 2.5.1))
#
# PYTHON_LONG_VERSION - The version of the Python interpreter found as a human
#     readable string.
#
# PYTHON_SITE_PACKAGES_DIR - Location of the Python site-packages directory.
#
# PYTHON_INCLUDE_PATH, PYTHON_INCLUDE_DIRS - Directory holding the python.h include file.
#
# PYTHON_LIBRARY, PYTHON_LIBRARIES- Location of the Python library.

# SPDX-FileCopyrightText: 2007 Simon Edwards <simon@simonzone.com>
# SPDX-FileCopyrightText: 2012 Luca Beltrame <lbeltrame@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause
#

if(APPLE)
    set(CMAKE_FRAMEWORK_PATH_OLD ${CMAKE_FRAMEWORK_PATH})
    set(CMAKE_FRAMEWORK_PATH "${CMAKE_INSTALL_PREFIX}/lib" ${CMAKE_SYSTEM_FRAMEWORK_PATH})
endif(APPLE)

include(FindPackageHandleStandardArgs)

if (WIN32 OR APPLE)
    set(Python_FIND_STRATEGY LOCATION)
    # on APPLE we search python using Python_ROOT_DIR set by the toolchain file
    find_package(Python 3.8 REQUIRED COMPONENTS Development Interpreter)
else()
    find_package(Python 3.8 REQUIRED COMPONENTS Interpreter OPTIONAL_COMPONENTS Development)
endif()

message(STATUS "FindPythonLibrary: ${Python_Interpreter_FOUND}")

if (Python_Interpreter_FOUND)
    set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})

    # Set the Python libraries to what we actually found for interpreters
    set(Python_ADDITIONAL_VERSIONS "${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
    # These are kept for compatibility
    set(PYTHON_SHORT_VERSION "${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
    set(PYTHON_LONG_VERSION ${PYTHON_VERSION_STRING})

    if(Python_Development_FOUND)
        set(PYTHON_INCLUDE_DIRS ${Python_INCLUDE_DIRS})
        set(PYTHON_INCLUDE_PATH ${Python_INCLUDE_DIRS})
        set(PYTHON_LIBRARY ${Python_LIBRARIES})
    endif(Python_Development_FOUND)

    # Auto detect Python site-packages directory
    execute_process(COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('platlib'))"
                    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES_DIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                   )

    message(STATUS "Python system site-packages directory: ${PYTHON_SITE_PACKAGES_DIR}")

    if (NOT CMAKE_PREFIX_PATH)
        message (WARNING "CMAKE_PREFIX_PATH variable is not set, we might NOT be able to detect SIP modules")
        # use install prefix as a fallback
        set(_all_prefix_paths ${CMAKE_INSTALL_PREFIX})
    else()
        set(_all_prefix_paths ${CMAKE_PREFIX_PATH})
    endif()

    unset(KRITA_PYTHONPATH_V4 CACHE)
    unset(KRITA_PYTHONPATH_V5 CACHE)

    cmake_path(CONVERT "${_all_prefix_paths}" TO_CMAKE_PATH_LIST _python_prefix_path_v4 NORMALIZE)
    list(REMOVE_ITEM _python_prefix_path_v4 "")
    list(TRANSFORM _python_prefix_path_v4 APPEND "/lib/krita-python-libs")
    cmake_path(CONVERT "${_python_prefix_path_v4}" TO_NATIVE_PATH_LIST KRITA_PYTHONPATH_V4 NORMALIZE)

    cmake_path(CONVERT "${_all_prefix_paths}" TO_CMAKE_PATH_LIST _python_prefix_path_v5 NORMALIZE)
    list(REMOVE_ITEM _python_prefix_path_v5 "")
    list(TRANSFORM _python_prefix_path_v5 APPEND "/lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages")
    cmake_path(CONVERT "${_python_prefix_path_v5}" TO_NATIVE_PATH_LIST KRITA_PYTHONPATH_V5 NORMALIZE)

    message(STATUS "Krita site-packages directories for SIP v4: ${KRITA_PYTHONPATH_V4}")
    message(STATUS "Krita site-packages directories for SIP v5+: ${KRITA_PYTHONPATH_V5}")
endif()

find_package_handle_standard_args(PythonLibrary DEFAULT_MSG PYTHON_LIBRARY PYTHON_INCLUDE_DIRS PYTHON_INCLUDE_PATH)

if(APPLE)
    set(CMAKE_FRAMEWORK_PATH ${CMAKE_FRAMEWORK_PATH_OLD})
endif(APPLE)
