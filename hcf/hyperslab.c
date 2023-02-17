#include <hdf5.h>
#include <stdio.h>

int main(void) {
    hid_t file_id, dataset_id, dataspace_id, memspace_id;
    hsize_t dims[2], start[2], count[2];
    int data[10][10];

    /* Open the file */
    file_id = H5Fopen("matrix.h5", H5F_ACC_RDONLY, H5P_DEFAULT);

    /* Open the dataset */
    dataset_id = H5Dopen2(file_id, "matrix", H5P_DEFAULT);

    /* Get the dataspace */
    dataspace_id = H5Dget_space(dataset_id);

    /* Get the dimensions of the dataspace */
    H5Sget_simple_extent_dims(dataspace_id, dims, NULL);

    /* Define the subset to read */
    start[0] = 2;
    start[1] = 2;
    count[0] = 3;
    count[1] = 3;

    /* Create a memory space for the subset */
    memspace_id = H5Screate_simple(2, count, NULL);

    /* Select the hyperslab in the dataspace */
    H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL);

    /* Read the subset from the file into memory */
    H5Dread(dataset_id, H5T_NATIVE_INT, memspace_id, dataspace_id, H5P_DEFAULT, data);

    /* Close the objects */
    H5Sclose(memspace_id);
    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
    H5Fclose(file_id);

    /* Print the subset */
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%d ", data[i][j]);
        }
        printf("\n");
    }

    return 0;
}
