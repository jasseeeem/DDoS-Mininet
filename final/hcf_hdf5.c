#include <stddef.h>
#include <hdf5.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE "hdf5.h5"
#define DATASETNAME "IntArray"
#define NX 4096
#define NY 4096
#define RANK 2

int update_hop_count(int row, int col, int hop_count)
{
    hid_t file, dataset;
    hid_t datatype, dataspace;
    herr_t status;

    int *data;
    /* dynamically allocating memory to the matrix*/
    data = (int *)malloc(NX * NY * sizeof(int));

    file = H5Fopen(FILE, H5F_ACC_RDWR, H5P_DEFAULT);
    dataset = H5Dopen2(file, DATASETNAME, H5P_DEFAULT);
    status = H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    printf("[%d,%d] = %d\n", row, col, hop_count);
    data[row * 4096 + col] = hop_count;
    status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    H5Dclose(dataset);
    H5Fclose(file);
    free(data);
    return 0;
}

// int main()
int main_()
{
    hid_t file, dataset;       /* file and dataset handles */
    hid_t datatype, dataspace; /* handles */
    hsize_t dimsf[2];          /* dataset dimensions */
    herr_t status;

    int *data; /* data to write */
    int i, j;

    data = (int *)malloc(16777216 * sizeof(int));
    memset(data, -1, NX * NY * sizeof(int));
    file = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    dimsf[0] = NX;
    dimsf[1] = NY;
    dataspace = H5Screate_simple(RANK, dimsf, NULL);

    datatype = H5Tcopy(H5T_NATIVE_INT);
    status = H5Tset_order(datatype, H5T_ORDER_LE);

    dataset = H5Dcreate(file, DATASETNAME, datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Dclose(dataset);
    H5Fclose(file);

    return 0;
}
