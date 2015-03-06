//
//  oddEvenSort.h
//  oddEven
//
//  Created by Simon Hewitt on 04/03/2015.
//  Copyright (c) 2015 Simon Hewitt. All rights reserved.
//

#ifndef __oddEven__oddEvenSort__
#define __oddEven__oddEvenSort__

#include <stdio.h>
#include <OpenCL/cl.h>


void OddEvenKernel ( cl_uint *inputArray,
                    cl_uint *oddArray,
                    cl_uint *evenArray,
                    cl_uint *oddCount,
                    cl_uint *evenCount,
                    cl_uint *configData,
                    cl_uint get_global_id);

void dummySort ( cl_uint *inputArray,
                cl_uint *oddArray,
                cl_uint *evenArray,
                cl_uint *oddCount,
                cl_uint *evenCount,
                cl_uint *configData,
                cl_uint get_global_id);


#endif /* defined(__oddEven__oddEvenSort__) */
