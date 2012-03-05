
MACRO(RUN_XSLTPROC _xsl _file _out _in_dir _out_dir )
	STRING(REGEX REPLACE "([0-9a-z_-]*).xml" "\\1" _file_base "${_file}")
	SET(_xsl ${_xsl})
	SET(_file ${_file})
	SET(_out ${_out})
	SET(_in_dir ${_in_dir})
	SET(_out_dir ${_out_dir})
	IF( ${ARGC} EQUAL 6 )
		SET(_target ${ARGN})
	ENDIF()
	
	CONFIGURE_FILE(${HPCC_SOURCE_DIR}/docs/BuildTools/xsltproc.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${_out}.cmake @ONLY)
	
	ADD_CUSTOM_COMMAND(
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/${_out}.cmake
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_out}.sentinel
		DEPENDS docbook-expand ${_target}
		)
	ADD_CUSTOM_TARGET(${_out} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_out}.sentinel)
ENDMACRO(RUN_XSLTPROC)

MACRO(RUN_FOP _file _out)
	ADD_CUSTOM_COMMAND(
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/docs
		COMMAND "${FOP_EXECUTABLE}" ${CMAKE_CURRENT_BINARY_DIR}/${_file} -pdf ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/docs/${_out} 
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_out}.sentinel
		DEPENDS ${_file}
		)
	ADD_CUSTOM_TARGET(${_out} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_out}.sentinel ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/docs/${_out} )
ENDMACRO(RUN_FOP)


#Use this Macro to clean an XML docbook file of relative paths for out of source builds.
MACRO(CLEAN_REL _file _doc_dir _in_dir _out_dir)
	STRING(REGEX REPLACE "([0-9a-z_-]*).xml" "\\1" _file_base "${_file}")
	SET(_clean_target "clean_${_file}")
	SET(when "")
	FOREACH( _ref ${ARGN})
		SET(when "${when} <xsl:when test=\"name()='${_ref}'\">${_doc_dir}<xsl:value-of select=\".\"/></xsl:when>")
	ENDFOREACH(_ref)
	CONFIGURE_FILE(${HPCC_SOURCE_DIR}/docs/BuildTools/relrem.xsl.in ${CMAKE_CURRENT_BINARY_DIR}/${_file_base}.xsl @ONLY)
	RUN_XSLTPROC( ${CMAKE_CURRENT_BINARY_DIR}/${_file_base}.xsl ${_file} ${_file} ${_in_dir} ${_out_dir})
	ADD_CUSTOM_TARGET( ${_clean_target} DEPENDS ${_file})
ENDMACRO(CLEAN_REL)

MACRO(DOCBOOK_TO_PDF _xsl _file _name)
	IF(MAKE_DOCS)
		STRING(REGEX REPLACE "([0-9a-z_-]*).xml" "\\1" _file_base "${_file}")
		SET(_fo_file ${_file_base}.fo)
		SET(_pdf_file ${_name}.pdf)
		SET( _docs_target "doc_${_file}_${_pdf_file}")  # File to Name of type.
		set( REFS fileref href )
		CLEAN_REL(${_file} ${DOC_IMAGES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${REFS})
		RUN_XSLTPROC(${_xsl} ${_file} ${_fo_file} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} "clean_${_file}")
		RUN_FOP(${_fo_file} ${_pdf_file})
		MESSAGE("-- Adding document: ${_pdf_file} -  target: ${_docs_target}")
		ADD_CUSTOM_TARGET(${_docs_target} ALL DEPENDS ${_pdf_file} ${CMAKE_CURRENT_BINARY_DIR}/${_file} ) 
		set_property(GLOBAL APPEND PROPERTY DOC_TARGETS "${_docs_target}")
	ENDIF(MAKE_DOCS)
ENDMACRO(DOCBOOK_TO_PDF targetname_suffix srcfile outfile targetdir deps_list)