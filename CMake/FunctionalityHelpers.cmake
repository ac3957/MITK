MACRO( FILL_WITH_BASENAMES DESTLIST SOURCELIST) 
  FOREACH(file ${${SOURCELIST}})
    GET_FILENAME_COMPONENT(BASENAME ${file} NAME)
    SET(${DESTLIST} ${${DESTLIST}} ${BASENAME})
  ENDFOREACH(file)
ENDMACRO( FILL_WITH_BASENAMES ) 

# legacy macro, called by "old" functionalities
MACRO( createFunctionalityLib )
  IF(NOT FUNC_NAME)
    GET_FILENAME_COMPONENT(FUNC_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    MESSAGE(STATUS "Warning. No FUNC_NAME. Setting it to directory name: ${FUNC_NAME}")
  ENDIF(NOT FUNC_NAME)
  MESSAGE(STATUS "Including ${FUNC_NAME} with legacy mechanisn. Fix it.")
  IF(NOT APPMOD_CPP)
    INCLUDE(${FUNC_NAME}.cmake)
  ENDIF(NOT APPMOD_CPP)
  # remove all prepending paths from the files
  FILL_WITH_BASENAMES(FUNC_CPP APPMOD_CPP)
  FILL_WITH_BASENAMES(FUNC_MOC_H APPMOD_MOC_H)
  FILL_WITH_BASENAMES(H_LIST APPMOD_H)
  FILL_WITH_BASENAMES(FUNC_FORMS APPMOD_FORMS)
  # MESSAGE(STATUS "Legacy result: ${FUNC_CPP}")
  CREATE_QFUNCTIONALITY(${FUNC_NAME})
ENDMACRO( createFunctionalityLib )

# 
# Variables
#
#   FUNC_NAME: Name of the functionality. If not set it will be set to the directory name
# 
# The following ones will be generated if not set
#   
#   ADD_FUNC_CODE: Name of a file which contains the AddFuncName() definition
#   ADD_FUNC_HEADER: Name of a file which contains the AddFuncName() declaration
#   ADD_FUNC_CALL: File which contains just the call to the above function 

MACRO(CREATE_QFUNCTIONALITY FUNC_NAME)
  # MESSAGE("Before MACRO: ${${KITNAME}FUNCTIONALITY_NAMES}")
  IF(NOT FUNC_NAME)
    GET_FILENAME_COMPONENT(FUNC_NAME ${PROJECT_SOURCE_DIR} NAME)
    MESSAGE(STATUS "Warning. No FUNC_NAME set. Setting it to directory name: ${FUNC_NAME}")
  ENDIF(NOT FUNC_NAME)
  IF(NOT FUNC_CPP)
    MESSAGE(STATUS "Using files.cmake for functionality ${FUNC_NAME}")
    INCLUDE(files.cmake)
    SET(FUNC_CPP ${CPP_FILES})
    SET(FUNC_MOC_H ${MOC_H_FILES})
    SET(FUNC_FORMS ${UI_FILES})
  ELSE(NOT FUNC_CPP)
    MESSAGE(STATUS "Using FUNC_CPP for functionality ${FUNC_NAME}")
  ENDIF(NOT FUNC_CPP)


  SET(FUNC_CPP_LIST ${FUNC_CPP})
  IF(FUNC_FORMS)
    QT_WRAP_UI(${FUNC_NAME} FUNC_H_LIST FUNC_CPP_LIST ${FUNC_FORMS})
  ENDIF(FUNC_FORMS)
  IF(FUNC_MOC_H)
    QT_WRAP_CPP(${FUNC_NAME} FUNC_CPP_LIST ${FUNC_MOC_H})
  ENDIF(FUNC_MOC_H)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  SET(FUNCTIONALITY_NAME ${FUNC_NAME})
  CONFIGURE_FILE("${REGISTER_QFUNCTIONALITY_CPP_IN}" "${CMAKE_CURRENT_BINARY_DIR}/Register${FUNC_NAME}.cpp" @ONLY)
  IF(${KITNAME}FUNCTIONALITY_NAMES)
     LIST(REMOVE_ITEM ${KITNAME}FUNCTIONALITY_NAMES ${FUNC_NAME}) 
  ENDIF(${KITNAME}FUNCTIONALITY_NAMES)
  IF(${KITNAME}FUNCTIONALITY_NAMES_KNOWN)
    LIST(REMOVE_ITEM ${KITNAME}FUNCTIONALITY_NAMES_KNOWN ${FUNC_NAME}) 
  ENDIF(${KITNAME}FUNCTIONALITY_NAMES_KNOWN)
  OPTION("BUILD_${FUNC_NAME}" "Build ${FUNC_NAME} functionality")
  IF(BUILD_${FUNC_NAME} OR BUILD_ALL_FUNCTIONALITIES)
    SET(${KITNAME}FUNCTIONALITY_NAMES ${${KITNAME}FUNCTIONALITY_NAMES};${FUNC_NAME} CACHE INTERNAL "Names of all enabled functionalities.")
    # SET(${KITNAME}FUNCTIONALITY_NAMES-MODIFIED 1 CACHE INTERNAL "test")
  #   CLEANUP_FUNCTIONALITY_LIST()
    ADD_LIBRARY(${FUNC_NAME} ${FUNC_CPP_LIST} "${CMAKE_CURRENT_BINARY_DIR}/Register${FUNC_NAME}.cpp" )
    TARGET_LINK_LIBRARIES(${FUNC_NAME} ${LIBS_FOR_QFUNCTIONALITY})
    ADD_DEPENDENCIES(${FUNC_NAME} ${LIBS_FOR_QFUNCTIONALITY})
  ENDIF(BUILD_${FUNC_NAME} OR BUILD_ALL_FUNCTIONALITIES)
  SET(${KITNAME}FUNCTIONALITY_NAMES_KNOWN ${${KITNAME}FUNCTIONALITY_NAMES_KNOWN};${FUNC_NAME} CACHE INTERNAL "Names of all known functionalities.")
# MESSAGE("after MACRO: ${${KITNAME}FUNCTIONALITY_NAMES}")
  
#  FILE(READ ${CMAKE_CACHEFILE_DIR}/CMakeCache.txt CACHE_CONTENT)
#  FILE(WRITE ${CMAKE_CACHEFILE_DIR}/CMakeCache${FUNC_NAME} ${CACHE_CONTENT})

ENDMACRO(CREATE_QFUNCTIONALITY)

MACRO(CLEANUP_FUNCTIONALITY_LIST)
  SET(TMP_LIST "")
  FOREACH(f ${FUNCTIONALITY_NAMES})
  IF("${TMP_LIST}" MATCHES "${f}")
  ELSE("${TMP_LIST}" MATCHES "${f}")
    SET(TMP_LIST ${TMP_LIST} "${f}")
  ENDIF("${TMP_LIST}" MATCHES "${f}")
  ENDFOREACH(f)
  SET(FUNCTIONALITY_NAMES ${TMP_LIST} CACHE INTERNAL "Names of all enabled functionalities.")
ENDMACRO(CLEANUP_FUNCTIONALITY_LIST)

MACRO(LINK_LIB_TO_FUNCTIONALITY FUNCTIONALITY LIBRARY)
  IF(BUILD_ALL_FUNCTIONALITIES OR BUILD_${FUNCTIONALITY})
    TARGET_LINK_LIBRARIES(${FUNCTIONALITY} ${LIBRARY})
  ENDIF(BUILD_ALL_FUNCTIONALITIES OR BUILD_${FUNCTIONALITY})
ENDMACRO(LINK_LIB_TO_FUNCTIONALITY FUNCTIONALITY LIBRARY)

