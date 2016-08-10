/*
 
 Distribution package file in dcf format.

 Collecion ids used:
 -------------------
   0 = Installable package files.
   1 = Meta-data for archive.
   2 = Build specification files
   3 = Files used to perform installation and uninstall.

 Metadata identifiers used in collection 1
 -----------------------------------------
 - Metadata for package.
  name = package name
  version
  release
  type = source|binary
  pkgdependencies
  fndependencies
  fnprovides

  disksize
  sha256
  nfiles
  summary
  description
  license
  os
  cpu-arch
  buildtime
  buildhost

 Metadata identifiers used in collection 2
 -----------------------------------------
 - For build specification files.
 - Owner unspecified. Only "normal" files.
 - Files should normally have a path in current directory (plain files without directory-structure).
 filepath
 filetype = regular|dir
 filemode = executable|read-only
 size

 Metadata identifiers used in collection 3
 -----------------------------------------
 - Files used for performing installation, uninstallation and bookkeeping.
 - 'name' MUST NOT contain '/'.
 name = manifest|preinstall|postinstall|preuninstall|postuninstall
 filetype = regular|dir
 filemode = executable|read-only
 size
 
 * manifest: DATA is list of files in collection 0.
 * (pre|post)[un]install: DATA is an executable to be run at the appropriate time during package management.

 Metadata identifiers used in collection 0
 -----------------------------------------
 - Installable package files.
 filepath
 filetype
 filemode
 fileflags = config|doc etc
 mtime
 ctime
 ownername
 groupname
 size
 devmajor
 devminor

 * DATA is file content or link target.

 */
