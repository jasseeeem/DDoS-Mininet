#include <stddef.h>
#include <hdf5.h>
#include <stdio.h>
#include <stdlib.h>

#include "curl.c"

#define FILE "hdf5.h5"
#define DATASETNAME "IntArray"
#define NX 4096
#define NY 4096
#define RANK 2
#define ALLOWED_HOP_COUNT_DIFFERENCE 2
#define INITIAL_HOP_COUNT -1
#define INTERMEDIATE_VALUE -2

int hlim_to_hop_count(int hlim)
{
    if (hlim > 255 || hlim < 0)
        return -1;
    else if (hlim <= 64)
        return 64 - hlim;
    else if (hlim <= 128)
        return 128 - hlim;
    else
        return 255 - hlim;
}

int check_hop_count(char src_ip[], int row, int col, int hlim)
{
    hid_t file, dataset, dataspace, memspace;
    hsize_t start[2], count[2];
    herr_t status;

    int data, calculated_hop_count = hlim_to_hop_count(hlim);

    file = H5Fopen(FILE, H5F_ACC_RDWR, H5P_DEFAULT);
    // file = get_or_create_table();
    dataset = H5Dopen2(file, DATASETNAME, H5P_DEFAULT);
    dataspace = H5Dget_space(dataset);

    start[0] = row;
    start[1] = col;
    count[0] = 1;
    count[1] = 1;

    /* Create a memory space for the subset */
    memspace = H5Screate_simple(1, count, NULL);

    /* Select the hyperslab in the dataspace */
    H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start, NULL, count, NULL);

    status = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, &data);
    printf("[%d,%d] = %d -> %d\n", row, col, data, calculated_hop_count);

    // next line only for testing
    // data = calculated_hop_count;

    if (data == INITIAL_HOP_COUNT)
    {
        // data = INTERMEDIATE_VALUE;
        data = hlim_to_hop_count(curl_to_hlim(src_ip));
        status = H5Dwrite(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, &data);
        printf("Curled value: %d\n", data);
    }
    else if (abs(data - calculated_hop_count) <= ALLOWED_HOP_COUNT_DIFFERENCE)
    {
        status = H5Dwrite(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, &data);
        printf("✅ Hop Count within the allowed limit\n");
    }
    // else if (data == INTERMEDIATE_VALUE)
    // {
    //     printf("💥💥💥💥💥💥💥💥 Dropping packets as the hop count is being calculated\n");
    // }
    else
    {
        printf("❌ Hop Count does not match - Possible IP spoofing\n");
        // add drop rule
    }

    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset);
    H5Fclose(file);

    return 0;
}

void get_or_create_table()
{
    hid_t file, dataset;       /* file and dataset handles */
    hid_t datatype, dataspace; /* handles */
    hsize_t dimsf[2];          /* dataset dimensions */
    herr_t status;

    int i, j;

    H5E_BEGIN_TRY
    {

        if ((file = H5Fopen(FILE, H5F_ACC_RDWR, H5P_DEFAULT)) < 0)
        {
            file = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

            int *data; /* data to write */
            data = (int *)malloc(16777216 * sizeof(int));
            memset(data, INITIAL_HOP_COUNT, NX * NY * sizeof(int));

            dimsf[0] = NX;
            dimsf[1] = NY;
            dataspace = H5Screate_simple(RANK, dimsf, NULL);

            datatype = H5Tcopy(H5T_NATIVE_INT);
            status = H5Tset_order(datatype, H5T_ORDER_LE);

            dataset = H5Dcreate(file, DATASETNAME, datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

            status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

            // veno?
            H5Sclose(dataspace);
            H5Tclose(datatype);
            H5Dclose(dataset);
        }
    }
    H5E_END_TRY;
    H5Fclose(file);
}

// int main()
// {
//     get_or_create_table();
// }