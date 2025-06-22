file(GLOB src_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(DDAS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/DDASRootHit.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/DDASRootEvent.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/DDASHit.h
)

set(DDASLegacy_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/ddaschannel_legacy.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/DDASEvent_legacy.h
)

ROOT_GENERATE_DICTIONARY(DDAS_root ${DDAS_HEADERS} LINKDEF include/DDASLinkDef.h )
add_library(DDASRoot SHARED DDAS_root.cxx DDAS_root)
target_link_libraries(DDASRoot ${ROOT_LIBRARIES})
list(APPEND PROCESSOR_STRUCT_LIBS DDASRoot)
list(APPEND PROCESSOR_STRUCT_PCM ${CMAKE_CURRENT_BINARY_DIR}/libDDAS_root_rdict.pcm)
list(APPEND PROCESSOR_STRUCT_RMAP ${CMAKE_CURRENT_BINARY_DIR}/libDDAS_root.rootmap)

ROOT_GENERATE_DICTIONARY(DDASRootLegacy_root ${DDASLegacy_HEADERS} LINKDEF include/DDASLegacyLinkDef.h )
add_library(DDASRootLegacy SHARED DDASRootLegacy_root.cxx DDASRootLegacy_root)
target_link_libraries(DDASRootLegacy ${ROOT_LIBRARIES})
list(APPEND PROCESSOR_STRUCT_LIBS DDASRootLegacy)
list(APPEND PROCESSOR_STRUCT_PCM ${CMAKE_CURRENT_BINARY_DIR}/libDDASRootLegacy_root_rdict.pcm)
list(APPEND PROCESSOR_STRUCT_RMAP ${CMAKE_CURRENT_BINARY_DIR}/libDDASRootLegacy_root.rootmap)

add_library(ldf2rootCore ${src_files})
target_link_libraries(ldf2rootCore PUBLIC ${ROOT_LIBRARIES})
target_include_directories(ldf2rootCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS ldf2rootCore DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

install(TARGETS ${PROCESSOR_STRUCT_LIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${PROCESSOR_STRUCT_PCM} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${PROCESSOR_STRUCT_RMAP} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
foreach( CURRLIB ${PROCESSOR_STRUCT_LIBS} )
	if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		install(CODE 
			"execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink lib${CURRLIB}.dylib ${CMAKE_INSTALL_PREFIX}/lib/lib${CURRLIB}_root.dylib)"
			)
	else()
		install(CODE 
			"execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink lib${CURRLIB}.so ${CMAKE_INSTALL_PREFIX}/lib/lib${CURRLIB}_root.so)"
			)
	endif()
endforeach()
