#include <iostream>

using namespace std;
//
// Simon Hewitt 806068
// Originally based on sample code from OpenCL Programming Guide, Munshi et al
// But extensively modified
// kernel completely rewritten (of course)

// main.cpp
// M67 Assignment 2, Filter random numbers into odd-first while maintaining sequence
// Expects to find sort.cl in same folder as source (and uses regex to do this, as follows:
// If source is  /Users/simon/MSc/M67_GPU/Ass2/806068/main.cpp
// Will try to open /Users/simon/MSc/M67_GPU/Ass2/806068/sort.cl
// source file can be any name , but must be .cpp and no other .cpp in the path,
// /Users/simon/MSc/M67_GPU/Ass2.cpp/806068/main.cpp will totally fail!
//

#include <fstream>
#include <sstream>
#include <regex>
#include <math.h>

#include "oddEvenSort.h"



#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include "CL/cl.h"
#endif

//#define WINDOWS
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else


#define GetCurrentDir getcwd
#endif

struct errorStruct {
    std::string errText;
    int errNum;

    errorStruct(string t, int n) : errText(t), errNum(n) {
    }
};

///
//  Constants
//
//const int WORKITEMS = 128;          // Number of workItems in work group
const int WORKITEMS = 16;          // Number of workItems in work group
const int CONFIG_ARRAY_SIZE = 4;    // passes configuration items to the kernel
const int MEM_OBJECTS_SIZE = 6;     // Number of memory objects passed to kernel
const int MAX_DEVICES = 8;          // Maximum number of devices we will read (should be plenty)
const int MAX_WORK_DIMS = 3;        // 3 dimensions!
const int WORK_SIZE_POWER = 8;      // The array to sort is 2^WORK_SIZE_POWER, in the range (8..25)
const int WORK_SIZE = 1<< WORK_SIZE_POWER; // The size of the input and results arrays
const int UNIT_SPAN = WORK_SIZE / WORKITEMS; // Number of elements to be processed per work-unit



struct CL_DeviceDataStruct {
    string *deviceName;
    cl_device_type deviceType;
    cl_uint computeUnits;
    cl_ulong globalMemSize;
    cl_ulong localMemorySize;
    size_t maxWorkGroupSize;
    cl_uint clockSpeed;
    size_t maxWorkItems[MAX_WORK_DIMS];
    cl_device_fp_config dbl_fp_config;
    cl_device_fp_config sngl_fp_config;
};

class CL_DeviceData {
/****
* Eases the grief of calling clGetDiviceInfo
* Call the static class method getNumDevices(cl_platform_id *p) first to find out how many there are
* Then create an object for each one you want using constructor (cl_platform_id *p, int n)
* Then use get methods to return the required data
*/

private:
    CL_DeviceDataStruct CL_Data;

public:

    static int getNumDevices(cl_platform_id *p) {
        cl_uint  numDevices;
        int errNo;
        errno = clGetDeviceIDs (*p, CL_DEVICE_TYPE_ALL, MAX_DEVICES, NULL, &numDevices);
        if (errNo != CL_SUCCESS) throw (errorStruct("CL_DeviceData error", errno));
        return (int)numDevices;
    }

    void getCL_DeviceData (CL_DeviceDataStruct *d)
    {
        d = &CL_Data;

    }

    // Constructor - gets the data
    CL_DeviceData(cl_platform_id *p, int devNum) {
        // Find out what devices we have on this installation
        int errNo;
        cl_uint numDevices;
        cl_device_id devs[MAX_DEVICES], dev;
        char buffer[128];
        std::string buf;

        errNo = clGetDeviceIDs(*p, CL_DEVICE_TYPE_ALL, MAX_DEVICES, devs, &numDevices);
        if (errNo != CL_SUCCESS) throw (errorStruct("CL_DeviceData error", errno));
        if (devNum > numDevices) throw (errorStruct("CL_DeviceData parameter error", devNum));
        dev = devs[devNum];

        errno |= clGetDeviceInfo(dev, CL_DEVICE_NAME, 127, buffer, NULL);
        CL_Data.deviceName = new std::string(buffer);

        errno |= clGetDeviceInfo(dev, CL_DEVICE_TYPE, sizeof(CL_Data.deviceType), &CL_Data.deviceType, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &CL_Data.computeUnits, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * 3, CL_Data.maxWorkItems, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &CL_Data.maxWorkGroupSize, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &CL_Data.globalMemSize, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &CL_Data.localMemorySize, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &CL_Data.clockSpeed, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * MAX_DEVICES, CL_Data.maxWorkItems, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(cl_device_fp_config), &CL_Data.dbl_fp_config, NULL);
        errno |= clGetDeviceInfo(dev, CL_DEVICE_SINGLE_FP_CONFIG, sizeof(cl_device_fp_config), &CL_Data.sngl_fp_config, NULL);

        if (errNo != CL_SUCCESS) throw (errorStruct("CL_DeviceDataclgetDevices error", errno));
    }

    string toString()
    {
        std::stringstream r;
        std::string dn = *new string(*CL_Data.deviceName);
        
        r << "Device Name : " << dn << ", device type: " << CL_Data.deviceType  << ", Compute units : " << CL_Data.computeUnits << std::endl <<
        "Max work items [3] : " << CL_Data.maxWorkItems[0] << ", "  << CL_Data.maxWorkItems[1] << ", "  << CL_Data.maxWorkItems[2] << std::endl <<
        "Max work group size : " << CL_Data.maxWorkGroupSize << ", Clock speed : " << CL_Data.clockSpeed << std::endl <<
        "Global memory : " << CL_Data.globalMemSize << ", Local memory : " <<CL_Data.localMemorySize << std::endl <<
        "Single FP type : " << CL_Data.sngl_fp_config << ", Double FP type : " << CL_Data.dbl_fp_config <<  std::endl;
        
        return r.str();
    }
}; // class CL_DeviceData





///
//  Create an OpenCL context on the first available platform using
//  either a GPU or CPU depending on what is available.
//  note type cl_context is a pointer
cl_context CreateContext() {
    cl_int errNum;
    cl_uint platformCount;
    cl_platform_id firstPlatformId;
    cl_context context = NULL;

    // First, select an OpenCL platform to run on.  We just chose the first, usually only
    errNum = clGetPlatformIDs(1, &firstPlatformId, &platformCount);
    if (errNum != CL_SUCCESS || platformCount <= 0)
        throw (errorStruct("Failed to find any OpenCL platforms.", 0));

    CL_DeviceData *dd;
    dd = new CL_DeviceData(&firstPlatformId, 0);
    string ddString = dd->toString();
    std::cout << ddString  <<std::endl;
    dd = new CL_DeviceData(&firstPlatformId, 1);
    ddString = dd->toString();
    std::cout << ddString  <<std::endl;

    // Next, create an OpenCL context on the platform.  Attempt to
    // create a GPU-based context, and if that fails, try to create
    // a CPU-based context. This is necessary on my HP EliteBook 8440p
    // which although supposed to have an Intel GPU has no DLL to access it,
    // so using CPU emulation (works fine on macBook, also an Intel GPU)
    cl_context_properties contextProperties[] =
            {CL_CONTEXT_PLATFORM, (cl_context_properties) firstPlatformId, 0};

    context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
            NULL, NULL, &errNum);

    if (errNum != CL_SUCCESS) {
        // fails to this point on HP EliteBook...
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;
        // But this succeeds
        context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
                NULL, NULL, &errNum);
        if (errNum != CL_SUCCESS)
            throw (errorStruct("Failed to create an OpenCL GPU or CPU context.", 0));
    }

    return context;
}

///
//  Create a command queue on the first device available on the
//  context
//
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device) {
    cl_int errNum;
    cl_device_id *devices;
    cl_command_queue commandQueue = NULL;
    size_t deviceBufferSize = -1;

    // First get the size of the devices buffer
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
    if (errNum != CL_SUCCESS)
        throw  (errorStruct("Failed call to clGetContextInfo", errNum));


    if (deviceBufferSize <= 0)
        throw  (errorStruct("No devices available.", 0));

    // Allocate memory for the devices buffer
    devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
    if (errNum != CL_SUCCESS) {
        delete[] devices;
        throw (errorStruct("Failed to get device IDs", errNum));
    }

    // Just choose first available device (only one nyway on laptops generally)
    commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
    if (commandQueue == NULL) {
        delete[] devices;
        throw (errorStruct("Failed to create commandQueue for device 0", 0));
    }

    *device = devices[0];
    delete[] devices;
    return commandQueue;
}

//void showPWD() {
//    char cCurrentPath[FILENAME_MAX];
//
//    if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))) {
//        return;
//    }
//
//    cCurrentPath[sizeof(cCurrentPath) - 1] = '\0'; /* not really required */
//
//    printf("The current working directory is %s", cCurrentPath);
//}

///
//  Create an OpenCL program from the kernel source file
//  open filename passed in, read, compile.
//
//  Complex fiddling to open .cl in same directory as source file main.cpp
//  As Cmake runs the executable in the weirdest places!

cl_program CreateProgram(cl_context context, cl_device_id device, string fileName) {
    cl_int errNum;
    cl_program program;

    // Get the folder where the source code was compiled from, and open the .cl file from the same folder
    std::string home = std::string(__FILE__);  // Get .cpp folder

    std::regex r("\\w*.cpp\\b");  // and swap the .cl filename into it. Regex matches ANY word ending in .cpp
    home = std::regex_replace(home, r, fileName);
    //std::cout << home << std::endl;
    fileName = home;
    //std::cout << "File : " << fileName << std::endl;


    std::ifstream kernelFile(fileName, std::ios::in);
    if (!kernelFile.is_open()) {
        std::cerr << "Failed to open file for reading: " << fileName << std::endl;
        throw (errorStruct("Failed to open Kernel file", errno));
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();  // read entire file

    std::string srcStdStr = oss.str();  // and get as a string
    const char *srcStr = srcStdStr.c_str();
    // compile it
    program = clCreateProgramWithSource(context, 1,
            &srcStr,
            NULL, NULL);
    if (program == NULL) {
        throw (errorStruct("Failed to create CL program from source.", 0));
    }

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS) {
        // Determine the reason for the error
        static char buildLog[2048];        // static to allocate on heap, as it is large
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                sizeof(buildLog), buildLog, NULL);

        std::cerr << "Error in kernel: " << std::endl;
        std::cerr << buildLog;
        clReleaseProgram(program);
        throw (errorStruct("Kernel error", errNum));
    }

    return program;
}

///
//  Create memory objects used as the arguments to the kernel

void create_mem_object (cl_context context, cl_mem_flags flags,  cl_mem &memObj, cl_uint objSize, cl_uint *array)
{
    cl_int errCode;
    memObj = clCreateBuffer(context, flags, objSize, array, &errCode);
    if (errCode != 0)
        throw(errorStruct("Error creating memory object", errCode));
}



///
//  Cleanup any created OpenCL resources
//
void Cleanup(cl_context context, cl_command_queue commandQueue,
        cl_program program, cl_kernel kernel, cl_mem memObjects[MEM_OBJECTS_SIZE]) {
    for (int i = 0; i < 3; i++) {
        if (memObjects[i] != 0)
            clReleaseMemObject(memObjects[i]);
    }
    if (commandQueue != 0)
        clReleaseCommandQueue(commandQueue);

    if (kernel != 0)
        clReleaseKernel(kernel);

    if (program != 0)
        clReleaseProgram(program);

    if (context != 0)
        clReleaseContext(context);
    
}


int createKernelProg(cl_kernel *kernel, cl_program program, cl_mem memObjects[],
                     int kernelArgsCount, const char *kernelName) {
    int errNum;
    string kn = kernelName;

    *kernel = clCreateKernel(program, kernelName, NULL);
    if (*kernel == NULL) {
        std::cerr << "Failed to create kernel named : " << kernelName << std::endl;
        throw (errorStruct(("Failed to create kernel named : " + kn), 0));
    }
    std::cout << "Created kernel : " << kernelName << " OK" << std::endl;

    // Set the kernel arguments
    errNum = 0;
    for (int i = 0; i < kernelArgsCount; i++) {
        errNum |= clSetKernelArg(*kernel, i, sizeof(cl_mem), &memObjects[i]);
    }
    
    if (errNum != CL_SUCCESS) {
        throw (errorStruct("Error setting kernel arguments.", errNum));
    }
    std::cout << "Created and attached memory objects OK" << std::endl;

    return CL_SUCCESS;
}

void dontExitYet() {
    // Read anything, just to keep the console open
    std::cout << "Key <Enter>" << std::endl;
    char str[101];
    std::cin.getline(str, 1);
}

void loadRandoms (cl_uint inputArray[])
{
    srand((unsigned int) time(0));
    for (int i = 0; i < WORK_SIZE; i++)
        inputArray[i] = rand();
}

bool checkResults (cl_uint inputArray[], cl_uint resultsArray[], int sz)
{
    bool OK = true;
    
    cl_uint *ipPtr, *rPtr, n ;
    
    ipPtr = inputArray;
    rPtr = resultsArray;
    cl_uint *endIpPtr = ipPtr + sz;
    cl_uint *endRPtr = rPtr + sz;
    
    while (((*rPtr & 0x01) == 1) && (rPtr <= endRPtr))
    {  // While result element is ODD and not end of the array
        n = *(rPtr++);
        std::cout << "Result : " << n << ", ";
        if (n & 0x01) {
            // ODD so n must be the next ODD number in the input array
            // so skip EVEN numbers
            while (((*ipPtr & 0x01) == 0) && (ipPtr <= endIpPtr)) ipPtr++;
            std::cout << "Input : " << *ipPtr << std::endl;
            if (n != *ipPtr)
            {
                std::cout << "RESULTS NOT THE SAME"  << std::endl;
            }
        }
        rPtr++; ipPtr++;
    }
    
    // and now the evens
    while (((*rPtr & 0x01) == 0) && (rPtr <= endRPtr))
    {  // While result element is ODD and not end of the array
        n = *(rPtr++);
        if (n & 0x00) {
            // EVEN so n must be the next EVEN number in the input array
            // so skip EVEN numbers
            while (((*ipPtr & 0x01) == 1) && (ipPtr <= endIpPtr)) ipPtr++;
            if (n != *ipPtr)
            {
                std::cout << "RESULTS NOT THE SAME"  << std::endl;
            }
        }
        rPtr++; ipPtr++;
    }
    
    return OK;
}

///***************************************************************
//	main()
//****************************************************************
int main(int argc, char **argv) {
    
    // OpenCL context variables
    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    
    // Data arrays
    cl_uint inputArray[WORK_SIZE];          // the input array, full of random numbers
    cl_uint resultsArray[WORK_SIZE];            // temporary Odd numbers  array
    cl_uint oddCountArray[WORKITEMS];
    cl_uint evenCountArray[WORKITEMS];
    
    cl_uint oddArray[WORK_SIZE];
    cl_uint evenArray[WORK_SIZE];
    
    
    // Sizes - just abbreviation to make code neater!
    const int elementArraySize = sizeof(cl_uint) * WORK_SIZE;
    const int workItemArraySize = sizeof(cl_uint) * WORKITEMS;
    
    cl_uint configData[CONFIG_ARRAY_SIZE];      // Configuration data for the work-item
    // Config data:     [0] is span, number of cells per work-unit,
    //                  [1] is number of work-units
    //                  [2] is returned as oddCount
    //                  [3] is returned as EvenCount
    
    
    cl_mem memObjects[MEM_OBJECTS_SIZE];    // Array of memory objects to be passed to kernel
    
    cl_int errNum;

    loadRandoms(inputArray);
    
    // set up config data for the kernel
    configData[0] = UNIT_SPAN;
    configData[1] = WORKITEMS;
    configData[2] = configData[3] = 0;

    size_t globalWorkSize[1] = {WORKITEMS};
    size_t localWorkSize[1] = {WORKITEMS};

    try {  // TRY initialisation block
        // Create an OpenCL context on first available platform
        context = CreateContext();
        std::cout << "Created Context OK" << std::endl;

        // Create a command-queue on the first device available on the created context
        commandQueue = CreateCommandQueue(context, &device);
        std::cout << "Created command queue OK" << std::endl;

        // Create OpenCL program from MonteCarlo.cl kernel source
        program = CreateProgram(context, device, "oddEvenSort.cl");
        std::cout << "Compiled kernel program  oddEvenSort.cl OK" << std::endl;

        // Create the memory objects that will be used as arguments to  kernel.
        // Input array, copied from local inputArray
        create_mem_object (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, memObjects[0], elementArraySize, inputArray);
        //  odd array, all odd numbers in sequence. Read when kernel complete
        create_mem_object (context, CL_MEM_READ_WRITE , memObjects[1], elementArraySize, NULL);
        // ditto even
        create_mem_object (context, CL_MEM_READ_WRITE , memObjects[2], elementArraySize, NULL);
        // oddCount and evencount, never seen by main code
        create_mem_object (context, CL_MEM_READ_WRITE , memObjects[3], workItemArraySize, NULL);
        create_mem_object (context, CL_MEM_READ_WRITE , memObjects[4], workItemArraySize, NULL);
        // Config data for the kernel. Main sends span, cl returns odd and even count
        create_mem_object (context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, memObjects[5], sizeof(int) * CONFIG_ARRAY_SIZE, configData);


        
        // Create OpenCL kernel
        errNum = createKernelProg(&kernel, program, memObjects, MEM_OBJECTS_SIZE,  "OddEvenKernel");
        if (errNum != CL_SUCCESS)
            throw (errorStruct("Error creating kernel part 1 for execution ", errNum));

        std::cout << "Starting openCL, : " << WORKITEMS << " kernels" << std::endl;

        //Queue the kernel up for execution across the array
        errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL,
                globalWorkSize, localWorkSize,
                0, NULL, NULL);
        if (errNum != CL_SUCCESS)
            throw (errorStruct("Error queuing kernel for execution ", errNum));
        
//
//        for (int i = 0; i < WORKITEMS; i++){
//            OddEvenKernel (inputArray, oddArray, evenArray, oddCountArray, evenCountArray, configData, i);
//        }
//        dummySort   (inputArray, oddArray, evenArray, oddCountArray, evenCountArray, configData, 0);
//        dummySort   (inputArray, oddArray, evenArray, oddCountArray, evenCountArray, configData, 1);
//        

        
        
        std::cout << "Completed Kernel  OK" << std::endl << std::endl;

        }
    catch (errorStruct errMesg) {
        // All of the initialisation routines throw(errorStruct) on any failure,
        // as there is no point going on.
        std::cerr << "Initialisation FAILED :\n" << errMesg.errText << endl;
        if (errMesg.errNum) std::cerr << "Error code : " << errMesg.errNum << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        std::exit(-1);
    }


    try { // Read the data back from kernel
    
        
        // Read 3 data objects, configData, oddArray, evenArray.
        // configData[2] has odd element count, [3] even element count
        errNum |= clEnqueueReadBuffer(commandQueue, memObjects[5], CL_TRUE,
                                      0, sizeof(int) * CONFIG_ARRAY_SIZE, configData,
                                      0, NULL, NULL);
        int oddNumsCount = configData[2];
        int evenNumsCount = configData[3];
        
        //        // Some clever stuff:-
        //        // configData[2] is number of ODD numbers, read exactly that number into results array
        //        errNum = clEnqueueReadBuffer(commandQueue, memObjects[1], CL_TRUE,
        //                                     0, sizeof(cl_uint) * oddNumsCount, resultsArray,
        //                                     0, NULL, NULL);
        //        // Now configData[3] is number of EVEN numbers, read those into the resulkts array
        //        // AFTER the odd numbers, i.e. resultsArray offset by configData[2]
        //        errNum |= clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE,
        //                                    0, sizeof(cl_uint) * evenNumsCount, &resultsArray[evenNumsCount],
        //                                    0, NULL, NULL);
        
                // Some clever stuff:-
                // configData[2] is number of ODD numbers, read exactly that number into results array
                errNum = clEnqueueReadBuffer(commandQueue, memObjects[1], CL_TRUE,
                                             0, WORK_SIZE, oddArray,
                                             0, NULL, NULL);
                // Now configData[3] is number of EVEN numbers, read those into the resulkts array
                // AFTER the odd numbers, i.e. resultsArray offset by configData[2]
                errNum |= clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE,
                                            0, WORK_SIZE, evenArray,
                                            0, NULL, NULL);
        
        
        
        if (errNum != CL_SUCCESS)
            throw(errorStruct("Error reading result buffers ", errNum));
    }
    catch (errorStruct e) {
        // All of the run routines throw(errorStruct) on any failure,
        // as there is no point going on.
        std::cerr << "Execution FAILED :\n" << e.errText << endl;
        if (e.errNum) std::cerr << "Error code : " << e.errNum << std::endl;
        Cleanup(context, commandQueue, program, kernel, memObjects);
        std::exit(-2);
    }
    
    // Got all the data back, just move into one array
    // (later)

    checkResults (inputArray, resultsArray, WORK_SIZE);
    

     // ALL DONE

    dontExitYet();

    return 0;
}
