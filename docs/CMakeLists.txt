################################################################################
#    Copyright (C) 2011 HPCC Systems.
#
#    All rights reserved. This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as
#    published by the Free Software Foundation, either version 3 of the
#    License, or (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

cmake_minimum_required(VERSION 2.8)
PROJECT(docs)

include(${CMAKE_SOURCE_DIR}/docMacros.cmake)

set (DOCBOOK_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/resources)
set (DOCBOOK_XSL ${CMAKE_CURRENT_BINARY_DIR}/resources/docbook-xsl)
set (DOC_IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/)
set (XML_CATALOG ${CMAKE_CURRENT_BINARY_DIR}/BuildTools/catalog.xml)
SET( ENV{SGML_CATALOG_FILES} "${XML_CATALOG}")
SET( ENV{XML_DEBUG_CATALOG} "1" )
SET( ENV{XML_CATALOG_FILES} "${XML_CATALOG}")

add_subdirectory(resources)
add_subdirectory(BuildTools)

set (VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/Version.xml)

configure_file(Version.xml.in ${CMAKE_CURRENT_BINARY_DIR}/Version.xml)

set (FO_XSL ${CMAKE_CURRENT_BINARY_DIR}/BuildTools/fo.xsl)

DOCBOOK_TO_PDF( ${FO_XSL} IMDB.xml "IMDB" _targets )
DOCBOOK_TO_PDF( ${FO_XSL} DataHandling.xml "DataHandling" _targets )

add_subdirectory(ECLR_mods)
add_subdirectory(SLR-Mods)

add_custom_target(docs ALL DEPENDS ${_targets})
