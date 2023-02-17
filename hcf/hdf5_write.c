#include <hdf5.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE "example.h5"
#define DATASET "dataset"
#define DIM0 5
#define DIM1 3

int main(void) {
    hid_t file_id, dataset_id, dataspace_id;
    herr_t status;
    hsize_t dims[2];
    int data[DIM0][DIM1];

    // Initialize data
    for (int i = 0; i < DIM0; i++) {
        for (int j = 0; j < DIM1; j++) {
            data[i][j] = i * DIM1 + j;
        }
    }

    // Create a new file using default properties
    file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        fprintf(stderr, "Failed to create file\n");
        exit(1);
    }

    // Define the size of the dataset
    dims[0] = DIM0;
    dims[1] = DIM1;
    dataspace_id = H5Screate_simple(2, dims, NULL);

    // Create the dataset
    dataset_id = H5Dcreate(file_id, DATASET, H5T_STD_I32BE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset_id < 0) {
        fprintf(stderr, "Failed to create dataset\n");
        exit(1);
    }

    // Write data to the dataset
    status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (status < 0) {
        fprintf(stderr, "Failed to write data\n");
        exit(1);
    }

    // Close the resources
    status = H5Dclose(dataset_id);
    status = H5Sclose(dataspace_id);
    status = H5Fclose(file_id);

    return 0;
}