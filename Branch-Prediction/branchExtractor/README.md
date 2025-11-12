# Branch Extractor

This software is modified from https://github.com/mbaharan/branchExtractor

This software logs the trace of branche executed during a specific benchmark. This software uses Intel PIN tools for instrumenting the retired instructions.

## Installation
```sh
$ make
```
If the compilation finished succefully, a file named `obj-intel64/branchExt.so` should be created inside the working folder.
## How to use
This is how the tool should be called
```sh
$ ./gen_trace.sh <program> <trace_name>
```
After execution, two log files named `<trace_name>.bz2` and `<trace_name>.txt` will be created. The first one containing all the information about branched executed by `<program>`  in a compressed version. Following is the sample of uncompressed output:
```
// Branch Address, Branch Target, (Taken-Not taken), (Conditional-Unconditional), (Call-Not Call), (Ret-Not Ret), (Direct-NotDirect)
```
```
0x24763089	0x24763128	0	1	0	0	1
0x247630be	0x247630da	1	0	0	0	1
0x247630de	0x247630c9	1	1	0	0	1
0x247630d8	0x24763128	0	1	0	0	1
0x247630de	0x247630c9	1	1	0	0	1
0x247630d8	0x24763128	0	1	0	0	1
0x247630de	0x247630c9	0	1	0	0	1
0x247630ea	0x247630c0	0	1	0	0	1
0x247630f8	0x24763108	1	1	0	0	1
0x24763112	0x247635d0	1	1	0	0	1
0x247635da	0x247630cd	0	1	0	0	1
0x247635e9	0x247630c9	1	0	0	0	1
0x247630d8	0x24763128	0	1	0	0	1
0x247630de	0x247630c9	1	1	0	0	1
0x247630d8	0x24763128	0	1	0	0	1
0x247630de	0x247630c9	1	1	0	0	1
```

while the latter one contains static information about the executed branches:
```
!!! Number of Instructions = 20573395
!!! Number of Unconditional branches = 23622
!!! Number of Conditional branches = 91429
!!! Number of Call branches = 6902
!!! Number of Ret branches = 6898
------------------------------------
```

About `<trace_name>`, the first column is the Branch Address, the second column is the Branch Address, the third column is `1` if it is taken, the fourth one is `1` if the branch is conditional, the fifth one is `1` if it is a call instruction, the sixth one is `1` if it is a RET instruction, the seventh one is `1` if it is direct branch

Please have look at following lines in branchExt.cpp to understand the tools options:

```c++
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "branches", "specifies the output file name prefix.");

KNOB<string> KnobHowManySet(KNOB_MODE_WRITEONCE, "pintool", "b", "1", "Specifies how many set should be created.");

KNOB<string> KnobHowManyBranch(KNOB_MODE_WRITEONCE, "pintool", "m", "-1", "Specifies how many instructions should be probed.");

KNOB<string> KnobOffset(KNOB_MODE_WRITEONCE, "pintool", "f", "0", "Starts saving instructions after seeing the first `f` instruction.");
```