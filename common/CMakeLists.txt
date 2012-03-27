################################################################################
#    Copyright (C) 2011 HPCC Systems.
#
#    This program is free software: you can redistribute it and/or All rights
#    reserved. This program is NOT PRESENTLY free software: you can NOT
#    redistribute
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

add_subdirectory (deftype)
add_subdirectory (dllserver)
add_subdirectory (environment)
add_subdirectory (remote)
add_subdirectory (thorhelper)
add_subdirectory (workunit)
add_subdirectory (fileview2)
add_subdirectory (roxiecommlib)
add_subdirectory (commonext)


if ( WITH_ESP )
    add_subdirectory (monitoring)
endif()

add_subdirectory (roxiehelper)
if ( WITH_ROXIE )
    add_subdirectory (roxiemanager)
endif()
add_subdirectory (wuwebview)
