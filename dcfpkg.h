/*
 
 Distribution package file in dcf format.

 Alignment
 ---------
 Padding of header and datasegments are adjusted so that all boundaries are
 4096 byte aligned.

 Datasegment
 -----------
 Each datasegment has a segmentid of: [pkgid, itemid, serialnr].
 This to allow restoration even from scrambled blocks.

 Collection ids used:
 -------------------
   0 = Installable package files.
   1 = Meta-data for archive.
   2 = Build specification files
   3 = Files used to perform installation and uninstall.
   4 = Manifest of contents

 Common metadata identifiers for all collections
 --------------------------------------
  pkgid (unique id for package)
  itemid (unique id for item within package)

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
 - For build specification files. Scripts, config, changelog, patches etc.
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
 name = preinstall|postinstall|preuninstall|postuninstall
 filemode = executable|read-only
 size
 
 * (pre|post)[un]install: DATA is an executable to be run at the appropriate time during package management.

 Metadata identifiers used in collection 4
 -----------------------------------------
 - Manifest of package collection 0 contents.
 - Several manifest records may be present in collection. The manifest with highest version is the current manifest.
 version = decimal string of version. Highest is latest.
 
 * DATA is list of files in collection 0.

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
 uid
 gid
 size
 devmajor
 devminor

 * DATA is file content or link target.

 */
