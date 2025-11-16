# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/) and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [3.0.0] - 2025-10
### Added
- Added the following files (for better management of contributions):
  - CREDITS
  - CODE_OF_CONDUCT.md
  - CONTRIBUTING.md
  - CLA.md
- Added *.gitkeep* file into sevaral directories (e.g. *./obj*) to keep them in the directory structure on GitHub
### Changed
- The unique file *src/mixfApi.c* in previous releases has been split into 4 files for increased readability
- All exposed funtions (i.e. those declared in *headers/mixf.h*) have been renamed due to a change in the name convention adopted (from *UpperCamelCase* to *snake_case*). For example, *InitParamList()* has been renamed in *init_param_list()*
- *Boolean* type and *TRUE*/*FALSE* macros have been substituted respectively with *bool* type and *true* and *false* values defined in *stdbool.h*
- in *include/mixfApi.h* type definitions have been renamed to denote new types with *UpperCamelCase* names
- in *include/mixfApi.h* the following macros have been renamed as follows:
	- *LONGSTRINGMAXLEN* -> *EXTENDEDSTRINGMAXLEN*
	- *MEDIUM2STRINGMAXLEN* -> *LONGSTRINGMAXLEN*
- in *include/mixfApi.h* type definitions for strings of various lengths have been slightly changed, to align their lengths to integer powers of 2
- Several routines in which string parameters whose lengths is checked and possibly truncated have been updated, to reflect new limits (see previous point)
- Changed README (content updated and format changed from plain text to markdown)
- Updated all examples to reflect the changes above
### Deprecated
### Removed
- The *Boolean* type previously defined and the corresponding macros TRUE* and *FALSE* have been removed (and substituted with *bool* type in *stdbool.h*)
- Removed *RevHistory.txt* (its content has been moved in this file, i.e. *CHANGELOG.md*)
- Deleted the *libmixf.pdf* guide in *./docs*. Its content has been updated and moved into the *README.md* and related files
### Fixed
- Fixed bug in *check_license()*, that didn't work correctly when hostId was shorter than 8 chars
- Fixed bug in *check_and_dump_ctr()*, that causes a segmentation fault when base and aggregate dump times starts again from the beginning of the dump times string
### Security


## [2.1.0] - 2021-06
### Added
- Introduction of some new parameter types in the family of Configuration Files Handling functions (ipv4, mail, url) along with the corresponding functions needed to manage them, i.e.:
    - *AddMailParam()*
    - *AddIPv4Param()*
    - *AddUrlParam()*
    - *GetMailParamValue()*
    - *GetIPv4ParamValue()*
    - *GetUrlParamValue()*
- Introduction of the following functions in the String Handling category:
    - *CheckMailValidity()*
    - *CheckIPv4AddValidity()*
    - *CheckUrlValidity()*
    - *GenerateToken()*
- Optimization of Configuration File Handling functions (added dynamic parameters allocation vs static allocation employed before).
  - Introduction of *InitParamList()*
### Changed
- Changed name from ***libmisc*** to ***libmixf*** (a ***libmisc*** library was already available on sourceforge)
- Licensed under the Apache License, Version 2.0
- Changed names to Error Definition Macros in *headers\mixf.h* (to avoid conflicts with C standard library)
### Deprecated
### Removed
### Fixed
- Corrected some bugs in the following functions (License Handling):
    - *CreateLicense()*
    - *CheckLicense()*
### Security


## [2.0.0] - 2019-11
### Added
- Added a new family of functions for Counters Handling
### Changed
### Deprecated
### Removed
### Fixed
### Security


## [1.0.0] - 2019-10
### Added
- First version
- Originally named ***libmisc***
- License-free Software (i.e. Proprietary)
- Not released on GitHub
- Static/Dinamic C/C++ Library which contains several general purpose functions organized in the following families:
   - File and File System Handling
   - Time and Date Handling
   - String Handling
   - Configuration Files Handling
   - Log Handling
   - License Handling
   - Lock Handling
### Changed
### Deprecated
### Removed
### Fixed
### Security

[Unreleased]: https://github.com/Roberto-Mameli/libmixf/v.3.0.0...HEAD
[3.0.0]: https://github.com/Roberto-Mameli/libmixf/releases/tag/v.3.0.0
[2.1.0]: https://github.com/Roberto-Mameli/libmixf/releases/tag/v.2.1.0
