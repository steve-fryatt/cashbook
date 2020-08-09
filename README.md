CashBook
========

A Home Accounts Manager for RISC OS.

Introduction
------------

CashBook is a home accounts manager, which can be used to keep track of home or club accounts. The system is based around transactions, which correspond to those on bank, building society and credit card statements. These transactions transfer money between accounts, or to and from analysis headings.

Transactions into and out of accounts can be reconciled against statements, making it quick and easy to spot discrepancies and errors. Budgets can be set for transactions into and out of analysis headings, so that income and expenditure over time can be monitored and recorded. Various reporting options allow detailed information to be produced on many aspects of the accounts and transactions.

Entering transactions can be done in two ways. In addition to manual entry, regular payments such as standing orders, direct debits or salary payments can be automated. Pending transactions up to a specified date can be included in any future balance figures for accounts.


Building
--------

CashBook consists of a collection of C and un-tokenised BASIC, which must be assembled using the [SFTools build environment](https://github.com/steve-fryatt). It will be necessary to have suitable Linux system with a working installation of the [GCCSDK](http://www.riscos.info/index.php/GCCSDK) to be able to make use of this.

With a suitable build environment set up, making CashBook is a matter of running

	make

from the root folder of the project. This will build everything from source, and assemble a working !CashBook application and its associated files within the build folder. If you have access to this folder from RISC OS (either via HostFS, LanManFS, NFS, Sunfish or similar), it will be possible to run it directly once built.

To clean out all of the build files, use

	make clean

To make a release version and package it into Zip files for distribution, use

	make release

This will clean the project and re-build it all, then create a distribution archive (no source), source archive and RiscPkg package in the folder within which the project folder is CashBookd. By default the output of `git describe` is used to version the build, but a specific version can be applied by setting the `VERSION` variable -- for example

	make release VERSION=1.23


Licence
-------

CashBook is licensed under the EUPL, Version 1.2 only (the "Licence"); you may not use this work except in compliance with the Licence.

You may obtain a copy of the Licence at <http://joinup.ec.europa.eu/software/page/eupl>.

Unless required by applicable law or agreed to in writing, software distributed under the Licence is distributed on an "**as is**"; basis, **without warranties or conditions of any kind**, either express or implied.

See the Licence for the specific language governing permissions and limitations under the Licence.