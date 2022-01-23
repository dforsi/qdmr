# Reverse engineering of the D578UV III codeplug

As the codeplug for the 578UV III will likely be almost identical to the 578UV, this document only
lists the differences. The D578UV codeplug documentation can be found under

https://dm3mat.darc.de/qdmr/libdmrconf/classD578UVCodeplug.html

The table there lists only the memory layout. That is, which memory regions are accessed and what 
is stored there. The encoding of each codeplug element (e.g., channel etc) is then documented it 
the corresponding class. For example, the Channel element for the D578UV is documented in

https://dm3mat.darc.de/qdmr/libdmrconf/classD578UVCodeplug_1_1ChannelElement.html


## Collaboration
The easiest way for me would be if you fork the project at github and work on the d578uv2 branch. 
Then just send pull-requests. 

## Setup
Setup a windows machine (I use VirtualBox for it and use a shared directory to inspect the dumped 
codeplugs under the host Linux) running the CPS, COM0COM driver (virtual null-modem, 
http://com0com.sourceforge.net/) as well as Python with the pyserial package. Then start the 
emulator  `at_d578uv2_emulator.py` connecting to one of the null-modem ports and select the other 
in the CPS. 

## Analysis
Create a *base codeplug* (simple one only a few channels). 

Write base-codeplug to *device*. This will create a new file *codeplug_0000.hex* containing the 
binary base codeplug as a hex dump. The first column shows the memory address the rest the content.

### Memory Layout Analysis
In a first step, check if all address ranges match the one in the documentation of the D578UV. 
Maybe there are *new* blocks that are not written to the old D578UV. Once that is complete, 
continue with the differential analysis of each block.

### Differential Analysis.
Just change a single setting within the *base codeplug* and write it to the *device*. Then use 
`diff` or any other comparison tool to compare the two hex dumps. That is, the dump of the *base  
codeplug* and the dump containing the change. Then re-open the base codeplug and continue with the
next setting.

## Results
TODO