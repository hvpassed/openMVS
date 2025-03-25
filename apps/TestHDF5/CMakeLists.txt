if(MSVC)
	create_rc_files(TestHDF5)
	FILE(GLOB LIBRARY_FILES_C "*.cpp" "${CMAKE_CURRENT_BINARY_DIR}/*.rc")
else()
	FILE(GLOB LIBRARY_FILES_C "*.cpp")
endif()
FILE(GLOB LIBRARY_FILES_H "*.h" "*.inl")
cxx_executable_with_flags(TestHDF5 "Apps" "${cxx_default}" "MVS;${OpenMVS_EXTRA_LIBS};" ${LIBRARY_FILES_C} ${LIBRARY_FILES_H})
# Install
INSTALL(TARGETS TestHDF5
	EXPORT OpenMVSTargets
	RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)
