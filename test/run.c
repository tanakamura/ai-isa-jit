#include <stdio.h>
#include <CL/cl.h>
#include <getopt.h>
#include <string.h>
#include "ai-isa-jit/ai-isa-jit.h"

int
main(int argc, char **argv)
{
    cl_uint num;
    cl_int err;
    int platform_idx = -1;
    cl_platform_id *plat_ids;
    int i;
    size_t sz;
    cl_device_id *gpu_devs;
    cl_context_properties cps[3];
    cl_context context;
    int opt;
    char *input;
    int run_size = 1024;
    struct AIISA_Program prog;

    clGetPlatformIDs(0, NULL, &num);

    plat_ids = (cl_platform_id*)malloc(sizeof(*plat_ids) * num);
    clGetPlatformIDs(num, plat_ids, NULL);

    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
        case 'n':
            run_size = atoi(optarg);
            break;

        default:
            puts("usage : run in.cl");
            return 1;
        }
    }

    if (optind >= argc) {
        puts("usage : run in.cl");
        return 1;
    }

    input = argv[optind];

    for (i=0; i<(int)num; i++) {
        char name[1024];
        size_t len;
        clGetPlatformInfo(plat_ids[i], CL_PLATFORM_VENDOR, sizeof(name), name, &len);

        //puts(name);
        if (strcmp(name, "Advanced Micro Devices, Inc.") == 0) {
            platform_idx = i;
            break;
        }
    }

    if (platform_idx == -1) {
        puts("no amd");
        return -1;
    }

    clGetDeviceIDs(plat_ids[platform_idx], CL_DEVICE_TYPE_GPU, 0, NULL, &num);
    if (num == 0) {
        puts("no gpu");
        return -1;
    }

    gpu_devs = (cl_device_id*)malloc(sizeof(gpu_devs[0]) * 1);
    //clGetDeviceIDs(plat_ids[platform_idx], CL_DEVICE_TYPE_GPU, num, gpu_devs, NULL);

    cps[0] = CL_CONTEXT_PLATFORM;
    cps[1] = (cl_context_properties)plat_ids[platform_idx];
    cps[2] = 0;

    context = clCreateContextFromType(cps, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
    clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(gpu_devs), gpu_devs, &sz);

    {
        char name[1024];
        size_t sz;
        clGetDeviceInfo(gpu_devs[0], CL_DEVICE_NAME, sizeof(name), name, &sz);

        puts(name);
    }

    //puts(input);

    aiisa_build_binary_from_cl(&prog, context, gpu_devs[0], input);

    aiisa_replace_text(&prog, NULL);

    {
        cl_program cl_prog;
        const unsigned char *bin[1];
        size_t bin_size[1];
        cl_kernel ker;
        cl_mem in, out;
        cl_command_queue queue;
        size_t global_size[3];
        cl_int stat;

        bin[0] = prog.cl_binary;
        bin_size[0] = prog.size;

        cl_prog = clCreateProgramWithBinary(context, 1, gpu_devs, bin_size, bin, NULL, NULL);
        clBuildProgram(cl_prog, 1, gpu_devs, NULL, NULL, NULL);
        ker = clCreateKernel(cl_prog, "f", &err);

        in = clCreateBuffer(context, CL_MEM_READ_WRITE, run_size * sizeof(int), NULL, &err);
        out = clCreateBuffer(context, CL_MEM_READ_WRITE, run_size * sizeof(int), NULL, &err);

        queue = clCreateCommandQueue(context, gpu_devs[0], 0, NULL);

        clSetKernelArg(ker, 0, sizeof(cl_mem), &in);
        clSetKernelArg(ker, 1, sizeof(cl_mem), &out);

        {
            int *ptr = (int*)clEnqueueMapBuffer(queue, in, CL_TRUE, CL_MAP_WRITE, 0, run_size*sizeof(int), 0, NULL, NULL, NULL);
            int i;
            for (i=0; i<run_size; i++) {
                ptr[i] = i;
            }
            clEnqueueUnmapMemObject(queue, in, ptr, 0, NULL, NULL);
        }

        global_size[0] = run_size;
        err = clEnqueueNDRangeKernel(queue, ker, 1, NULL, global_size, NULL, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            puts("enqueue nd");
        }
        err = clFinish(queue);
        if (err != CL_SUCCESS) {
            puts("fini");
        }

        {
            int *ptr = (int*)clEnqueueMapBuffer(queue, out, CL_TRUE, CL_MAP_READ, 0, run_size*sizeof(int), 0, NULL, NULL, NULL);
            int i;
            for (i=0; i<run_size; i++) {
                printf("%d : %d\n", i, ptr[i]);
            }
            clEnqueueUnmapMemObject(queue, in, ptr, 0, NULL, NULL);
        }

    }
}
