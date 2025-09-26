rem This needs to be run from a visual studio command prompt
cl /c rawpdb.cpp /I ./raw_pdb/src/
lib /out:rawpdb.lib rawpdb.obj