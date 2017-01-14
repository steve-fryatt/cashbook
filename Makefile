# Copyright 2010-2017, Stephen Fryatt (info@stevefryatt.org.uk)
#
# This file is part of CashBook:
#
#   http://www.stevefryatt.org.uk/software/
#
# Licensed under the EUPL, Version 1.1 only (the "Licence");
# You may not use this work except in compliance with the
# Licence.
#
# You may obtain a copy of the Licence at:
#
#   http://joinup.ec.europa.eu/software/page/eupl
#
# Unless required by applicable law or agreed to in
# writing, software distributed under the Licence is
# distributed on an "AS IS" basis, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the Licence for the specific language governing
# permissions and limitations under the Licence.

# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

ARCHIVE := cashbook

APP := !CashBook
SHHELP := CashBook,3d6
HTMLHELP := manual.html

OBJS = account.o accview.o amenu.o analysis.o budget.o caret.o choices.o clipboard.o	\
       column.o currency.o date.o edit.o file.o filing.o find.o fontlist.o goto.o	\
       iconbar.o interest.o main.o mainmenu.o presets.o printing.o purge.o report.o	\
       sorder.o	sort.o sort_dialogue.o transact.o window.o

include $(SFTOOLS_MAKE)/CApp

