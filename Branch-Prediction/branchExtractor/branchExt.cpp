/*BEGIN_LEGAL
Intel Open Source License

Copyright (c) 2002-2018 Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

/*
    Following code has been modified by "Mohammadreza Baharani"
    UNC, Charlotte, NC, 28223 USA

    This code extracts the number of instruction executed by the processors and logs
    the branches. It produces two log files name `generalInfo.out` consisting
    information about the total number of instuctions and `branches.out`(default)
    consisting following information:
    BRANCH_TARGET `S` `K` `C` BRANCH_ADDR # BASE ??????
    where `S` can be `1`(Taken) or `0`(Not taken) and `K` can be `1`(Conditional) or
    `0`(Unconditional), and `C` can be `1`(Call ints.) or `0`(Not call instruction)
    and ?????? is the disassembled instruction and its opcode.
*/

// T = 1, C = 1      ,  Call = 0      ,  Ret = 0   ,  Direct = 1
// (T-N), (Con-Uncon), (Call-NotCall), (Ret-NotRet), (Direct-NotDirect), (first_inst_count_after_offset)

#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <map>
#include "pin.H"
#include "instlib.H"

using namespace std;

#define axuliryFileName "generalInfo"
std::map<ADDRINT, std::string> disAssemblyMap;

static ADDRINT dl_debug_state_Addr = 0;
static ADDRINT dl_debug_state_AddrEnd = 0;
static BOOL justFoundDlDebugState = FALSE;

ofstream OutFile;
ofstream axuFile;
// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;
static UINT64 cbcount = 0;
static UINT64 ubcount = 0;
static UINT64 callcount = 0;
static UINT64 retcount = 0;
static int64_t howManyBranch = 0;
static UINT64 howManySet = 0;
static UINT64 fileCounter = 0;
static UINT64 offset_inst = 0;
static UINT64 first_inst_count_after_offset = 0;
static bool first_record = true;
static bool record = false;
static ostringstream filePrefix;

static UINT64 CBCOUNT_LIMIT = 10000000;
static UINT64 prev_cbcount = -1;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "branches", "specifies the output file name prefix.");

KNOB<string> KnobHowManySet(KNOB_MODE_WRITEONCE, "pintool", "b", "1", "Specifies how many set should be created.");

KNOB<string> KnobHowManyBranch(KNOB_MODE_WRITEONCE, "pintool", "m", "-1", "Specifies how many instructions should be probed. -1 for probing whole program.");

KNOB<string> KnobOffset(KNOB_MODE_WRITEONCE, "pintool", "f", "20000000", "Starts saving instructions after seeing the first `f` instruction.");
// KNOB<string> KnobOffset(KNOB_MODE_WRITEONCE, "pintool", "f", "0", "Starts saving instructions after seeing the first `f` instruction.");

VOID write_on_axu()
{
    axuFile << "!!! Number of Instructions = " << (icount - offset_inst - ((fileCounter - 1) * howManyBranch) + 1) << endl;
    axuFile << "!!! Number of Unconditional branches = " << ubcount << endl;
    axuFile << "!!! Number of Conditional branches = " << cbcount << endl;
    axuFile << "!!! Number of Call branches = " << callcount << endl;
    axuFile << "!!! Number of Ret branches = " << retcount << endl;

    axuFile.close();
}

VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    cout << "Logging data..." << endl;
    write_on_axu();
    OutFile.close();
}

VOID reset_var()
{
    cbcount = 0;
    ubcount = 0;
    callcount = 0;
    retcount = 0;
    first_inst_count_after_offset = 0;
}

UINT32 file_init()
{
    cout << "Writing " << fileCounter - 1 << endl;

    write_on_axu();

    OutFile.close();
    filePrefix.str("");
    filePrefix.clear();
    filePrefix << KnobOutputFile.Value() << "_" << fileCounter << ".out";
    OutFile.open(filePrefix.str().c_str());
    OutFile.setf(ios::showbase);

    filePrefix.str("");
    filePrefix.clear();
    filePrefix << axuliryFileName << "_" << fileCounter << ".out";
    axuFile.open(filePrefix.str().c_str());
    axuFile.setf(ios::showbase);

    reset_var();

    return 0;
}

// This function is called before every instruction is executed
VOID docount()
{
    // cerr<< "I:" << icount << "V:" << (howManyBranch+ offset_inst - 1) << (!((icount) % (howManyBranch+ offset_inst - 1))? "Tr":"Fa") << endl;
    if (howManyBranch > 0)
    {
        if (!((icount) % ((howManyBranch * (fileCounter + 1)) + offset_inst - 1)) && icount > 0)
        {
            fileCounter++;
            if (fileCounter > howManySet - 1)
            {
                cout << "Exiting because of user conditions" << endl;
                Fini(0, 0);
                exit(0);
            }
            else
            {
                file_init();
            }
        }
    }

    icount++;

    if (cbcount != prev_cbcount && cbcount % 10000 == 0)
        cout << icount << " "<< cbcount << endl;
    prev_cbcount = cbcount;

    if (cbcount >= CBCOUNT_LIMIT)
    {
        fileCounter++;
        cout << "Exiting because of CBCOUNT_LIMIT" << endl;
        Fini(0, 0);
        exit(0);
    }

    if (icount >= offset_inst && fileCounter == 0)
    {
        // cout << "Here!" << endl;
        first_inst_count_after_offset++; // Although here is going to be increased by one, it will be set to 1 whenever it reaches to fist branch;
        record = true;
    }
    else if (fileCounter > 0)
    {
        first_inst_count_after_offset++;
    }
}

VOID ImageLoad(IMG img, VOID *v)
{

    printf("ImageLoad %s\n", IMG_Name(img).c_str());
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
    {
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
        {
            if (strcmp(RTN_Name(rtn).c_str(), "_dl_debug_state") == 0)
            {
                printf("  RTN %s at %p\n", RTN_Name(rtn).c_str(), reinterpret_cast<void *>(RTN_Address(rtn)));
                printf("    ** found _dl_debug_state\n");
                dl_debug_state_Addr = RTN_Address(rtn);
                justFoundDlDebugState = TRUE;
            }
            else if (justFoundDlDebugState)
            {
                printf("  RTN %s at %p\n", RTN_Name(rtn).c_str(), reinterpret_cast<void *>(RTN_Address(rtn)));
                dl_debug_state_AddrEnd = RTN_Address(rtn);
                justFoundDlDebugState = FALSE;
                printf("    ** _dl_debug_state from %p to %p\n", reinterpret_cast<void *>(dl_debug_state_Addr), reinterpret_cast<void *>(dl_debug_state_AddrEnd));
            }
        }
    }
}

/************
 *
 * JMP segment
 *
 */

static VOID UnconDirectJMP(ADDRINT ip, ADDRINT target, BOOL taken)
{

    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t0"                         // Unconditional
            << "\t0"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t1"                         // Direct
            << "\n"
            << flush;
    ubcount++;
}

static VOID UnconUnDirectJMP(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t0"                         // Unconditional
            << "\t0"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t0"                         // Not Direct
            << "\n"
            << flush;
    ubcount++;
}

static VOID ConDirectJMP(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t1"                         // Conditional
            << "\t0"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t1"                         // Direct
            << "\n"
            << flush;
    cbcount++;
}
static VOID ConUnDirectJMP(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t1"                         // Conditional
            << "\t0"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t0"                         // Not Direct
            << "\n"
            << flush;
    cbcount++;
}
//****************************************************************

/************
 *
 * Ret segment
 *
 */

static VOID UnconDirectRet(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t0"                         // Unconditional
            << "\t0"                         // Not Call
            << "\t1"                         // Ret
            << "\t1"                         // Direct
            << "\n"
            << flush;
    ubcount++;
    retcount++;
}

static VOID UnconUnDirectRet(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t0"                         // Unconditional
            << "\t0"                         // Not Call
            << "\t1"                         // Ret
            << "\t0"                         // Not Direct
            << "\n"
            << flush;
    ubcount++;
    retcount++;
}

static VOID ConDirectRet(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t1"                         // Unconditional
            << "\t0"                         // Not Call
            << "\t1"                         // Not Ret
            << "\t1"                         // Direct
            << "\n"
            << flush;
    cbcount++;
    retcount++;
}
static VOID ConUnDirectRet(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t1"                         // Unconditional
            << "\t0"                         // Not Call
            << "\t1"                         // Not Ret
            << "\t0"                         // Direct
            << "\n"
            << flush;
    cbcount++;
    retcount++;
}
//****************************************************************

/************
 *
 * Call segment
 *
 */
static VOID UnconDirectCall(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t0"                         // Unconditional
            << "\t1"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t1"                         // Direct
            << "\n"
            << flush;
    ubcount++;
    callcount++;
}
static VOID UnconUnDirectCall(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t0"                         // Unconditional
            << "\t1"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t0"                         // Direct
            << "\n"
            << flush;
    ubcount++;
    callcount++;
}
static VOID ConDirectCall(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t1"                         // Unconditional
            << "\t1"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t1"                         // Direct
            << "\n"
            << flush;
    cbcount++;
    callcount++;
}
static VOID ConUnDirectCall(ADDRINT ip, ADDRINT target, BOOL taken)
{
    OutFile << std::hex
            << (ip & 0xffffffff)             // PC
            << "\t" << (target & 0xffffffff) // Target
            << (taken ? "\t1" : "\t0")       // T-N
            << "\t1"                         // Unconditional
            << "\t1"                         // Not Call
            << "\t0"                         // Not Ret
            << "\t0"                         // Direct
            << "\n"
            << flush;
    cbcount++;
    callcount++;
}
//****************************************************************

static VOID Instruction(INS ins, VOID *v)
{
    // Insert a call to docount before every instruction, no arguments are passed

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);

    if (record)
    {
        if (INS_IsValidForIpointTakenBranch(ins))
        {
            if (first_record)
            { // Detected the first branch
                first_inst_count_after_offset = 1;
                first_record = false;
            }
            if (INS_HasFallThrough(ins) == false)
            { // It is unconditional branch
                if (INS_IsCall(ins))
                { // It is call
                    if (INS_IsDirectControlFlow(ins) == true)
                    { // direct
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)UnconDirectCall, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                    else
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)UnconUnDirectCall, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                }
                else if (INS_IsRet(ins))
                { // It is RET
                    if (INS_IsDirectControlFlow(ins) == true)
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)UnconDirectRet, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                    else
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)UnconUnDirectRet, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                }
                else
                { // It is JMP
                    if (INS_IsDirectControlFlow(ins) == true)
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)UnconDirectJMP, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                    else
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)UnconUnDirectJMP, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                }
            }
            else
            { // It is conditional branch
                if (INS_IsCall(ins))
                { // It is call
                    if (INS_IsDirectControlFlow(ins) == true)
                    { // direct
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ConDirectCall, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                    else
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ConUnDirectCall, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                }
                else if (INS_IsRet(ins))
                { // It is RET
                    if (INS_IsDirectControlFlow(ins) == true)
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ConDirectRet, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                    else
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ConUnDirectRet, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                }
                else
                { // It is JMP
                    if (INS_IsDirectControlFlow(ins) == true)
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ConDirectJMP, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                    else
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ConUnDirectJMP, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
                    }
                }
            }
        }
    }
    // We do not care about instrunctions that are not branches.
    // else
    //    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)AtNonBranch, IARG_INST_PTR, IARG_END);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed and log program branches" << endl;
    cerr << endl
         << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

INT32 InitFile()
{
    filePrefix.str("");
    filePrefix.clear();
    filePrefix << KnobOutputFile.Value() << "_" << fileCounter << ".out";
    OutFile.open(filePrefix.str().c_str());
    OutFile.setf(ios::showbase);

    filePrefix.str("");
    filePrefix.clear();
    filePrefix << axuliryFileName << "_" << fileCounter << ".out";
    axuFile.open(filePrefix.str().c_str());
    axuFile.setf(ios::showbase);

    howManyBranch = strtoull(KnobHowManyBranch.Value().c_str(), NULL, 0);
    howManySet = strtoull(KnobHowManySet.Value().c_str(), NULL, 0);
    offset_inst = strtoull(KnobOffset.Value().c_str(), NULL, 0);
    cout << "My offset " << offset_inst << endl;

    cout << KnobHowManyBranch.Value() << endl;

    return 0;
}

int main(INT32 argc, CHAR **argv)
{
    PIN_Init(argc, argv);
    PIN_InitSymbols();

    InitFile();

    INS_AddInstrumentFunction(Instruction, 0);
    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
