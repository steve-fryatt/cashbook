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

# CCFLAGS := -DDEBUG

OBJS = account.o			\
       account_list_menu.o		\
       account_menu.o			\
       accview.o			\
       amenu.o				\
       analysis.o			\
       analysis_balance.o		\
       analysis_cashflow.o		\
       analysis_data.o			\
       analysis_dialogue.o		\
       analysis_lookup.o		\
       analysis_period.o		\
       analysis_template.o		\
       analysis_template_menu.o		\
       analysis_template_save.o		\
       analysis_transaction.o		\
       analysis_unreconciled.o		\
       budget.o				\
       caret.o				\
       choices.o			\
       clipboard.o			\
       column.o				\
       currency.o			\
       date.o				\
       edit.o				\
       file.o				\
       filing.o				\
       find.o				\
       flexutils.o			\
       fontlist.o			\
       goto.o				\
       iconbar.o			\
       interest.o			\
       main.o				\
       presets.o			\
       preset_menu.o			\
       printing.o			\
       printing_protocol.o		\
       purge.o				\
       refdesc_menu.o			\
       report.o				\
       sorder.o				\
       sort.o				\
       sort_dialogue.o			\
       stringbuild.o			\
       transact.o			\
       window.o

include $(SFTOOLS_MAKE)/CApp
