/*
 
 Distribution package file in dcf format.

 Collecion ids used:
 -------------------
   0 = Archive files.
   1 = Meta-data for archive.
   2 = Build specification files

 Metadata identifiers used in collection 1
 -----------------------------------------
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

  manifest [DATA is list of files in collection 0]

  postinstall [DATA is postinstall executable]

 Metadata identifiers used in collection 2
 -----------------------------------------
 - For build specification files.
 - Owner unspecified. Only "normal" files.
 - files should normally have a path in current directory (plain files without directory-structure).
 filepath
 filetype = regular|dir
 filemode = executable|read-only
 size

 Metadata identifiers used in collection 0
 -----------------------------------------
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

 */
