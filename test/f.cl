__kernel void f(__global int *in, __global int *out)
{
    //out[get_global_id(0)] = in[get_local_id(0)]*2;
    out[get_global_id(0)] = 8;
}

