#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"

int main(int argc, char *argv[]) {
    hid_t file_id;          // variable to store the file identifier
    hid_t dataset_id;       // variable to store the dataset identifier
    hid_t space_id;         // variable to store the space identifier
    hsize_t dims[2];        // array to store the dimensions of the dataset
    int * data;          // array to store the data read from the dataset
    int i, j;
    
    data = (int *)malloc(16777216 * sizeof(int));
    // open the .h5 file for reading
    file_id = H5Fopen("hdf5.h5", H5F_ACC_RDONLY, H5P_DEFAULT);
    
    // open the dataset within the file
    dataset_id = H5Dopen2(file_id, "IntArray", H5P_DEFAULT);
    
    // get the space identifier for the dataset
    space_id = H5Dget_space(dataset_id);
    
    // get the dimensions of the dataset
    H5Sget_simple_extent_dims(space_id, dims, NULL);
    
    // read the data from the dataset
    H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    
    // loop through the data and print it
    // for (i = 0; i < dims[0]; i++) {
    //     for (j = 0; j < dims[1]; j++) {
    //         printf("%d ", data[i * dims[1] + j]);
    //     }
    //     printf("\n");
    // }
    printf("%d ", data[3927 * 4096 + 3975]);
    
    // close the space identifier
    H5Sclose(space_id);
    
    // close the dataset identifier
    H5Dclose(dataset_id);
    
    // close the file
    H5Fclose(file_id);
    
    return 0;
}
