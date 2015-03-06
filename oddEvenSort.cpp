//
//  oddEvenSort.cpp
//  oddEven
//
//  Created by Simon Hewitt on 04/03/2015.
//  Copyright (c) 2015 Simon Hewitt. All rights reserved.
//
#include <iostream>

using namespace std;
#include "oddEvenSort.h"
#include <OpenCL/cl.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <math.h>


#pragma OPENCL EXTENSION cl_khr_fp64 : enable


 void OddEvenKernel ( cl_uint *inputArray,
                              cl_uint *oddArray,
                              cl_uint *evenArray,
                              cl_uint *oddCount,
                              cl_uint *evenCount,
                              cl_uint *configData,
                              cl_uint get_global_id)



{
    // *inputArray      2^x random numbers in cl_uint range, where 8 <= x <= 25 (256..33,554,432)
    // *oddArray,       2^x, temp array of odd numbers in chunks
    // *evenArray,      ditto even
    // *oddCount,       globalID range (i.e. globalWorkSize) array that stores the number of ODDs sorted by each work unit
    // *evenCount,      ditto even
    // *configData      size 1, [0] is work item span, how many numbers to process, 2^x / GlobalWorkSize
    //                  globalWorkSize must be <= 2^8 (so each work item has at least ONE cell!)
    //                  so span is in the range 1..131025
    
    // Pass 1 is easy, for i = globalID * span to GlobalID * span + span, for each input cell, move it to the ODD or EVEN temporary arrays
    // Indexes to odd/even arrays start at globalID * span (same as input array), and are contiguously filled.
    // so each array can have (0..span) numbers in it
    // and the array is 'sparse', one or both have unused cells.
    // At the end of this loop, store the number of odd and even numbers in oddCount, evenCount, at index globalID
    
    // second pass - use the odd/even counts to condense the odd/even arrays into teh results array.
    
    // Lots of pointer bashing!
    
    cl_uint inputIndex, oddIndex, evenIndex;
    cl_uint baseIndex;
    cl_uint globalID;
    cl_uint span, n, workItems;
    
    globalID = get_global_id;
    span = configData[0];           //How many elements to process
    workItems = configData[1];      // how many workItems are working on this
    baseIndex = globalID * span;
    oddIndex = evenIndex = 0;
    
    for (inputIndex = 0; inputIndex < span; inputIndex++) {
        n = inputArray[baseIndex + inputIndex];
        if (n & 0x1)
            // its ODD
            oddArray[baseIndex + oddIndex++] = n;
        else
            evenArray[baseIndex + evenIndex++] = n;
    }
    // Sorted 1st pass, save the odd and even counts
    oddCount[globalID] = oddIndex;
    evenCount[globalID] = evenIndex;
    std::cout << "Iter : " << globalID << " baseIndex : " << baseIndex << " oddIndex : " << oddIndex <<
    " evenIndex : " << evenIndex << " Sum : " << oddIndex + evenIndex << std::endl;
    
    
    
} // oddEvenKernel (simulation)

void dummySort ( cl_uint *inputArray,
                cl_uint *oddArray,
                cl_uint *evenArray,
                cl_uint *oddCount,
                cl_uint *evenCount,
                cl_uint *configData,
                cl_uint get_global_id)
{
    // Thats the first step!! Sync all work-units in the work-group (there is only 1 work group)
    //barrier(CLK_GLOBAL_MEM_FENCE);
    
    // At this stage, all oddnumbers are in oddArray and even in evenArray
    // But the arrays are sparse. The next action is to condense these up using the odd and evenCount arrays
    // Only 2 work-units, odd and even
    cl_uint oddIndex, evenIndex;
    cl_uint baseIndex;
    cl_uint globalID;
    cl_uint span, workItems;
    cl_uint i, j;
    
    globalID = get_global_id;
    span = configData[0];           //How many elements to process
    workItems = configData[1];      // how many workItems are working on this
    baseIndex = globalID * span;
    oddIndex = evenIndex = 0;
    
    
    if (globalID == 0) {
        oddIndex = oddCount[0];  // Start moving odd numbers to this index
        for (i = 1; i < workItems; i++) { // NB no need to process step [0], already at the start of the array
            baseIndex = span * i;
            for (j = 0; j < oddCount[i]; j++)
            {
                // for each Odd number from GlobalID j from last step
                oddArray[oddIndex++] = oddArray[baseIndex + j];
            }
        }
        // done, all Odds nicely in sequence in OddArray, just save the odd-count for main code
        configData[2] = oddIndex;
    }
    
    if (globalID == 1) {
        evenIndex = evenCount[0];  // Start moving even numbers to this index
        for (i = 1; i < workItems; i++) { // NB no need to process step [0], already at the start of the array
            baseIndex = span * i;
            for (j = 0; j < evenCount[i]; j++)
            {
                // for each even number from GlobalID j from last step
                evenArray[evenIndex++] = evenArray[baseIndex + j];
            }
        }
        // done, all evens nicely in sequence in evenArray, just save the even-count for main code
        configData[3] = evenIndex;
    }
    
    // And thats it!
    // Now main will read configData to get the counts, and oddArray and evenArray to get the data,
    // and append even to odd for the final result.
    // No need for barrier here as main is already doing a blocking read.
    
}
