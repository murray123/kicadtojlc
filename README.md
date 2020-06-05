# kicadtojlc
Kicad bom and cpl file conversion for JCLpcb

To compile on a linux system using GCC:
gcc -o kicadtojlc kicadtojlc.c 

 Description : Application for the conversion of Kicad .pos files to JLCPCB compatible .cpl and .bom files.
  	  	  	   A footprint renaming, and rotation configuration file is supported.
  	  	  	   This allows standard KICAD footprints to translate to JLCpcb's placement system.
  	  	  	   This file is plain text and can be user modified to accommodate all types of components.
               
parrameters:

  InputFilename.pos : normal operation, Kicad.pos input file entered.
  -help             : Information             
  -convert          : Create a sample part_conversion.txt file
  
