notes
- please look at testoutput.txt for some hints on what commands to run
- the test executables are written so that there will be a pause after every free, memset, malloc, or memcpy
- to dump to a binary, include "bin" in the filename; i.e. "d testing.bin"
- to dump to a txt, include any other extension; i.e. "d testing.txt"
- I changed the MAX size to 8192 because 8196 is not divisible by 8
- the log file will keep appending; it is not overwritten each time mmc is run (my log file may still include previous runs)
- VMMS.MEM includes mem_start, mapping table, and free list table
- any dumps will only include mem_start
- I did not free everything in my test exe just for the purpose of having things that are not freed to view in the binary file at the end

p.s. apologies if I missed any test cases 

recommended run:
1. run the "run.bat" to compile cpp
2. in 1st window, run "mmc 8"
3. enter m to view initial tables
4. in 2nd window, run "test1"
5. in 3rd window, run "test2"
